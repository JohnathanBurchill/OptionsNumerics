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
#include "on_dataproviders.h"

#include "on_api.h"
#include "on_remote.h"

#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
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

static long streamWindowStatusCount = 0;


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
        streamWindowStatusCount = 0;
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
    if (wssData == NULL)
        return ON_MISSING_ARG_POINTER;

    if (wssData->screen == NULL)
        return ON_NO_SCREEN;

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
    // TODO handle more than one entry - could have multiple messages
    json_t *entry = json_array_get(root, 0);
    if (!json_is_object(entry))
    {
        status = ON_PIO_WSS_JSON_NO_ARRAY_ENTRY;
        json_decref(root);
        goto cleanup;
    }
    char *jsonstatus = (char *)json_string_value(json_object_get(entry, "status"));
    char *msg = (char *)json_string_value(json_object_get(entry, "message"));
    // if (jsonstatus != NULL)
    //     print(wssData->screen, wssData->screen->streamWindow, "Polygon.IO WSS status: %s\n", jsonstatus);
    if (msg != NULL)
    {
        mvwprintw(wssData->screen->streamWindow, wssData->screen->streamWindowHeight - 1, 0, "%s", msg);
        wclrtoeol(wssData->screen->streamWindow);
        streamWindowStatusCount = 0;
    }
    if (jsonstatus != NULL && msg != NULL)
    {
        if (strcmp("connected", jsonstatus) == 0 && strcmp("Connected Successfully", msg) == 0)
        {
            status = ON_OK;
            mvwprintw(wssData->screen->streamWindow, wssData->screen->streamWindowHeight - 1, 0, "Connected to Polygon.IO Websocket");
            streamWindowStatusCount = 0;
            // wprintw(wssData->screen->streamWindow, "Connected to Polygon.IO Websocket\n");
            wssData->connected = true;
        }
        else if (strcmp("auth_success", jsonstatus) == 0 && strcmp("authenticated", msg) == 0)
        {
            // Authenticated
            mvwprintw(wssData->screen->streamWindow, wssData->screen->streamWindowHeight - 1, 0, "Authenticated with Polygon.IO Websocket");
            // wprintw(wssData->screen->streamWindow, "Authenticated with Polygon.IO Websocket\n");
            streamWindowStatusCount = 0;
            status = ON_OK;
            wssData->authenticated = true;
        }
        else if (strlen(msg) > 15 && strncmp("subscribed to: ", msg, 15) == 0)
        {
            mvwprintw(wssData->screen->streamWindow, wssData->screen->streamWindowHeight - 1, 0, "%s", msg);
            streamWindowStatusCount = 0;
            wclrtoeol(wssData->screen->streamWindow);
            int subscribeCount = 0;
            for (; subscribeCount < wssData->nSubscriptions; subscribeCount++)
            {
                if (strcmp(wssData->subscriptions[subscribeCount].channel, msg + 15) == 0)
                    break;
            }
            if (subscribeCount == wssData->nSubscriptions)
            {
                // Add a new subscription
                void *mem = realloc(wssData->subscriptions, sizeof *wssData->subscriptions * (wssData->nSubscriptions + 1));
                if (mem == NULL)
                {
                    status = ON_HEAP_MEMORY_ERROR;
                    goto cleanup;
                }
                wssData->subscriptions = mem;
                wssData->nSubscriptions++;
                bzero(&wssData->subscriptions[wssData->nSubscriptions-1], sizeof *wssData->subscriptions);
                if (wssData->screen->streamWindowHeight < LINES / 2)
                {
                    wssData->screen->streamWindowHeight++;
                    data.screen->mainWindowViewHeight--;
                    mvwin(wssData->screen->statusWindow, wssData->screen->streamWindowHeight, 0);
                    mvwin(wssData->screen->mainWindow, wssData->screen->streamWindowHeight + wssData->screen->statusHeight, 0);
                    resetPromptPosition(wssData->screen, false);
                }
                snprintf(wssData->subscriptions[subscribeCount].channel, PIO_CHANNEL_LENGTH, "%s", msg + 15);
                // TODO ? Pause timer
                char *p = msg;
                while (p && *p != '.')
                    p++;
                if (p + 1)
                {
                    polygonIoPreviousClose(NULL, p+1, &wssData->subscriptions[subscribeCount].previousClose, NULL);
                }
            }
        }
    }
    else
    {
        if (wssData->screen != NULL)
        {
            char *event = NULL;
            char *sym = NULL;
            PioSubscription *s = NULL;

            for (int e = 0; e < json_array_size(root); e++)
            {
                entry = json_array_get(root, e);
                event = (char *)json_string_value(json_object_get(entry, "ev"));
                if (event != NULL)
                {
                    if (strcmp("A", event) == 0 || strcmp("AM", event) == 0)
                    {
                        sym = (char *)json_string_value(json_object_get(entry, "sym"));

                        for (int sInd = 0; sInd < wssData->nSubscriptions; sInd++)
                        {
                            s = &wssData->subscriptions[sInd];
                            if (strncmp(event, s->channel, strlen(event)) == 0 && strcmp(sym, s->channel + strlen(event) + 1) == 0)
                            {
                                s->aggVolume = json_number_value(json_object_get(entry, "v"));
                                s->dayVolume = json_number_value(json_object_get(entry, "av"));
                                s->dayOpen = json_number_value(json_object_get(entry, "op"));
                                s->aggOpen = json_number_value(json_object_get(entry, "o"));
                                s->aggHigh = json_number_value(json_object_get(entry, "h"));
                                s->aggLow = json_number_value(json_object_get(entry, "l"));
                                s->aggClose = json_number_value(json_object_get(entry, "c"));
                                s->reportedTimeSecs = json_number_value(json_object_get(entry, "e")) / 1000.0;
                                s->aggChange = s->aggClose - s->aggOpen;
                                s->dayChange = s->aggClose - s->previousClose;
                                s->dayPercentChange = s->dayChange / s->previousClose * 100.0;
                                break;
                            }
                        }
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
        // print(screen, screen->mainWindow, "url: %s\n", url);
        // print(mainWindow, "url:\n%s\n", url);
        curl_easy_setopt(data.curl, CURLOPT_URL, url);
        bzero(url, strlen(url));
        curl_easy_setopt(data.curl, CURLOPT_WRITEFUNCTION, polygonIoWssCallback);
        curl_easy_setopt(data.curl, CURLOPT_WRITEDATA, &data);

        multi_handle = curl_multi_init();
        curl_multi_add_handle(multi_handle, data.curl);
        int count = 0;
        bool authRequested = false;
        // From curl_multi_wait example
        int repeats = 0;
        int num_fds = 0;
        do
        {
            mStatus = curl_multi_perform(multi_handle, &pio_wss_transfers_running);
            if (mStatus == CURLM_OK)
                mStatus = curl_multi_wait(multi_handle, NULL, 0, 10, &num_fds);

            if (mStatus != CURLM_OK)
            {
                print(screen, screen->mainWindow, "Failed to connect to Polygon.IO websocket %d %d %d\n", count, data.authenticated, pio_wss_transfers_running);
                curl_multi_remove_handle(multi_handle, data.curl);
                return ON_PIO_WSS_NOT_CONNECTED;
            }
            if (num_fds == 0)
            {
                repeats++;
                if (repeats > 1)
                    usleep(1000);
            }
            else
                repeats = 0;

            count++;
        } while (pio_wss_transfers_running && !data.connected && count < 300);

        if (!data.connected)
        {
            print(screen, screen->mainWindow, "Unable to connect to Polygon.IO websocket %d %d %d\n", count, data.authenticated, pio_wss_transfers_running);
            return ON_PIO_WSS_NOT_CONNECTED;
        }
        status = polygonIoStreamAuthenticate(&data);

        // Start alarm
        struct itimerval interval = {0};
        interval.it_interval.tv_sec = 0;
        interval.it_interval.tv_usec = 100000;
        interval.it_value.tv_sec = 0;
        interval.it_value.tv_usec = 100000;
        setitimer(ITIMER_REAL, &interval, NULL);


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
    char source[8] = {0};
 
    if (!data.authenticated)
    {
        if (strncmp("O:", channel, 2) == 0)
            snprintf(source, 8, "%s", "options");
        else if (strncmp("X:", channel, 2) == 0)
            snprintf(source, 8, "%s", "crypto");
        else if (strncmp("C:", channel, 2) == 0)
            snprintf(source, 8, "%s", "forex");
        else
            snprintf(source, 8, "%s", "stocks");

        status = polygonIoStreamConnect(screen, source, "delayed");
        if (status != ON_OK)
            status = polygonIoStreamConnect(screen, source, "socket");

    }

    // print(screen, screen->mainWindow, "connection status: %d\n", status);

    // Subscribe to a channel
    char subscribe[1024] = {0};
    sprintf(subscribe, "{\"action\":\"subscribe\",\"params\":\"%s\"}", channel);
    size_t responseLength = strlen(subscribe) + 1;
    size_t sent = 0;
    CURLcode res = curl_ws_send(data.curl, subscribe, responseLength, &sent, 0, CURLWS_TEXT);

    return ON_OK;
}

int polygonIoStreamUnsubscribe(ScreenState *screen, char *channel)
{
    if (!data.authenticated)
        return ON_PIO_WSS_NOT_AUTHENTICATED;
    if (channel == NULL)
        return ON_PIO_WSS_NO_CHANNEL;

    int status = ON_OK;

    bool removeAll = strcasecmp("all", channel) == 0;

    // Unsubscribe from a channel
    char unsubscribe[1024] = {0};
    size_t responseLength = 0;
    size_t sent = 0;
    CURLcode res = 0;

    char *channels = strdup(channel);
    if (channels == NULL)
        return ON_HEAP_MEMORY_ERROR;
    
    // based on strsep manual page
    char *token = NULL;
    int nRemoved = 0;
    while ((token = strsep(&channels, ",")) != NULL)
    {
        for (int i = data.nSubscriptions - 1; i >= 0; i--)
        {
            if (strcmp(token, data.subscriptions[i].channel) == 0 || removeAll)
            {
                nRemoved++;
                sprintf(unsubscribe, "{\"action\":\"unsubscribe\",\"params\":\"%s\"}", data.subscriptions[i].channel);
                responseLength = strlen(unsubscribe) + 1;
                sent = 0;
                res = curl_ws_send(data.curl, unsubscribe, responseLength, &sent, 0, CURLWS_TEXT);
                if (i < data.nSubscriptions - 1)
                {
                    memmove(&data.subscriptions[i], &data.subscriptions[i+1], (sizeof *data.subscriptions) * (data.nSubscriptions - i - 1));
                }
                data.nSubscriptions--;
            }
        }
        if (data.nSubscriptions > 0)
        {
            void *mem = realloc(data.subscriptions, (sizeof *data.subscriptions) * data.nSubscriptions);
            if (mem == NULL)
            {
                status = ON_HEAP_MEMORY_ERROR;
                goto cleanup;

            }
            data.subscriptions = mem;
        }
        else
        {
            free(data.subscriptions);
            data.subscriptions = NULL;
        }
        if (data.screen->streamWindowHeight > 2)
        {
            data.screen->streamWindowHeight -= nRemoved;
            data.screen->mainWindowViewHeight += nRemoved;
            mvwin(data.screen->statusWindow, data.screen->streamWindowHeight, 0);
            mvwin(data.screen->mainWindow, data.screen->streamWindowHeight + data.screen->statusHeight, 0);
            wmove(data.screen->streamWindow, data.nSubscriptions, 0);
            wclrtobot(data.screen->streamWindow);
            resetPromptPosition(data.screen, false);
        }
        
    }

cleanup:

    if (nRemoved > 0)
        werase(screen->streamWindow);

    free(channels);

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

    struct timeval tv = {0};
    double dt = 0;
    PioSubscription *s = NULL;
    int longestLine = 0;
    int y = 0, x = 0;
    for (int i = 0; i < data.nSubscriptions; i++)
    {
        s = &data.subscriptions[i];
        if (s->reportedTimeSecs > 0)
        {
            mvwprintw(data.screen->streamWindow, i, 0, "%25s: $%.2lf (%+.2lf, %+.2lf%%) %.0lf (%+.0lf) ", s->channel, s->aggClose, s->dayChange, s->dayPercentChange, s->dayVolume, s->aggVolume);
            getyx(data.screen->streamWindow, y, x);
            if (x > longestLine)
                longestLine = x;
        }
        else
            mvwprintw(data.screen->streamWindow, i, 0, "%25s: \"...nothing continued to happen.\"", s->channel);
        wclrtoeol(data.screen->streamWindow);

    }
    for (int i = 0; i < data.nSubscriptions; i++)
    {
        s = &data.subscriptions[i];
        gettimeofday(&tv, NULL);
        if (s->reportedTimeSecs > 0)
        {
            dt = tv.tv_sec + tv.tv_usec / 1e6 - s->reportedTimeSecs;
            if (dt > 60)
            {
                dt /= 60.0;
                mvwprintw(data.screen->streamWindow, i, longestLine, "%.1lf min ago", dt);
            }
            else
                mvwprintw(data.screen->streamWindow, i, longestLine, "%.3lf s ago", dt);
        }
        wclrtoeol(data.screen->streamWindow);


    }
    wrefresh(data.screen->streamWindow);

    return;
}

void clearWssStreamStatusLine(void)
{
    static int col = 0;

    if (data.screen && data.screen->streamWindow && streamWindowStatusCount > 49)
    {
        wmove(data.screen->streamWindow, data.screen->streamWindowHeight - 1, col);
        wprintw(data.screen->streamWindow, "%s", "  ");
        wrefresh(data.screen->streamWindow);
        col += 2;
        if (col > 20)
        {
            wclrtoeol(data.screen->streamWindow);
            streamWindowStatusCount = 0;
            col = 0;
        }
    }
    streamWindowStatusCount++;

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

int saveWssStreamList(void)
{
    if (data.screen == NULL)
        return ON_NO_SCREEN;

    int status = ON_OK;

    char streamsFile[FILENAME_MAX] = {0};
    char *home = getenv("HOME");
    if (home != NULL && strlen(home) > 0)
        snprintf(streamsFile, FILENAME_MAX, "%s/%s/%s", home, ON_OPTIONS_DIR, ON_STREAMS_LOG);

    FILE *streamsLog = fopen(streamsFile, "w");
    if (streamsLog != NULL)
    {
        for (int i = 0; i < data.nSubscriptions; i++)
        {
            fprintf(streamsLog, "%s%s", i > 0 ? "," : "", data.subscriptions[i].channel);
        }
        fprintf(streamsLog, "\n");
        fclose(streamsLog);
    }
    else
        status = ON_FILE_WRITE_ERROR;

    return status;
}

int restoreWssStreamList(ScreenState *screen)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (data.screen == NULL)
        data.screen = screen;

    int status = ON_OK;

    char streamsFile[FILENAME_MAX] = {0};
    char *home = getenv("HOME");
    if (home != NULL && strlen(home) > 0)
        snprintf(streamsFile, FILENAME_MAX, "%s/%s/%s", home, ON_OPTIONS_DIR, ON_STREAMS_LOG);

    struct stat st = {0};
    stat(streamsFile, &st);
    if (st.st_size < 3 || st.st_size > 1000)
        return ON_FILE_READ_ERROR;

    char *streams = malloc(sizeof *streams * (st.st_size + 1));

    if (streams == NULL)
        return ON_HEAP_MEMORY_ERROR;

    FILE *streamsLog = fopen(streamsFile, "r");
    if (streamsLog != NULL)
    {
        int r = fread(streams, sizeof *streams, st.st_size, streamsLog);
        if (r != st.st_size)
            status = ON_FILE_READ_ERROR;
        else
            streams[st.st_size] = '\0';

        fclose(streamsLog);
    }
    else
        status = ON_FILE_WRITE_ERROR;

    status = polygonIoStreamSubscribe(data.screen, streams);

    free(streams);

    return status;

}
