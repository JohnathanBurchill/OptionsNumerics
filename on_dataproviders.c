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
#include "on_status.h"
#include "on_api.h"
#include "on_statistics.h"
#include "on_parse.h"
#include "on_remote.h"
#include "on_data.h"
#include "on_screen_io.h"
#include "on_optionstiming.h"
#include "on_optionsmodels.h"
#include "on_utilities.h"

#include <string.h>
#include <curl/curl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#include <jansson.h>

#include <ncurses.h>

// Based on ChatGPT response 13 Jan 2023
int updateQuestradeAccessToken(ScreenState *screen)
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
            if (screen != NULL && screen->statusWindow != NULL)
                mvwprintw(screen->statusWindow, 0, 0, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            status = ON_REST_LIBCURL_ERROR;
        }
        else
        {
            // Check response
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if (screen != NULL && screen->statusWindow != NULL)
                mvwprintw(screen->statusWindow, 0, 0, "HTTP response code: %ld\n", httpCode);
            switch (httpCode)
            {
                case 401:
                    if (screen != NULL)
                        print(screen, screen->mainWindow, "Access token is invalid.\n");
                    status = ON_QUESTRADE_INVALID_TOKEN;
                    break;
                case 404:
                    if (screen != NULL)
                        print(screen, screen->mainWindow, "Account not recognized.\n");
                    status = ON_QUESTRADE_INVALID_ACCOUNT;
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

double questradeStockQuote(ScreenState *screen, char *ticker)
{
    return 0.0;
}

// From libcurl c example
size_t restCallback(char *data, size_t size, size_t nmemb, void *userdata)
{
    if (data == NULL || userdata == NULL)
        return 0;

    size_t realsize = size *nmemb;
    CurlData *mem = (CurlData *)userdata;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL)
        return 0;

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

int fredSOFR(ScreenState *screen, double *sofr)
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
    int status = ON_OK;

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
            if (screen != NULL && screen->statusWindow != NULL)
                mvwprintw(screen->statusWindow, 0, 0, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            status = ON_REST_LIBCURL_ERROR;
            goto cleanup;
        }

        if (data.response != NULL)
        {
            json_error_t error = {0};
            root = json_loads(data.response, 0, &error);
            if (!root)
            {
                if (screen != NULL)
                    print(screen, screen->mainWindow, "Could not parse FRED JSON response: line %d: text: %s\n", error.line, error.text);
                status = ON_FRED_INVALID_JSON;
                goto cleanup;
            }

            json_t *observations = json_object_get(root, "observations");
            if (!json_is_array(observations))
            {
                status = ON_FRED_INVALID_JSON;
                goto cleanup;
            }
            for (int i = 0; i < json_array_size(observations); i++)
            {
                entry = json_array_get(observations, i);
                if (!json_is_object(entry))
                {
                    status = ON_FRED_INVALID_JSON;
                    goto cleanup;
                }
                date = json_string_value(json_object_get(entry, "date"));
                rateStr = json_string_value(json_object_get(entry, "value"));
                if (rateStr != NULL)
                {
                    rate = atof(rateStr);
                }
                else
                {
                    status = ON_FRED_INVALID_JSON;
                    rate = nan("");
                }

                if (sofr != NULL)
                    *sofr = rate;                
                if (screen != NULL)
                    print(screen, screen->mainWindow, "  FRED SOFR on %s: %g%%\n", date, rate);
            }
        }

    cleanup:
        json_decref(root);
        free(data.response);
        curl_easy_cleanup(curl);
    }

    bzero(token, strlen(token));
    free(token);

    return status;
}

json_t *polygonIoRESTRequest(ScreenState *screen, const char *requestUrl)
{
    json_t *root = NULL;
    CURL *curl;
    CURLcode res;
    char url[URL_BUFFER_SIZE] = {0};

    char *token = loadApiToken("PIO.apitoken");
    if (token == NULL)
    {
        char *tmptoken = getpass("  Polygon.IO (PIO) personal API token: ");
        if (tmptoken == NULL || strlen(tmptoken) == 0)
            return NULL;

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
        // print(screen, screen->mainWindow, "url:\n%s\n", url);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        bzero(url, strlen(url));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, restCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        /* Perform the request */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
        {
            if (screen != NULL && screen->statusWindow != NULL)
                mvwprintw(screen->statusWindow, 0, 0, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            goto cleanup;
        }

        if (data.response != NULL)
        {
            int size = data.size;
            // print(screen, screen->mainWindow, "Polygon.io response:\n%s\n", data.response);
            json_error_t error = {0};
            root = json_loads(data.response, 0, &error);
            if (!root)
            {
                if (screen != NULL)
                    print(screen, screen->mainWindow, "Could not parse Polygon.IO JSON response: line %d: text: %s\n", error.line, error.text);
                goto cleanup;
            }
            else if (!json_is_object(root))
            {
                if (screen != NULL)
                    print(screen, screen->mainWindow, "Invalid JSON response from Polygon.IO.\n");
                json_decref(root);
                root = NULL;
                goto cleanup;
            }
            char *jsonstatus = (char *)json_string_value(json_object_get(root, "status"));
            if (strcmp("ERROR", jsonstatus) == 0)
            {
                char *msg = (char *)json_string_value(json_object_get(root, "error"));
                if (screen != NULL)
                    print(screen, screen->mainWindow, "Polygon.IO: %s\n", msg);
                json_decref(root);
                root = NULL;
                goto cleanup;
            }
            else if (strcmp("NOT_AUTHORIZED", jsonstatus) == 0)
            {
                char *msg = (char *)json_string_value(json_object_get(root, "message"));
                if (screen != NULL)
                    print(screen, screen->mainWindow, "Polygon.IO: %s\n", msg);
                json_decref(root);
                root = NULL;
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


int polygonIoOptionsSearch(ScreenState *screen, char *ticker, char type, double minstrike, double maxstrike, Date date1, Date date2, bool expired, char **nextPagePtr)
{
    char url[URL_BUFFER_SIZE] = {0};
    bool continuedSearch = false;
    int status = 0;

    if (screen == NULL)
        return ON_NO_SCREEN;
    if (ticker == NULL)
        return ON_PIO_NO_TICKER_ARG;

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

    json_t *root = polygonIoRESTRequest(screen, url);
    if (root == NULL)
        return ON_PIO_REST_NO_JSON_ROOT;

    json_t *results = json_object_get(root, "results");
    if (results == NULL)
        return ON_PIO_REST_NO_JSON_RESULTS;

    time_t t = 0;

    char *optionTicker = NULL;
    double strike = 0;
    double prevStrike = 0;

    char *expiry = NULL;
    json_t *entry;
    if (!json_is_array(results) || json_array_size(results) == 0)
    {
        print(screen, screen->mainWindow, "No data from Polygon.IO.\n");
        json_decref(root);
        return ON_PIO_REST_JSON_NO_ARRAY_ENTRY;
    }
    if (!continuedSearch)
        print(screen, screen->mainWindow, "%15s   %8s   %s\n", "Expiry date", "Strike", "Ticker");
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(screen, screen->mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return ON_PIO_REST_INVALID_JSON;
        }
        optionTicker = (char *)json_string_value(json_object_get(entry, "ticker"));
        strike = json_number_value(json_object_get(entry, "strike_price"));
        expiry = (char *)json_string_value(json_object_get(entry, "expiration_date"));
        if (strike != prevStrike)
            print(screen, screen->mainWindow, "\n");
        print(screen, screen->mainWindow, "%8.2lf %15s   %s\n", strike, expiry, optionTicker);
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

    return ON_OK;
}

int polygonIoOptionsChain(ScreenState *screen, char *ticker, char type, double minstrike, double maxstrike, Date date1, Date date2, double minpremium, char **nextPagePtr)
{

    if (screen == NULL)
        return ON_NO_SCREEN;

    if (ticker == NULL)
        return ON_PIO_NO_TICKER_ARG;

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

    json_t *root = polygonIoRESTRequest(screen, url);
    if (root == NULL)
        return ON_PIO_REST_NO_JSON_ROOT;

    json_t *results = json_object_get(root, "results");
    if (results == NULL)
        return ON_PIO_REST_NO_JSON_RESULTS;

    time_t t = 0;

    json_t *entry;
    if (!json_is_array(results) || json_array_size(results) == 0)
    {
        print(screen, screen->mainWindow, "No data from Polygon.IO.\n");
        json_decref(root);
        return ON_PIO_REST_JSON_NO_ARRAY_ENTRY;
    }
    if (!continuedSearch)
        print(screen, screen->mainWindow, "%10s %8s %s %s %s %s\n", "Strike", "Expiry", "Bid", "Ask", "Last", "Ticker");

    json_t *details = NULL;
    json_t *quote = NULL;
    json_t *dayInfo = NULL;
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(screen, screen->mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return ON_PIO_REST_INVALID_JSON;
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
                print(screen, screen->mainWindow, "\n");
            print(screen, screen->mainWindow, "%8.2lf %15s %.3lf %.3lf %.3lf  %s\n", strike, expiry, bid, ask, close, optionTicker);
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

    return ON_OK;
}

int polygonIoLatestPrice(ScreenState *screen, char *ticker, TickerData *tickerData, OptionsData *optionsData, bool verbose)
{
    // Maybe later this will return data to the caller, but 
    // for now only prints to screen, so require a screen
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (ticker == NULL)
        return ON_PIO_NO_TICKER_ARG;

    char url[URL_BUFFER_SIZE] = {0};

    int market = STOCKS;

    if (strncmp("O:", ticker, 2) == 0)
    {
        market = OPTIONS;
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
    {
        market = FOREX;
        sprintf(url, "https://api.polygon.io/v2/snapshot/locale/global/markets/forex/tickers/%s", ticker);
    }
    else if (strncmp("X:", ticker, 2) == 0)
    {
        market = CRYPTO;
        sprintf(url, "https://api.polygon.io/v2/snapshot/locale/global/markets/crypto/tickers/%s", ticker);
    }
    else
        sprintf(url, "https://api.polygon.io/v2/snapshot/locale/us/markets/stocks/tickers/%s", ticker);

    json_t *root = polygonIoRESTRequest(screen, url);
    if (root == NULL)
        return ON_PIO_REST_NO_JSON_ROOT;

    int status = ON_OK;

    switch(market)
    {
        case STOCKS:
            status = printLatestPriceStocks(screen, root);
            break;
        case OPTIONS:
            status = printLatestPriceOptions(screen, root);
            break;
    }

    json_decref(root);

    return status;
}

int printLatestPriceStocks(ScreenState *screen, json_t *root)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (!json_is_object(root))
        return ON_PIO_INVALID_RESPONSE;

    int status = ON_OK;

    json_t *ticker = json_object_get(root, "ticker");    
    if (!json_is_object(ticker))
        return ON_PIO_NO_STOCK_TICKER;

    char *tickerName = (char *)json_string_value(json_object_get(ticker, "ticker"));
    if (tickerName == NULL || strlen(tickerName) == 0)
        return ON_PIO_NO_STOCK_TICKER_NAME;

    double updatetime = json_number_value(json_object_get(ticker, "updated"));
    double change = json_number_value(json_object_get(ticker, "todaysChange"));
    double changePercent = json_number_value(json_object_get(ticker, "todaysChangePerc"));
    double mdailyVolume = 0.0;
    double mclose = 0.0;

    // json_t *day = json_object_get(ticker, "day");
    // json_t *prevDay = json_object_get(ticker, "prevDay");
    // json_t *quote = json_object_get(ticker, "lastQuote");
    // json_t *trade = json_object_get(ticker, "lastTrade");

    json_t *minuteData = json_object_get(ticker, "min");
    if (json_is_object(minuteData))
    {
        mdailyVolume = json_number_value(json_object_get(minuteData, "av"));
        mclose = json_number_value(json_object_get(minuteData, "c"));
        char *t = nanoSecondsAsStringMustFree(updatetime);

        print(screen, screen->mainWindow, "%5s $%.3lf ($%+.3lf, %.2lf%%) Vol: %.0lf @ %s\n", tickerName, mclose, change, changePercent, mdailyVolume, t == NULL ? "? UTC" : t);

        free(t);
    }
    else
        status = ON_PIO_NO_STOCK_MINUTE_DATA;

    return status;

}

int printLatestPriceOptions(ScreenState *screen, json_t *root)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (!json_is_object(root))
        return ON_PIO_INVALID_RESPONSE;

    json_t *results = json_object_get(root, "results");    
    if (!json_is_object(results))
        return ON_PIO_NO_OPTIONS_RESULTS;

    double oi = json_number_value(json_object_get(results, "open_interest"));

    json_t *details = json_object_get(results, "details");
    if (!json_is_object(details))
        return ON_PIO_NO_OPTIONS_DETAILS;
        
    char *tickerName = (char *)json_string_value(json_object_get(details, "ticker"));
    double strike = json_number_value(json_object_get(details, "strike_price"));
    char *expiry = (char *)json_string_value(json_object_get(details, "expiration_date"));
    char *otype = (char *)json_string_value(json_object_get(details, "contract_type"));
    OptionType type = OTHER;
    if (strcmp("call", otype) == 0)
        type = CALL;
    else if (strcmp("put", otype) == 0)
        type = PUT;

    Date expiryDate = {0};
    interpretDate(expiry, &expiryDate);
    int daysLeft = tradingDaysToExpiry(expiryDate);
    double yearsLeft = (double)daysLeft / (double)OPTIONS_TRADING_DAYS_PER_YEAR;

    double close = 0.0;
    double volume = 0.0;
    double prevClose = 0.0;
    double uprice = 0.0;
    double uvolatility = 0.0;
    double impliedVolatility = 0.0;

    json_t *day = json_object_get(results, "day");
    if (!json_is_object(day))
        return ON_PIO_NO_OPTIONS_DAY_INFO;

    double updatetime = json_number_value(json_object_get(day, "last_updated"));
    double change = json_number_value(json_object_get(day, "change"));
    double changePercent = json_number_value(json_object_get(day, "change_percent"));

    close = json_number_value(json_object_get(day, "close"));
    prevClose = json_number_value(json_object_get(day, "previous_close"));
    volume = json_number_value(json_object_get(day, "volume"));

    json_t *underlying = json_object_get(results, "underlying_asset");
    char *uticker = (char *)json_string_value(json_object_get(underlying, "ticker"));
    json_t *p = json_object_get(underlying, "price");
    if (p != NULL)
    {
        double sofr = 0;
        int fres = fredSOFR(NULL, &sofr);
        if (fres == ON_FRED_OK)
        {
            uprice = json_number_value(p);
            // TODO? get dividend rate from PIO stock info
            char start[20] = {0};
            snprintf(start, 20, "-%dd", daysLeft);
            Date startDate = {0};
            if (daysLeft < 5)
                interpretDate("-1w", &startDate);
            else
                interpretDate(start, &startDate);
            Date stopDate = {0};
            interpretDate("today", &stopDate);
            int pres = polygonIoVolatility(screen, uticker, startDate, stopDate, &uvolatility);

            if (pres == 0)
            {
                Option opt = {uprice, strike, sofr/100.0, 0.0, uvolatility/100.0, yearsLeft};
                int ivres = binomial_option_implied_volatility(opt, type, close, &impliedVolatility);

            }
        }
    }

    char *t = nanoSecondsAsStringMustFree(updatetime);

    print(screen, screen->mainWindow, "%25s $%.3lf/$%.3lf ($%+.3lf, %.2lf%%) Vol: %.0lf OI: %.0lf IV: %.1lf%%/%.1lf%% @ %s\n", tickerName, close, uprice, change, changePercent, volume, oi, impliedVolatility * 100.0, uvolatility, t == NULL ? "? UTC" : t);

    free(t);

    return ON_OK;

}

int polygonIoPreviousClose(ScreenState *screen, char *ticker)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (ticker == NULL)
        return ON_PIO_NO_TICKER_ARG;
    
    char url[URL_BUFFER_SIZE] = {0};
    sprintf(url, "https://api.polygon.io/v2/aggs/ticker/%s/prev?adjusted=true", ticker);

    json_t *root = polygonIoRESTRequest(screen, url);

    json_t *results = json_object_get(root, "results");
    int nResults = json_integer_value(json_object_get(root, "resultsCount"));
    if (results == NULL || !json_is_array(results) || nResults == 0)
    {
        json_decref(root);
        return ON_PIO_REST_NO_JSON_RESULTS;
    }

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
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(screen, screen->mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return ON_PIO_REST_JSON_NO_ARRAY_ENTRY;
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

        print(screen, screen->mainWindow, "%4d-%02d-%02d vwap: %.2lf, open: %.2lf, high: %.2lf, low: %.2lf, close: %.2lf, vol: %.0lf, trades: %d, shares/trade: %.1lf\n", timedata->tm_year+1900, timedata->tm_mon + 1, timedata->tm_mday, vwap, open, high, low, close, volume, nTransactions, volume / (double)nTransactions);
        
    }

    json_decref(root);

    return ON_OK;
}

int polygonIoPriceHistory(ScreenState *screen, char *ticker, Date startDate, Date stopDate, PriceData *priceData)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (ticker == NULL)
        return ON_PIO_NO_TICKER_ARG;
    
    char url[URL_BUFFER_SIZE] = {0};
    sprintf(url, "https://api.polygon.io/v2/aggs/ticker/%s/range/1/day/%4d-%02d-%02d/%4d-%02d-%02d?adjusted=true&sort=asc", ticker, startDate.year, startDate.month, startDate.day, stopDate.year, stopDate.month, stopDate.day);

    json_t *root = polygonIoRESTRequest(screen, url);
    if (root == NULL)
        return ON_PIO_REST_NO_JSON_ROOT;

    json_t *results = json_object_get(root, "results");
    int nResults = json_integer_value(json_object_get(root, "resultsCount"));
    if (results == NULL || !json_is_array(results) || nResults == 0)
    {
        json_decref(root);
        return ON_PIO_REST_NO_JSON_RESULTS;
    }
        
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
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            print(screen, screen->mainWindow, "Invalid JSON entry %d\n", i);
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
        print(screen, screen->mainWindow, "%4d-%02d-%02d vwap: %.2lf, open: %.2lf, high: %.2lf, low: %.2lf, close: %.2lf, vol: %.0lf, trades: %d, shares/trade: %.1lf\n", timedata->tm_year+1900, timedata->tm_mon + 1, timedata->tm_mday, vwap, open, high, low, close, volume, nTransactions, volume / (double)nTransactions);
        
    }

    json_decref(root);

    return ON_OK;
}

int polygonIoVolatility(ScreenState *screen, char *ticker, Date startDate, Date stopDate, double *volatility)
{
    if (screen == NULL && volatility == NULL)
        return ON_MISSING_ARG_POINTER;

    if (ticker == NULL)
        return ON_PIO_NO_TICKER_ARG;
    
    char url[URL_BUFFER_SIZE] = {0};
    sprintf(url, "https://api.polygon.io/v2/aggs/ticker/%s/range/1/day/%4d-%02d-%02d/%4d-%02d-%02d?adjusted=true&sort=asc", ticker, startDate.year, startDate.month, startDate.day, stopDate.year, stopDate.month, stopDate.day);

    json_t *root = polygonIoRESTRequest(screen, url);
    if (root == NULL)
        return ON_PIO_REST_NO_JSON_ROOT;

    json_t *results = json_object_get(root, "results");
    int nResults = json_integer_value(json_object_get(root, "resultsCount"));
    if (results == NULL || !json_is_array(results) || nResults == 0)
    {
        json_decref(root);
        return ON_PIO_REST_NO_JSON_RESULTS;
    }

    double close = 0;

    double *prices = NULL;
    int nPrices = 0;

    json_t *entry;
    for (int i = 0; i < json_array_size(results); i++)
    {
        entry = json_array_get(results, i);
        if (!json_is_object(entry))
        {
            if (screen != NULL)
                print(screen, screen->mainWindow, "Invalid JSON entry %d\n", i);
            json_decref(root);
            return ON_PIO_REST_INVALID_JSON;
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
            if (screen != NULL)
                print(screen, screen->mainWindow, "Memory error\n");
            json_decref(root);
            return ON_HEAP_MEMORY_ERROR;
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
        double vol = calculate_volatility(prices, nPrices);

        if (volatility != NULL)
            *volatility = vol * 100;
        if (screen != NULL)
            print(screen, screen->mainWindow, "  Annualized Volatility: %.1lf%%\n", vol*100);

        free(prices);
    }
    json_decref(root);

    return ON_OK;
}

