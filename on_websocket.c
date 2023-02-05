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

    if (data == NULL || userdata == NULL)
        return ON_OK;

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
    if (strcmp("connected", jsonstatus) == 0 && strcmp("Connected Successfully", msg) == 0)
    {
        status = ON_OK;
        wssData->connected = true;
    }
    else if (strcmp("auth_success", jsonstatus) == 0 && strcmp("authenticated", msg) == 0)
    {
        // Authenticated
        status = ON_OK;
        wssData->authenticated = true;
    }
    else
    {
        status = ON_PIO_WSS_UNHANDLED_TEXT_FRAME;
    }

cleanup:
    json_decref(root);

    return status;
}

int polygonIoStreamConnect(char *socketName)
{
    if (!wssSupportEnabled)
        return 1;

    char url[WSS_URL_BUFFER_SIZE] = {0};

    CURLcode res;

    long httpCode = 0;
    int status = 0;

    WssData data = {0};
    data.curl = curl_easy_init();


    if (data.curl)
    {
        // Realtime
        // sprintf(url, "wss://socket.polygon.io/%s", socketName);
        // Delayed
        sprintf(url, "wss://delayed.polygon.io/%s", socketName);
        // print(mainWindow, "url: %s\n", url);
        // print(mainWindow, "url:\n%s\n", url);
        curl_easy_setopt(data.curl, CURLOPT_URL, url);
        bzero(url, strlen(url));
        curl_easy_setopt(data.curl, CURLOPT_WRITEFUNCTION, polygonIoWssCallback);
        curl_easy_setopt(data.curl, CURLOPT_WRITEDATA, &data);

        CURLM *multi_handle = curl_multi_init();
        curl_multi_add_handle(multi_handle, data.curl);

        // Attempt to connect and go into an interactive loop
        int transfers_running = 0;
        CURLMcode mStatus = 0;
        int key = 0;
        do {
            mStatus = curl_multi_wait(multi_handle, NULL, 0, 50, &transfers_running);
            if (mStatus)
                break;
            mStatus = curl_multi_perform(multi_handle, &transfers_running);
            if (mStatus)
                break;

            // Check for keyboard interrupt
            key = 0;
            timeout(0);
            refresh();
            key = getch();
            // if (key != 0)
            //     mvwprintw(statusWindow, 0, COLS - 10, "key: %d", key);
            if (transfers_running && (!running || key == KEY_BACKSPACE))
            {
                // Close the WSS connection
                size_t sent = 0;
                CURLcode res = curl_ws_send(data.curl, "{\"status\":\"done\", \"message\":\"Bye\"}", 4, &sent, 0, CURLWS_TEXT | CURLWS_CLOSE);
                break;

                if (data.connected)
                {
                    if (!data.authenticated)
                        polygonIoStreamAuthenticate(&data);
                    else if (key == 's')
                        polygonIoStreamSubscribe(&data, "A.O:GME230317C00016000");
                    else if (key == 'u')
                        polygonIoStreamUnsubscribe(&data, "A.O:GME230317C00016000");
                }
            }
            key = 0;
        } while (transfers_running);
        curl_multi_remove_handle(multi_handle, data.curl);

        // wprintw(statusWindow, "PolyhonIO WSS connection complete.");
        /* Check for errors */

cleanup:
        curl_easy_cleanup(data.curl);
        free(data.frame);
        data.frame = NULL;
        data.size = 0;
    }

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
    CURLcode res = curl_ws_send(wssData, authenticate, responseLength, &sent, 0, CURLWS_TEXT);
    bzero(authenticate, sizeof(authenticate));

    return ON_OK;

}

int polygonIoStreamSubscribe(WssData *wssData, char *channel)
{
    if (!wssData->authenticated)
        return ON_PIO_WSS_NOT_AUTHENTICATED;
    if (wssData == NULL)
        return ON_PIO_WSS_NO_DATA;
    if (channel == NULL)
        return ON_PIO_WSS_NO_CHANNEL;

    // Subscribe to a channel
    char subscribe[1024] = {0};
    sprintf(subscribe, "{\"action\":\"subscribe\",\"params\":\"AM.%s\"}", channel);
    size_t responseLength = strlen(subscribe) + 1;
    size_t sent = 0;
    CURLcode res = curl_ws_send(wssData->curl, subscribe, responseLength, &sent, 0, CURLWS_TEXT);

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
    sprintf(unsubscribe, "{\"action\":\"unsubscribe\",\"params\":\"AM.%s\"}", channel);
    size_t responseLength = strlen(unsubscribe) + 1;
    size_t sent = 0;
    CURLcode res = curl_ws_send(wssData->curl, unsubscribe, responseLength, &sent, 0, CURLWS_TEXT);

    int status = ON_OK;
    if (res != CURLE_OK)
        status = ON_PIO_WSS_LIBCURL_ERROR;
    return status;
}
