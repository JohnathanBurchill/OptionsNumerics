/*
    Options Numerics: on_info.c

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

#include "on_info.h"
#include "on_commands.h"
#include "on_remote.h"
#include "on_utilities.h"
#include "on_screen_io.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>

#include <curl/curl.h>

extern Command *commands;

extern WINDOW *mainWindow;
extern long mainWindowLines;

FunctionValue aboutFunction(FunctionValue notUsed)
{
    (void)notUsed;

    if (mainWindow == NULL)

    return FV_NOTOK;

    print(mainWindow, "%s\n", READING_CUE);
    print(mainWindow, "%sOptions Numerics\n", READING_CUE);
    print(mainWindow, "%sCopyright 2023 Johnathan K. Burchill\n", READING_CUE);
    print(mainWindow, "%s\n", READING_CUE);
    print(mainWindow, "%sThis program comes with ABSOLUTELY NO WARRANTY.\n", READING_CUE);
    print(mainWindow, "%s\n", READING_CUE);
    print(mainWindow, "%sThis is free software, and you are welcome to redistribute\n", READING_CUE);
    print(mainWindow, "%sit under the terms of the GNU General Public License Version 3.\n", READING_CUE);
    print(mainWindow, "%sFor details type \"license\" or look at the LICENSE file in\n", READING_CUE);
    print(mainWindow, "%sthe source code.\n", READING_CUE);
    print(mainWindow, "%s\n", READING_CUE);
    print(mainWindow, "%sUP and DOWN arrow keys cycle the command history\n", READING_CUE);
    print(mainWindow, "%sh assists, q desists\n", READING_CUE);
    print(mainWindow, "%s\n", READING_CUE);

    return FV_OK;
}

static size_t curlCallback(char *data, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    struct curlData *mem = (struct curlData *)userdata;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL)
        return 0; /* out of memory! */

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

FunctionValue licenseFunction(FunctionValue notUsed)
{
    (void)notUsed;
    
    CURL *curl = curl_easy_init();

    struct curlData data = {0};
    CURLcode res = CURLE_OK;

    bool gotLicense = false;

    char url[256] = {0};
    if (curl)
    {
        sprintf(url, "https://www.gnu.org/licenses/gpl-3.0.txt");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        /* Perform the request */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res == CURLE_OK && data.response != NULL)
        {
            struct winsize w;
            ioctl(0, TIOCGWINSZ, &w);
            int height = w.ws_row;
            int lineCount = 0;
            char action = 0;
            int maxLineLength = 0;
            char *start = data.response;
            char *end = strchr(start, '\n');
            while (end != NULL)
            {
                maxLineLength=0;
                while (end != NULL && lineCount < height - 1)
                {
                    *end = 0;
                    if (strlen(start) > maxLineLength)
                        maxLineLength = strlen(start);
                    print(mainWindow, "%s%s\n", READING_CUE, start);
                    lineCount++;
                    start = end + 1;
                    end = strchr(start, '\n');
                }
                if (end != NULL)
                    action = continueOrQuit(maxLineLength, false);

                if (action == 'q')
                    break;

                lineCount = 0;
            }
            gotLicense = true;
        }
    }
    if (!gotLicense)
    {
        print(mainWindow, "%s\n", READING_CUE);
        print(mainWindow, "%sOptions Numerics runs on the assumption that you agree\n", READING_CUE);
        print(mainWindow, "%sto use it under the terms of the GNU General Public License\n", READING_CUE);
        print(mainWindow, "%sVersion 3.\n", READING_CUE);
        print(mainWindow, "%s\n", READING_CUE);
        print(mainWindow, "%sSee https://www.gnu.org/licenses/gpl-3.0.txt for terms.\n", READING_CUE);
        print(mainWindow, "%s\n", READING_CUE);
    }

    curl_easy_cleanup(curl);
    free(data.response);

    return FV_OK;

}

FunctionValue helpFunction(FunctionValue topicArg)
{
    if (commands == NULL)
        return FV_NOTOK;

    char *topic = topicArg.charStarValue;
    if (topic == NULL || topic[0] == 0)
    {
        print(mainWindow, "%s\n", READING_CUE);
        print(mainWindow, "%shelp <topic> describes any of these topics\n", READING_CUE);
        print(mainWindow, "%s  Calculator or calc\n", READING_CUE);
        print(mainWindow, "%s  Data\n", READING_CUE);
        print(mainWindow, "%s  API\n", READING_CUE);
        print(mainWindow, "%s  Commands\n", READING_CUE);
        print(mainWindow, "%s  Dates\n", READING_CUE);
        print(mainWindow, "%s  Assists\n", READING_CUE);
        print(mainWindow, "%s\n", READING_CUE);
    }
    else if (strcasecmp("data", topic) == 0)
    {
        print(mainWindow, "%shelp <source> for information about commands for any of these data sources:\n", READING_CUE);
        print(mainWindow, "%s  PIO (Polygon.IO stock, option, forex and crypto data)\n", READING_CUE);
        print(mainWindow, "%s  FRED (St. Louis Federal Reserve Economic Data portal)\n", READING_CUE);
        print(mainWindow, "%s  Questrade\n", READING_CUE);
    }
    else if (strcasecmp("calculator", topic) == 0 || strcasecmp("calc", topic) == 0)
    {
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "eo", "print book, time, and total value for a European option (Black-Scholes, no dividend)");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "ao", "print book, time, and total value for an American option including dividend (binomial model)");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "greeks", "option properties; type \"help greeks\" for details");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "time_value or tv", "time value of money");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "fees", "total trading fee and fee per share or contract");
        
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "a <operator> b", "evaluate a simple math expression on numbers a and b");
        print(mainWindow, "%s%20s   %s\n", READING_CUE, "", "the whitespace is required; <operator> can be");
        print(mainWindow, "%s%20s    %s\n", READING_CUE, "", "+ (a plus b)");
        print(mainWindow, "%s%20s    %s\n", READING_CUE, "", "- (a minus b)");
        print(mainWindow, "%s%20s    %s\n", READING_CUE, "", "* (a times b)");
        print(mainWindow, "%s%20s    %s\n", READING_CUE, "", "/ (a divided by b)");
        print(mainWindow, "%s%20s    %s\n", READING_CUE, "", "^ (a to the power of b)");
        print(mainWindow, "%s%20s    %s\n", READING_CUE, "", "| (bit-wise logical OR of a and b, both interpreted as integers)");
        print(mainWindow, "%s%20s    %s\n", READING_CUE, "", "& (bit-wise logical AND of a and b, both interpreted as integers)");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "<operator> b", "evaluate a simple math expression using the result of the previous math expression as the first operand");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "result = b", "set the calculator result buffer to the value b");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "0x...", "number entered in hexidecimal notation");
    }
    else if (strcasecmp("greeks", topic) == 0)
    {
        print(mainWindow, "%s  Option greeks estimated from the binomial option model using finite difference derivatives\n", READING_CUE);
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "t", "theta, the time rate of change of option value in $/day");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "v", "vega, the volatility rate of change of option value in $/%%");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "d", "delta, the stock price rate of change of option value in $/$");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "g", "gamma, time rate of change of maxwell $/$/$");
        print(mainWindow, "%s%20s - %s\n", READING_CUE, "a", "all of the above");
    }
    else if (strcasecmp("api", topic) == 0)
    {
        print(mainWindow, "%s%25s - %s\n", READING_CUE, "api_names", "list supported APIs by name");
        print(mainWindow, "%s%25s - %s\n", READING_CUE, "list_key_names", "list the cached key names by API name");
        print(mainWindow, "%s%25s - %s\n", READING_CUE, "add_key <api_name>", "cache a key on disk for the specified API name");
        print(mainWindow, "%s%25s - %s\n", READING_CUE, "rm_key <api_name>", "remove a cached key for the specified API name");
    }
    else if (strcasecmp("assists", topic) == 0)
    {
        print(mainWindow, "%s%25s - %s\n", READING_CUE, "<Ctrl>-F", "search output window for a word or phrase");
        print(mainWindow, "%s%25s - %s\n", READING_CUE, "<Ctrl>-R", "repeat last search");
    }
    else if (strcasecmp("questrade", topic) == 0)
    {
        print(mainWindow, "%s  Questrade (get API key from https://questrade.com)\n", READING_CUE);
        print(mainWindow, "%s%25s - %s\n", READING_CUE, "questrade_connect", "connect to Questrade API using an account number and personal API token");
    }
    else if (strcasecmp("fred", topic) == 0)
    {
        print(mainWindow, "%s  FRED (get API key from https://fred.stlouisfed.org)\n", READING_CUE);
        print(mainWindow, "%s%25s - %s\n", READING_CUE, "fred_sofr or fs", "get the latest Secured Overnight Financing Rate (SOFR) from FRED");
    }
    else if (strcasecmp("pio", topic) == 0)
    {
        for (int i = 0; i < NCOMMANDS; i++)
        {
            if (strcmp("Polygon.IO", commands[i].commandGroup) == 0 && commands[i].helpMessage != NULL && !commands[i].isAlias)
                print(mainWindow, "%s%30s - %s\n", READING_CUE, commands[i].longName, commands[i].helpMessage);
        }
        print(mainWindow, "%s\n", READING_CUE);
    }
    else if (strcasecmp("dates", topic) == 0)
    {
        print(mainWindow, "%s\n", READING_CUE);
        print(mainWindow, "%sDates can be absolute (YYYY-MM-DD) or relative (+60d, -4w or +2m)\n", READING_CUE);
        print(mainWindow, "%sDate ranges are entered <date1>,<date2>\n", READING_CUE);
        print(mainWindow, "%sRelative dates specify intervals from today\n", READING_CUE);
        print(mainWindow, "%sRelative dates start with + or - followed by an integer and an interval unit\n", READING_CUE);
        print(mainWindow, "%sThere interval units are recognized: d (days), f (Fridays), w (calendar weeks), F (3rd Fridays of the month), m (calendar months), y (years)\n", READING_CUE);
        print(mainWindow, "%s\"today\" is an alias for today\n", READING_CUE);
        print(mainWindow, "%sSome dates are meaningless for certain commands; e.g., historical dates for an options_chain\n", READING_CUE);
        print(mainWindow, "%s\n", READING_CUE);
        print(mainWindow, "%sDate range examples: \"2023-03-17,2023-04-21\", \"-1y,+3m\", and \"today,+5w\"\n", READING_CUE);
        print(mainWindow, "%s\n", READING_CUE);
    }
    else if (strcasecmp("commands", topic) == 0)
    {
        char * prevGroup = "";
        for (int i = 0; i < NCOMMANDS; i++)
        {
            if (strcmp(commands[i].commandGroup, prevGroup) != 0)
            {
                print(mainWindow, "%s\n", READING_CUE);
                print(mainWindow, "%s%s\n", READING_CUE, commands[i].commandGroup);
            }
            prevGroup = commands[i].commandGroup;
            if (commands[i].helpMessage != NULL && !commands[i].isAlias)
                print(mainWindow, "%s%30s - %s\n", READING_CUE, commands[i].longName, commands[i].helpMessage);
        }
        print(mainWindow, "%s\n", READING_CUE);
        print(mainWindow, "%sexample <command> loads a template into the command history and runs it\n", READING_CUE);
        print(mainWindow, "%s\n", READING_CUE);
    }
    else // get help on a command
    {
        for (int i = 0; i < NCOMMANDS; i++)
        {
            if (strcasecmp(commands[i].longName, topic) == 0 || (commands[i].shortName != NULL && strcasecmp(commands[i].shortName, topic) == 0))
            {
                print(mainWindow, "%s\n", READING_CUE);
                print(mainWindow, "%s%20s: %s\n", READING_CUE, "Command", commands[i].longName);
                print(mainWindow, "%s%20s: %s\n", READING_CUE, "Description", commands[i].helpMessage);
                print(mainWindow, "%s%20s: %s\n", READING_CUE, "Short form", commands[i].shortName != NULL ? commands[i].shortName : "--");
                print(mainWindow, "%s%20s: %s\n", READING_CUE, "Usage", commands[i].usage != NULL ? commands[i].usage : "--");
                print(mainWindow, "%s\n", READING_CUE);
                break;
            }
        }
    }

    return FV_OK;
    
}

void api_names(void)
{
    print(mainWindow, "Supported API names (case sensitive) for data providers:\n");
    print(mainWindow, "  QUESTRADE (https://questrade.com)\n");
    print(mainWindow, "  FRED (https://fred.stlouisfed.org)\n");
    print(mainWindow, "  PIO (https://polygon.io)\n");

    return;
}

