/*
    Options Numerics: on_websocket.c

    Copyright (C) 2023  Johnathan K Burchill

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "on_websocket.h"
#include "on_status.h"
#include "on_screen_io.h"

#include "on_api.h"
#include "on_remote.h"

#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>
#include <jansson.h>

#include <ncurses.h>

static bool wssSupportEnabled = false;
static bool wssSupportChecked = false;

extern volatile sig_atomic_t running;

int pio_wss_transfers_running = 0;
CURLM *multi_handle = NULL;
WssData data = {0};

// Call at beginning of program
int checkWebSocketSupport(ScreenState *screen)
{
    if (!wssSupportChecked)
    {
        curl_version_info_data *version = curl_version_info(CURLVERSION_NOW);
        int n = 0;
        bool haveWss = false;
        while (version->protocols[n] != NULL && !haveWss)
        {
            haveWss |= strcmp("wss", version->protocols[n]) == 0;
            n++;
        }
        wssSupportEnabled = haveWss;
        wssSupportChecked = true;
    }

    if (!wssSupportEnabled)
    {
        mvwprintw(screen->statusWindow, 0, 0, "Your CURL library does not have WebSocket (WSS); streaming disabled.");
        return ON_WSS_PROTOCOL_NOT_SUPPORTED;
    }

    return ON_OK;
}

static size_t polygonIoWssCallback(char *data, size_t size, size_t nmemb, void *userdata)
{

    if (!running)
        return ON_OK;

    char *jsonString = NULL;

    size_t realsize = size *nmemb;
    WssData *wssData = (WssData *)userdata;

    char *ptr = realloc(wssData->frame, wssData->size + realsize + 1);
    if (ptr == NULL)
        return ON_OK;

    wssData->frame = ptr;
    memcpy(&(wssData->frame[wssData->size]), data, realsize);
    wssData->size += realsize;
    wssData->frame[wssData->size] = 0;

    struct curl_ws_frame *frameInfo = curl_ws_meta(wssData->curl);

    if ((frameInfo->flags & CURLWS_CONT) == 0)
    {
        if (frameInfo->flags & CURLWS_TEXT)
            polygonIoParseWssFrame(wssData);

        free(wssData->frame);
        wssData->frame = NULL;
        wssData->size = 0;
    }

    return realsize;
}

int polygonIoParseWssFrame(WssData *wssData)
{
    int status = 0;
    json_error_t error = {0};
    if (wssData->frame == NULL)
        return ON_PIO_WSS_NO_JSON_ROOT;
    json_t *root = json_loads(wssData->frame, 0, &error);
    if (!root)
    {
        status = ON_PIO_WSS_NO_JSON_ROOT;
        goto cleanup;
    }
    if (!json_is_array(root))
    {
        status = ON_PIO_WSS_JSON_ROOT_NOT_ARRAY;
        json_decref(root);
        goto cleanup;
    }
    json_t *entry = json_array_get(root, 0);
    if (!json_is_object(entry))
    {
        status = ON_PIO_WSS_JSON_NO_ARRAY_ENTRY;
        json_decref(root);
        goto cleanup;
    }
    char *jsonstatus = (char *)json_string_value(json_object_get(entry, "status"));
    char *msg = (char *)json_string_value(json_object_get(entry, "message"));
    if (jsonstatus != NULL && msg != NULL)
    {
        if (strcmp("connected", jsonstatus) == 0 && strcmp("Connected Successfully", msg) == 0)
        {
            status = ON_OK;
            wprintw(data.screen->streamWindow, "Connected to Polygon.IO Websocket\n");
            wssData->connected = true;
        }
        else if (strcmp("auth_success", jsonstatus) == 0 && strcmp("authenticated", msg) == 0)
        {
            // Authenticated
            wprintw(data.screen->streamWindow, "Authenticated with Polygon.IO Websocket\n");
            status = ON_OK;
            wssData->authenticated = true;
        }
    }
    else
    {
        if (wssData->screen != NULL)
        {
            char *event = NULL;
            char *sym = NULL;
            double volume = 0;
            double totalVolume = 0;
            double dayOpen = 0;
            double open = 0;
            double high = 0;
            double low = 0;
            double close = 0;
            for (int e = 0; e < json_array_size(root); e++)
            {
                entry = json_array_get(root, e);
                event = (char *)json_string_value(json_object_get(entry, "ev"));
                if (event != NULL)
                {
                    if (strcmp("A", event) == 0 || strcmp("AM", event) == 0)
                    {
                        sym = (char *)json_string_value(json_object_get(entry, "sym"));
                        volume = json_number_value(json_object_get(entry, "v"));
                        totalVolume = json_number_value(json_object_get(entry, "av"));
                        dayOpen = json_number_value(json_object_get(entry, "op"));
                        open = json_number_value(json_object_get(entry, "o"));
                        high = json_number_value(json_object_get(entry, "h"));
                        low = json_number_value(json_object_get(entry, "l"));
                        close = json_number_value(json_object_get(entry, "c"));
                        wprintw(wssData->screen->streamWindow, "%s: $%.2lf %.0lf / %.0lf\n", sym, close, volume, totalVolume);
                        wclrtoeol(wssData->screen->streamWindow);
                        wrefresh(wssData->screen->streamWindow);
                    }
                }
            }
        }
        status = ON_OK;
    }

cleanup:
    json_decref(root);

    return status;
}

int polygonIoStreamConnect(ScreenState *screen, char *socketName, char *timing)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (!wssSupportEnabled)
        return ON_PIO_WSS_NOT_ENABLED;

    char url[WSS_URL_BUFFER_SIZE] = {0};

    CURLcode mStatus;

    long httpCode = 0;
    int status = 0;

    if (data.curl == NULL)
        data.curl = curl_easy_init();
    if (data.screen == NULL)
        data.screen = screen;

    if (data.curl && !data.authenticated)
    {
        // timing is either "socket" or "delayed"
        sprintf(url, "wss://%s.polygon.io/%s", timing, socketName);
        print(screen, screen->mainWindow, "url: %s\n", url);
        // print(mainWindow, "url:\n%s\n", url);
        curl_easy_setopt(data.curl, CURLOPT_URL, url);
        bzero(url, strlen(url));
        curl_easy_setopt(data.curl, CURLOPT_WRITEFUNCTION, polygonIoWssCallback);
        curl_easy_setopt(data.curl, CURLOPT_WRITEDATA, &data);

        multi_handle = curl_multi_init();
        curl_multi_add_handle(multi_handle, data.curl);
        int count = 0;
        bool authRequested = false;
        int num_fds = 0;
        do
        {
            mStatus = curl_multi_perform(multi_handle, &pio_wss_transfers_running);
            if (mStatus == CURLM_OK)
                mStatus = curl_multi_wait(multi_handle, NULL, 0, 10, &num_fds);

            if (data.connected && !data.authenticated && !authRequested)
            {
                polygonIoStreamAuthenticate(&data);
                authRequested = true;
            }
            count++;
        } while (pio_wss_transfers_running && mStatus == CURLM_OK && !data.authenticated && count < 300);

        // Start alarm
        struct itimerval interval = {0};
        interval.it_interval.tv_sec = 0;
        interval.it_interval.tv_usec = 100000;
        interval.it_value.tv_sec = 0;
        interval.it_value.tv_usec = 100000;
        setitimer(ITIMER_REAL, &interval, NULL);

        if (!data.connected)
        {
            print(screen, screen->mainWindow, "Unable to connect to Polygon.IO websocket %d %d %d\n", count, data.authenticated, pio_wss_transfers_running);
            return ON_PIO_WSS_NOT_CONNECTED;
        }
        else if (!data.authenticated)
        {
            print(screen, screen->mainWindow, "Unable to authenticate to Polygon.IO websocket\n");
            return ON_PIO_WSS_NOT_AUTHENTICATED;
        }

    }
cleanup:

    return ON_OK;
}

int polygonIoStreamAuthenticate(WssData *wssData)
{
    if (!wssSupportEnabled || wssData == NULL || !wssData->connected)
        return ON_PIO_WSS_NO_DATA;

    char authenticate[512] = {0};
    char *token = loadApiToken("PIO.apitoken");
    if (token == NULL)
    {
        char *tmptoken = getpass("  Polygon.IO (PIO) personal API token: ");
        saveApiToken("PIO.apitoken", tmptoken);
        token = strdup(tmptoken);
        bzero(tmptoken, strlen(tmptoken));
    }
    sprintf(authenticate, "{\"action\":\"auth\",\"params\":\"%s\"}", token);
    bzero(token, strlen(token));
    free(token);

    size_t responseLength = strlen(authenticate) + 1;
    size_t sent = 0;
    CURLcode res = curl_ws_send(wssData->curl, authenticate, responseLength, &sent, 0, CURLWS_TEXT);
    bzero(authenticate, sizeof(authenticate));

    return ON_OK;

}


int polygonIoStreamSubscribe(ScreenState *screen, char *channel)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (channel == NULL)
        return ON_PIO_WSS_NO_CHANNEL;

    int status = ON_OK;

    if (!data.authenticated)
    {
        if (strncmp("O:", channel, 2) == 0)
            status = polygonIoStreamConnect(screen, "options", "socket");
        else if (strncmp("X:", channel, 2) == 0)
            status = polygonIoStreamConnect(screen, "crypto", "delayed");
        else if (strncmp("C:", channel, 2) == 0)
            status = polygonIoStreamConnect(screen, "forex", "delayed");
        else
            status = polygonIoStreamConnect(screen, "stocks", "delayed");
    }

    // Subscribe to a channel
    char subscribe[1024] = {0};
    sprintf(subscribe, "{\"action\":\"subscribe\",\"params\":\"A.%s\"}", channel);
    size_t responseLength = strlen(subscribe) + 1;
    size_t sent = 0;
    CURLcode res = curl_ws_send(data.curl, subscribe, responseLength, &sent, 0, CURLWS_TEXT);

    return ON_OK;
}

int polygonIoStreamUnsubscribe(WssData *wssData, char *channel)
{
    if (!wssData->authenticated)
        return ON_PIO_WSS_NOT_AUTHENTICATED;
    if (wssData == NULL)
        return ON_PIO_WSS_NO_DATA;
    if (channel == NULL)
        return ON_PIO_WSS_NO_CHANNEL;

    // Subscribe to a channel
    char unsubscribe[1024] = {0};
    sprintf(unsubscribe, "{\"action\":\"unsubscribe\",\"params\":\"A.%s\"}", channel);
    size_t responseLength = strlen(unsubscribe) + 1;
    size_t sent = 0;
    CURLcode res = curl_ws_send(wssData->curl, unsubscribe, responseLength, &sent, 0, CURLWS_TEXT);

    int status = ON_OK;
    if (res != CURLE_OK)
        status = ON_PIO_WSS_LIBCURL_ERROR;
    return status;
}
void updateWssStreamContent(void)
{
    CURLMcode mStatus = CURLM_OK;

    int num_fds = 0;

    if (pio_wss_transfers_running)
    {
        mStatus = curl_multi_perform(multi_handle, &pio_wss_transfers_running);
        if (mStatus == CURLM_OK)
            mStatus = curl_multi_wait(multi_handle, NULL, 0, 1, &num_fds);

        if (pio_wss_transfers_running && (!running))
        {
            // Close the WSS connection
            size_t sent = 0;
            CURLcode res = curl_ws_send(data.curl, "{\"status\":\"done\", \"message\":\"Bye\"}", 4, &sent, 0, CURLWS_TEXT | CURLWS_CLOSE);
            curl_multi_remove_handle(multi_handle, data.curl);
        }
    }

    return;
}

void wssCleanup(void)
{
    curl_easy_cleanup(data.curl);
    free(data.frame);
    data.frame = NULL;
    data.size = 0;
    return;
}
