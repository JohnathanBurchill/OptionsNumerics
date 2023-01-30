/*
    Options Numerics: on_dataproviders.c

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

#include "on_dataproviders.h"
#include "on_api.h"
#include "on_statistics.h"
#include "on_parse.h"
#include "on_remote.h"
#include "on_data.h"
#include "on_screen_io.h"

#include <string.h>
#include <curl/curl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#include <jansson.h>

#include <ncurses.h>

extern WINDOW *mainWindow;
extern WINDOW *statusWindow;
extern WINDOW *streamWindow;

// Based on ChatGPT response 13 Jan 2023
int updateQuestradeAccessToken(void)
{

    CURL *curl;
    CURLcode res;

    char *accountNumber = loadApiToken("QUESTRADE.accountnumber");
    if (accountNumber == NULL)
    {
        char *tmpAccountNumber = getpass("  Questrade account number: ");
        saveApiToken("QUESTRADE.accountnumber", tmpAccountNumber);
        accountNumber = strdup(tmpAccountNumber);
        bzero(tmpAccountNumber, strlen(tmpAccountNumber));
    }

    char url[URL_BUFFER_SIZE] = {0};
    snprintf(url, URL_BUFFER_SIZE, "https://api.questrade.com/v1/accounts/%s", accountNumber);
    bzero(accountNumber, strlen(accountNumber));
    free(accountNumber);

    char authorizationHeader[AUTH_HEADER_BUFFER_SIZE] = {0};

    char *token = loadApiToken("QUESTRADE.apitoken");
    if (token == NULL)
    {
        char *tmpToken = getpass("  Questrade personal API token: ");
        saveApiToken("QUESTRADE.apitoken", tmpToken);
        token = strdup(tmpToken);
        bzero(tmpToken, strlen(tmpToken));
    }

    snprintf(authorizationHeader, 512, "Authorization: Bearer %s", token);
    bzero(token, strlen(token));
    free(token);

    long httpCode = 0;
    int status = 0;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, authorizationHeader);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept-Language: en-CA");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        bzero(url, strlen(url));

        /* Perform the request */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
        {
            mvwprintw(statusWindow, 0, 0, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            status = 1;
        }
        else
        {
            // Check response
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            mvwprintw(statusWindow, 0, 0, "HTTP response code: %ld\n", httpCode);
            switch (httpCode)
            {
                case 401:
                    print(mainWindow, "Access token is invalid.\n");
                    status = 2;
                    break;
                case 404:
                    print(mainWindow, "Account not recognized.\n");
                    status = 2;
                    break;

            }
        }
        /* always cleanup */
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    bzero(url, URL_BUFFER_SIZE);
    bzero(authorizationHeader, AUTH_HEADER_BUFFER_SIZE);

    return status;
}

double questradeStockQuote(char *symbol)
{
    return 0.0;
}

// From libcurl c example

size_t restCallback(char *data, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size *nmemb;
    CurlData *mem = (CurlData *)userdata;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL)
        return 0; /* out of memory! */

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

int fredSOFR(double *sofr)
{
    CURL *curl;
    CURLcode res;
    char url[URL_BUFFER_SIZE] = {0};

    char authorizationHeader[AUTH_HEADER_BUFFER_SIZE] = {0};

    char *token = loadApiToken("FRED.apitoken");
    if (token == NULL)
    {
        char *tmptoken = getpass("  Federal Reserve Economic Data (FRED) personal API token: ");
        saveApiToken("FRED.apitoken", tmptoken);
        token = strdup(tmptoken);
        bzero(tmptoken, strlen(tmptoken));
    }

    long httpCode = 0;
    int status = 0;

    curl = curl_easy_init();

    CurlData data = {0};

    json_t *root = NULL;
    json_t *entry = NULL;

    const char *date = NULL;
    const char *rateStr = NULL;
    double rate = 0;

    if (curl)
    {
        sprintf(url, "https://api.stlouisfed.org/fred/series/observations?series_id=SOFR&limit=1&sort_order=desc&file_type=json&api_key=%s", token);
        bzero(token, strlen(token));

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, restCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        bzero(url, strlen(url));
        /* Perform the request */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
        {
            mvwprintw(statusWindow, 0, 0, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            status = 1;
            goto cleanup;
        }

        if (data.response != NULL)
        {
            json_error_t error = {0};
            root = json_loads(data.response, 0, &error);
            if (!root)
            {
                print(mainWindow, "Could not parse FRED JSON response: line %d: text: %s\n", error.line, error.text);
                goto cleanup;
            }

            json_t *observations = json_object_get(root, "observations");
            if (!json_is_array(observations))
            {
                goto cleanup;
            }
            for (int i = 0; i < json_array_size(observations); i++)
            {
                entry = json_array_get(observations, i);
                if (!json_is_object(entry))
                {
                    print(mainWindow, "Invalid JSON entry %d\n", i);
                    goto cleanup;
                }
                date = json_string_value(json_object_get(entry, "date"));
                rateStr = json_string_value(json_object_get(entry, "value"));
                if (rateStr != NULL)
                    rate = atof(rateStr);
                else
                    rate = nan("");

                print(mainWindow, "  FRED SOFR on %s: %g%%\n", date, rate);
                if (sofr != NULL)
                    *sofr = rate;                

            }
        }

    cleanup:
        json_decref(root);
        free(data.response);
        curl_easy_cleanup(curl);
    }

    bzero(token, strlen(token));
    free(token);

    return 0;
}

json_t *polygonIoRESTRequest(const char *requestUrl)
{
    json_t *root = NULL;
    CURL *curl;
    CURLcode res;
    char url[URL_BUFFER_SIZE] = {0};

    char *token = loadApiToken("PIO.apitoken");
    if (token == NULL)
    {
        char *tmptoken = getpass("  Polygon.IO (PIO) personal API token: ");
        saveApiToken("PIO.apitoken", tmptoken);
        token = strdup(tmptoken);
        bzero(tmptoken, strlen(tmptoken));
    }

    long httpCode = 0;
    int status = 0;

    curl = curl_easy_init();

    struct curlData data = {0};

    unsigned char join = '?';
    for (int i = 0; i < strlen(requestUrl); i++)
        if (requestUrl[i] == '?')
        {
            join = '&';
            break;
        }

    if (curl)
    {
        sprintf(url, "%s%capiKey=%s", requestUrl, join, token);
        bzero(token, strlen(token));
        // print(mainWindow, "url:\n%s\n", url);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        bzero(url, strlen(url));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, restCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        /* Perform the request */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
        {
            mvwprintw(statusWindow, 0, 0, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            goto cleanup;
        }

        if (data.response != NULL)
        {
            int size = data.size;
            // print(mainWindow, "Polygon.io response:\n%s\n", data.response);
            json_error_t error = {0};
            root = json_loads(data.response, 0, &error);
            if (!root)
            {
                print(mainWindow, "Could not parse Polygon.IO JSON response: line %d: text: %s\n", error.line, error.text);
                goto cleanup;
            }
            else if (!json_is_object(root))
            {
                print(mainWindow, "Invalid JSON response from Polygon.IO.\n");
                json_decref(root);
                goto cleanup;
            }
            char *jsonstatus = (char *)json_string_value(json_object_get(root, "status"));
            if (strcmp("ERROR", jsonstatus) == 0)
            {
                char *msg = (char *)json_string_value(json_object_get(root, "error"));
                print(mainWindow, "Polygon.IO: %s\n", msg);
                json_decref(root);
                goto cleanup;
            }
            else if (strcmp("NOT_AUTHORIZED", jsonstatus) == 0)
            {
                char *msg = (char *)json_string_value(json_object_get(root, "message"));
                print(mainWindow, "Polygon.IO: %s\n", msg);
                json_decref(root);
                goto cleanup;
            }
        }
cleanup:
        curl_easy_cleanup(curl);
        free(data.response);
    }

    bzero(token, strlen(token));
    free(token);

    return root;
}


int polygonIoOptionsSearch(char *ticker, char type, double minstrike, double maxstrike, Date date1, Date date2, bool expired, char **nextPagePtr)
{
    char url[URL_BUFFER_SIZE] = {0};
    bool continuedSearch = false;
    int status = 0;

    if (nextPagePtr != NULL && *nextPagePtr != NULL)
    {
        sprintf(url, "%s", *nextPagePtr);
        free(*nextPagePtr);
        *nextPagePtr = NULL;
        continuedSearch = true;
    }
    else
    {
        // TODO what is as_of good for?
        sprintf(url, "https://api.polygon.io/v3/reference/options/contracts?underlying_ticker=%s&contract_type=%s&expiration_date.gte=%d-%02d-%02d&expiration_date.lte=%d-%02d-%02d&strike_price.gte=%.3lf&strike_price.lte=%.3lf&expired=%s&sort=strike_price&limit=250", ticker, type == 'C' ? "call" : "put", date1.year, date1.month, date1.day, date2.year, date2.month, date2.day, minstrike, maxstrike, expired ? "true" : "false");
    }

    json_t *root = polygonIoRESTRequest(url);
    if (root == NULL)
        return 1;

    json_t *results = json_object_get(root, "results");
    if (results == NULL)
        return 1;

    time_t t = 0;

    char *optionTicker = NULL;
    double strike = 0;
    double prevStrike = 0;

    char *expiry = NULL;
    json_t *entry;
    if (!json_is_array(results) || json_array_size(results) == 0)
    {
        print(mainWindow, "No data from Polygon.IO.\n");
        json_decref(root);
        return 2;
    }
    if (!continuedSearch)
        print(mainWindow, "%15s   %8s   %s\n", "Expiry date", "Strike", "Ticker");
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return 2;
        }
        optionTicker = (char *)json_string_value(json_object_get(entry, "ticker"));
        strike = json_number_value(json_object_get(entry, "strike_price"));
        expiry = (char *)json_string_value(json_object_get(entry, "expiration_date"));
        if (strike != prevStrike)
            print(mainWindow, "\n");
        print(mainWindow, "%8.2lf %15s   %s\n", strike, expiry, optionTicker);
        prevStrike = strike;
        
    }
    char *nextUrl = (char *)json_string_value(json_object_get(root, "next_url"));
    if (nextUrl != NULL && strlen(nextUrl) > 0 && nextPagePtr != NULL)
        *nextPagePtr = strdup(nextUrl);
    else
    {
        if (nextPagePtr != NULL)
        {
            free(*nextPagePtr);
            *nextPagePtr = NULL;
        }
    }
    json_decref(root);

    return 0;
}

int polygonIoOptionsChain(char *ticker, char type, double minstrike, double maxstrike, Date date1, Date date2, double minpremium, char **nextPagePtr)
{
    char url[URL_BUFFER_SIZE] = {0};

    bool continuedSearch = false;

    char *optionTicker = NULL;
    double strike = 0;
    char *expiry = NULL;
    double bid = 0;
    double ask = 0;
    double close = 0;
    double openInterest = 0;
    double prevStrike = 0;

    if (nextPagePtr != NULL && *nextPagePtr != NULL)
    {
        sprintf(url, "%s", *nextPagePtr);
        free(*nextPagePtr);
        *nextPagePtr = NULL;
        continuedSearch = true;
    }
    else
        sprintf(url, "https://api.polygon.io/v3/snapshot/options/%s?contract_type=%s&expiration_date.gte=%d-%02d-%02d&expiration_date.lte=%d-%02d-%02d&strike_price.gte=%.3lf&strike_price.lte=%.3lf&sort=strike_price&order=asc&limit=250", ticker, type == 'C' ? "call" : "put", date1.year, date1.month, date1.day, date2.year, date2.month, date2.day, minstrike, maxstrike);

    json_t *root = polygonIoRESTRequest(url);
    if (root == NULL)
        return 1;

    json_t *results = json_object_get(root, "results");
    if (results == NULL)
        return 1;

    time_t t = 0;

    json_t *entry;
    if (!json_is_array(results) || json_array_size(results) == 0)
    {
        print(mainWindow, "No data from Polygon.IO.\n");
        json_decref(root);
        return 2;
    }
    if (!continuedSearch)
        print(mainWindow, "%10s %8s %s %s %s %s\n", "Strike", "Expiry", "Bid", "Ask", "Last", "Ticker");

    json_t *details = NULL;
    json_t *quote = NULL;
    json_t *dayInfo = NULL;
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return 2;
        }
        details = json_object_get(entry, "details");
        optionTicker = (char *)json_string_value(json_object_get(details, "ticker"));
        strike = json_number_value(json_object_get(details, "strike_price"));
        expiry = (char *)json_string_value(json_object_get(details, "expiration_date"));
        openInterest = json_number_value(json_object_get(entry, "open_interest"));
        quote = json_object_get(entry, "last_quote");
        bid = json_number_value(json_object_get(quote, "bid"));
        ask = json_number_value(json_object_get(quote, "ask"));
        dayInfo = json_object_get(entry, "day");
        close = json_number_value(json_object_get(dayInfo, "close"));
        if (bid >= minpremium || ask >= minpremium || close >= minpremium)
        {
            if (strike != prevStrike)
                print(mainWindow, "\n");
            print(mainWindow, "%8.2lf %15s %.3lf %.3lf %.3lf  %s\n", strike, expiry, bid, ask, close, optionTicker);
            prevStrike = strike;
        }
    }
    char *nextUrl = (char *)json_string_value(json_object_get(root, "next_url"));
    if (nextUrl != NULL && strlen(nextUrl) > 0 && nextPagePtr != NULL)
        *nextPagePtr = strdup(nextUrl);
    else
    {
        if (nextPagePtr != NULL)
        {
            free(*nextPagePtr);
            *nextPagePtr = NULL;
        }
    }

    json_decref(root);

    return 0;
}

int polygonIoLatestPrice(char *ticker, TickerData *tickerData, OptionsData *optionsData, bool verbose)
{
    char url[URL_BUFFER_SIZE] = {0};
    if (strncmp("O:", ticker, 2) == 0)
    {
        char *underlying = strdup(ticker + 2);
        char *p = underlying;
        while (p && !isdigit(*p))
            p++;
        if (p != underlying + strlen(underlying))
            *p = '\0';
        sprintf(url, "https://api.polygon.io/v3/snapshot/options/%s/%s", underlying, ticker);
        free(underlying);
    }
    else if (strncmp("C:", ticker, 2) == 0)
        sprintf(url, "https://api.polygon.io/v2/snapshot/locale/global/markets/forex/tickers/%s", ticker);
    else if (strncmp("X:", ticker, 2) == 0)
        sprintf(url, "https://api.polygon.io/v2/snapshot/locale/global/markets/crypto/tickers/%s", ticker);
    else
        sprintf(url, "https://api.polygon.io/v2/snapshot/locale/us/markets/stocks/tickers/%s", ticker);

    json_t *root = polygonIoRESTRequest(url);
    if (root == NULL)
        return 1;

    json_t *results = json_object_get(root, "results");
    if (results == NULL)
        return 1;

    double tday = 0;
    time_t tday_secs = 0;
    struct tm *tday_data = NULL;
    double tlastquote = 0;
    time_t tlastquote_secs = 0;
    struct tm *tlastquote_data = NULL;

    double volume = 0;
    double vwap = 0;
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int nTransactions = 0;
            
    char *type = NULL;
    char *expirationDate = NULL;
    double strike = 0;

    double bid = 0;
    double bidsize = 0;
    double ask = 0;
    double asksize = 0;
    double midpoint = 0;

    char *underlyingTicker = NULL;
    double underlyingPrice = 0;
    double tunderlying = 0;
    time_t tunderlying_secs = 0;
    struct tm *tunderlying_data = NULL;

    double theta = 0;
    double vega = 0;
    double delta = 0;
    double gamma = 0;

    double openInterest = 0;
    double impliedVolatility = 0;

    json_t *quote = json_object_get(results, "last_quote");
    if (!json_is_object(quote))
    {
        print(mainWindow, "Invalid JSON for last_quote\n");
        json_decref(root);
        return 2;
    }

    bid = json_number_value(json_object_get(quote, "bid"));
    bidsize = json_number_value(json_object_get(quote, "bid_size"));
    ask = json_number_value(json_object_get(quote, "ask"));
    asksize = json_number_value(json_object_get(quote, "ask_size"));
    midpoint = json_number_value(json_object_get(quote, "midpoint"));
    tlastquote = json_number_value(json_object_get(quote, "last_updated")) / 1e9;
    tlastquote_secs = floor(tlastquote);

    json_t *detailsItem = json_object_get(results, "details");
    type = (char *)json_string_value(json_object_get(detailsItem, "contract_type"));
    strike = json_number_value(json_object_get(detailsItem, "strike_price"));
    expirationDate = (char *)json_string_value(json_object_get(detailsItem, "expiration_date"));

    json_t *dayItem = json_object_get(results, "day");
    open = json_number_value(json_object_get(dayItem, "open"));
    high = json_number_value(json_object_get(dayItem, "high"));
    low = json_number_value(json_object_get(dayItem, "low"));
    close = json_number_value(json_object_get(dayItem, "close"));
    volume = json_number_value(json_object_get(dayItem, "volume"));
    vwap = json_number_value(json_object_get(dayItem, "vwap"));
    tday = json_number_value(json_object_get(dayItem, "last_updated")) / 1e9;
    tday_secs = floor(tday);

    tlastquote_data = localtime(&tlastquote_secs);
    if (verbose)
    {
        print(mainWindow, "Latest quote at %4d-%02d-%02d %02d:%02d:%02d.%06.0lf\n", tlastquote_data->tm_year+1900, tlastquote_data->tm_mon + 1, tlastquote_data->tm_mday, tlastquote_data->tm_hour, tlastquote_data->tm_min, tlastquote_data->tm_sec, floor(1e6*(tlastquote - (double)tlastquote_secs)));
        print(mainWindow, "  %d bidding $%.3lf --> $%.3lf <-- %d asking $%.3lf\n", (int)bidsize, bid, midpoint,(int)asksize,  ask);
    }
    if (tickerData != NULL && strncasecmp("O:", ticker, 2) != 0)
    {
        tickerData->ticker = strdup(ticker);
        tickerData->lastUpdatedSeconds = tday;
        tickerData->volume = volume;
        tickerData->vwap = vwap;
        tickerData->open = open;
        tickerData->high = high;
        tickerData->low = low;
        tickerData->close = close;
        tickerData->nTransactions = nTransactions;
        tickerData->quote.ticker = strdup(ticker);
        tickerData->quote.lastUpdatedSeconds = tlastquote;
        tickerData->quote.bid = bid;
        tickerData->quote.bidsize = bidsize;
        tickerData->quote.ask = ask;
        tickerData->quote.asksize = asksize;
        tickerData->quote.midpoint = midpoint;
    }
    if (strncasecmp("O:", ticker, 2) == 0)
    {
        json_t *greeks = json_object_get(results, "greeks");
        theta = json_number_value(json_object_get(greeks, "theta"));
        vega = json_number_value(json_object_get(greeks, "vega"));
        delta = json_number_value(json_object_get(greeks, "delta"));
        gamma = json_number_value(json_object_get(greeks, "gamma"));

        openInterest = json_number_value(json_object_get(results, "open_interest"));
        impliedVolatility = json_number_value(json_object_get(results, "implied_volatility"));
        if (verbose)
        {
            print(mainWindow, "  volume: %.0lf, open interest: %.0lf\n", volume, openInterest);
            print(mainWindow, "  vwap: %.2lf, open: %.2lf, high: %.2lf, low: %.2lf, close: %.2lf\n", vwap, open, high, low, close);
            print(mainWindow, "  greeks T: $%.4lf/day V: $%.4lf/%% D: $%.4lf/$ G: $%.4lf/$/$\n", theta, vega, delta, gamma);
            print(mainWindow, "  implied volatility: %.2lf%%\n", impliedVolatility * 100);
        }

        json_t *underlyingItem = json_object_get(results, "underlying_asset");
        underlyingTicker = (char *)json_string_value(json_object_get(underlyingItem, "ticker"));
        underlyingPrice = json_number_value(json_object_get(underlyingItem, "price"));
        tunderlying = json_number_value(json_object_get(underlyingItem, "last_updated")) / 1e9;
        tunderlying_secs = floor(tunderlying);

        tunderlying_data = localtime(&tunderlying_secs);
        if (verbose)
        {
            print(mainWindow, "  %s $%.3lf at %4d-%02d-%02d %02d:%02d:%02d.%06.0lf\n", underlyingTicker, underlyingPrice, tunderlying_data->tm_year+1900, tunderlying_data->tm_mon + 1, tunderlying_data->tm_mday, tunderlying_data->tm_hour, tunderlying_data->tm_min, tunderlying_data->tm_sec, floor(1e6*(tunderlying - (double)tunderlying_secs)));
        }
        if (optionsData != NULL)
        {
            optionsData->ticker = strdup(ticker);
            optionsData->lastUpdatedSeconds = tday;

            optionsData->underlyingTickerData.ticker = strdup(underlyingTicker);
            optionsData->underlyingTickerData.close = underlyingPrice;
            optionsData->underlyingTickerData.lastUpdatedSeconds = tunderlying;

            optionsData->type = strcasecmp("call", type) == 0 ? CALL : PUT;
            optionsData->strike = strike;
            interpretDate(expirationDate, &optionsData->expiry);

            optionsData->tickerData.volume = volume;
            optionsData->tickerData.vwap = vwap;
            optionsData->tickerData.open = open;
            optionsData->tickerData.high = high;
            optionsData->tickerData.low = low;
            optionsData->tickerData.close = close;
            optionsData->tickerData.nTransactions = nTransactions;
            optionsData->tickerData.quote.ticker = strdup(ticker);
            optionsData->quote.lastUpdatedSeconds = tlastquote;
            optionsData->quote.bid = bid;
            optionsData->quote.bidsize = bidsize;
            optionsData->quote.ask = ask;
            optionsData->quote.asksize = asksize;
            optionsData->quote.midpoint = midpoint;

            optionsData->theta = theta;
            optionsData->vega = vega;
            optionsData->delta = delta;
            optionsData->gamma = gamma;

            optionsData->openInterest = openInterest;
            optionsData->impliedVolatility = impliedVolatility;

        }
    }

    json_decref(root);

    return 0;
}

int polygonIoPreviousClose(char *ticker)
{
    char url[URL_BUFFER_SIZE] = {0};
    sprintf(url, "https://api.polygon.io/v2/aggs/ticker/%s/prev?adjusted=true", ticker);

    json_t *root = polygonIoRESTRequest(url);

    if (root == NULL)
        return 1;

    json_t *results = json_object_get(root, "results");
    if (results == NULL)
        return 1;
    int nResults = json_integer_value(json_object_get(root, "resultsCount"));


    time_t t = 0;
    struct tm *timedata = NULL;
    double volume = 0;
    double vwap = 0;
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int nTransactions = 0;
    int year = 0;
    int month = 0;
    int day = 0;

    json_t *entry;
    if (!json_is_array(results) || nResults == 0)
    {
        print(mainWindow, "No data from Polygon.IO.\n");
        json_decref(root);
        return 2;
    }
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return 2;
        }
        t = (time_t) json_integer_value(json_object_get(entry, "t")) / 1000;
        volume = json_number_value(json_object_get(entry, "v"));
        vwap = json_number_value(json_object_get(entry, "vw"));
        open = json_number_value(json_object_get(entry, "o"));
        high = json_number_value(json_object_get(entry, "h"));
        low = json_number_value(json_object_get(entry, "l"));
        close = json_number_value(json_object_get(entry, "c"));
        nTransactions = json_integer_value(json_object_get(entry, "n"));
        timedata = localtime(&t);

        print(mainWindow, "%4d-%02d-%02d vwap: %.2lf, open: %.2lf, high: %.2lf, low: %.2lf, close: %.2lf, vol: %.0lf, trades: %d, shares/trade: %.1lf\n", timedata->tm_year+1900, timedata->tm_mon + 1, timedata->tm_mday, vwap, open, high, low, close, volume, nTransactions, volume / (double)nTransactions);
        
    }

    json_decref(root);

    return 0;
}

int polygonIoPriceHistory(char *symbol, Date startDate, Date stopDate, PriceData *priceData)
{
    char url[URL_BUFFER_SIZE] = {0};
    sprintf(url, "https://api.polygon.io/v2/aggs/ticker/%s/range/1/day/%4d-%02d-%02d/%4d-%02d-%02d?adjusted=true&sort=asc", symbol, startDate.year, startDate.month, startDate.day, stopDate.year, stopDate.month, stopDate.day);

    json_t *root = polygonIoRESTRequest(url);
    if (root == NULL)
        return 1;

    json_t *results = json_object_get(root, "results");
    if (results == NULL)
        return 1;

    int nResults = json_integer_value(json_object_get(root, "resultsCount"));
    time_t t = 0;
    struct tm *timedata = NULL;
    double volume = 0;
    double vwap = 0;
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int nTransactions = 0;
    int year = 0;
    int month = 0;
    int day = 0;

    json_t *entry;
    if (!json_is_array(results) || nResults == 0)
    {
        print(mainWindow, "No data from Polygon.IO.\n");
        json_decref(root);
        return 2;
    }
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return 2;
        }
        t = (time_t) json_integer_value(json_object_get(entry, "t")) / 1000;
        volume = json_number_value(json_object_get(entry, "v"));
        vwap = json_number_value(json_object_get(entry, "vw"));
        open = json_number_value(json_object_get(entry, "o"));
        high = json_number_value(json_object_get(entry, "h"));
        low = json_number_value(json_object_get(entry, "l"));
        close = json_number_value(json_object_get(entry, "c"));
        nTransactions = json_integer_value(json_object_get(entry, "n"));
        timedata = localtime(&t);
        print(mainWindow, "%4d-%02d-%02d vwap: %.2lf, open: %.2lf, high: %.2lf, low: %.2lf, close: %.2lf, vol: %.0lf, trades: %d, shares/trade: %.1lf\n", timedata->tm_year+1900, timedata->tm_mon + 1, timedata->tm_mday, vwap, open, high, low, close, volume, nTransactions, volume / (double)nTransactions);
        
    }

    json_decref(root);

    return 0;
}

int polygonIoVolatility(char *symbol, Date startDate, Date stopDate)
{
    char url[URL_BUFFER_SIZE] = {0};
    sprintf(url, "https://api.polygon.io/v2/aggs/ticker/%s/range/1/day/%4d-%02d-%02d/%4d-%02d-%02d?adjusted=true&sort=asc", symbol, startDate.year, startDate.month, startDate.day, stopDate.year, stopDate.month, stopDate.day);

    json_t *root = polygonIoRESTRequest(url);
    if (root == NULL)
        return 1;

    json_t *results = json_object_get(root, "results");
    if (results == NULL)
        return 1;

    int nResults = json_integer_value(json_object_get(root, "resultsCount"));
    double close = 0;

    double *prices = NULL;
    int nPrices = 0;

    json_t *entry;
    if (!json_is_array(results) || nResults == 0)
    {
        print(mainWindow, "No data from Polygon.IO.\n");
        json_decref(root);
        return 2;
    }
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return 2;
        }
        close = json_number_value(json_object_get(entry, "c"));
        
        if (close > 0.0)
            nPrices++;
    }
    if (nPrices > 0)
    {
        prices = malloc(nPrices * sizeof *prices);
        if (prices == NULL)
        {
            print(mainWindow, "Memory error\n");
            json_decref(root);
            return 1;
        }
        nPrices = 0;
        for (int i = 0; i < json_array_size(results); i++)
        {
            entry = json_array_get(results, i);
            close = json_real_value(json_object_get(entry, "c"));                
            if (close > 0.0)
            {
                prices[nPrices] = close;
                nPrices++;
            }
        }
        double volatility = calculate_volatility(prices, nPrices);
        print(mainWindow, "  Annualized Volatility: %.1lf%%\n", volatility*100);

        free(prices);
    }
    json_decref(root);

    return 0;
}

