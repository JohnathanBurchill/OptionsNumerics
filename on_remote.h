/*
    Options Numerics: on_remote.h

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

#ifndef _ON_REMOTE_H
#define _ON_REMOTE_H

#include "on_state.h"
#include "on_dataproviders.h"

#include <stdbool.h>
#include <sys/types.h>

#include <curl/curl.h>

typedef struct curlData
{
    char *response;
    size_t size;
} CurlData;

typedef struct wssData
{
    char *frame;
    size_t size;
    CURL *curl;
    bool connected;
    bool authenticated;
    PioSubscription *subscriptions;
    int nSubscriptions;
    ScreenState *screen;
} WssData;


#endif // _ON_REMOTE_H
