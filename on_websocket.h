/*
    Options Numerics: on_websocket.h

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

#ifndef _ON_WEBSOCKET_H
#define _ON_WEBSOCKET_H

#include "on_state.h"
#include "on_remote.h"

#include <stdlib.h>

#include <curl/curl.h>

#define WSS_URL_BUFFER_SIZE 8192

int checkWebSocketSupport(ScreenState *screen);

static size_t polygonIoWssCallback(char *data, size_t size, size_t nmemb, void *userdata);

int polygonIoParseWssFrame(WssData *wssData);

int polygonIoStreamConnect(ScreenState *screen, char *socketName, char *timing);
int polygonIoStreamAuthenticate(WssData *wssData);

int polygonIoStreamSubscribe(ScreenState *screen, char *channel);
int polygonIoStreamUnsubscribe(ScreenState *screen, char *channel);

void updateWssStreamContent(void);
void clearWssStreamStatusLine(void);
void wssCleanup(void);

int saveWssStreamList(void);
int restoreWssStreamList(ScreenState *screen);

#endif // _ON_WEBSOCKET_H
