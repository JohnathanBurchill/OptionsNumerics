/*
    Options Numerics: on_dataproviders.h

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

#ifndef _ON_DATAPROVIDERS_H
#define _ON_DATAPROVIDERS_H

#define URL_BUFFER_SIZE 8192
#define AUTH_HEADER_BUFFER_SIZE 512
#define JSON_DATA_BUFFER_SIZE 8192

#include "on_parse.h"
#include "on_data.h"

#include <stdbool.h>
#include <time.h>

#include <jansson.h>

typedef struct {
    size_t nPrices;
    time_t *times;
    double *opens;
    double *highs;
    double *lows;
    double *closes;
} PriceData;

int updateQuestradeAccessToken(void);
double questradeStockQuote(char *symbol);

int fredSOFR(double *sofr);

json_t *polygonIoRESTRequest(const char *requestUrl);

int polygonIoOptionsSearch(char *ticker, char type, double minstrike, double maxstrike, Date date1, Date date2, bool expired, char **nextPagePtr);
int polygonIoOptionsChain(char *ticker, char type, double minstrike, double maxstrike, Date date1, Date date2, double minpremium, char **nextPagePtr);
int polygonIoPriceHistory(char *symbol, Date startDate, Date stopDate, PriceData *priceData);
int polygonIoVolatility(char *symbol, Date startDate, Date stopDate);
int polygonIoLatestPrice(char *ticker, TickerData *tickerData, OptionsData *optionsData, bool verbose);
int polygonIoPreviousClose(char *ticker);

#endif // _ON_DATAPROVIDERS_H

