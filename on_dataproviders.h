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

enum fredStatus
{
    ON_FRED_OK = 0,
    ON_FRED_NO_DATA = 1
};

enum polygonIoStatus
{
    ON_PIO_OK = 0,
    ON_PIO_INVALID_RESPONSE = -1,
    ON_PIO_NO_STOCK_TICKER = 1 << 0,
    ON_PIO_NO_STOCK_TICKER_NAME = 1 << 1,
    ON_PIO_NO_STOCK_QUOTE = 1 << 2,
    ON_PIO_NO_STOCK_DAY_INFO = 1 << 3,
    ON_PIO_NO_STOCK_MINUTE_DATA = 1 << 4,   
    ON_PIO_NO_STOCK_PREVIOUS_DAY_INFO = 1 << 5,
    ON_PIO_NO_OPTIONS_RESULTS = 1 << 6,
    ON_PIO_NO_OPTIONS_QUOTE = 1 << 7,
    ON_PIO_NO_OPTIONS_DAY_INFO = 1 << 8,
    ON_PIO_NO_OPTIONS_GREEKS = 1 << 9,
    ON_PIO_NO_OPTIONS_DETAILS = 1 << 10
};

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

enum polygonMarket
{
    STOCKS,
    OPTIONS,
    FOREX,
    CRYPTO
};

json_t *polygonIoRESTRequest(const char *requestUrl);

int polygonIoOptionsSearch(char *ticker, char type, double minstrike, double maxstrike, Date date1, Date date2, bool expired, char **nextPagePtr);
int polygonIoOptionsChain(char *ticker, char type, double minstrike, double maxstrike, Date date1, Date date2, double minpremium, char **nextPagePtr);
int polygonIoPriceHistory(char *symbol, Date startDate, Date stopDate, PriceData *priceData);
int polygonIoVolatility(char *symbol, Date startDate, Date stopDate, double *volatility);
int polygonIoLatestPrice(char *ticker, TickerData *tickerData, OptionsData *optionsData, bool verbose);
int printLatestPriceStocks(json_t *root);
int printLatestPriceOptions(json_t *root);

int polygonIoPreviousClose(char *ticker);



#endif // _ON_DATAPROVIDERS_H

