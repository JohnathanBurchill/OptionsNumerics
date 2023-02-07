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
#include "on_status.h"
#include "on_config.h"
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

FunctionValue aboutFunction(ScreenState *screen, FunctionValue notUsed)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    (void)notUsed;

    print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sOptions Numerics\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sCopyright 2023 Johnathan K. Burchill\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sThis program comes with ABSOLUTELY NO WARRANTY.\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sThis is free software, and you are welcome to redistribute\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sit under the terms of the GNU General Public License Version 3.\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sFor details type \"license\" or look at the LICENSE file in\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sthe source code.\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sUP and DOWN arrow keys cycle the command history\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%sh assists, q desists\n", ON_READING_CUE);
    print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);

    return FV_OK;
}

static size_t curlCallback(char *data, size_t size, size_t nmemb, void *userdata)
{
    if (data == NULL || userdata == NULL)
        return ON_OK;

    size_t realsize = size * nmemb;
    struct curlData *mem = (struct curlData *)userdata;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL)
        return ON_OK; /* out of memory! */

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

FunctionValue licenseFunction(ScreenState *screen, FunctionValue notUsed)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

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
            int height = screen->mainWindowViewHeight;
            int lineCount = 0;
            char action = 0;
            int maxLineLength = 0;
            char *start = data.response;
            char *end = strchr(start, '\n');
            while (end != NULL)
            {
                maxLineLength=0;
                prepareForALotOfOutput(screen, height - 1);
                while (end != NULL && lineCount < height - 1)
                {
                    *end = 0;
                    if (strlen(start) > maxLineLength)
                        maxLineLength = strlen(start);
                    print(screen, screen->mainWindow, "%s%s\n", ON_READING_CUE, start);
                    lineCount++;
                    start = end + 1;
                    end = strchr(start, '\n');
                }
                if (end != NULL)
                    action = continueOrQuit(screen, maxLineLength, false);

                if (action == 'q')
                    break;


                lineCount = 0;
            }
            gotLicense = true;
        }
    }
    if (!gotLicense)
    {
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sOptions Numerics runs on the assumption that you agree\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sto use it under the terms of the GNU General Public License\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sVersion 3.\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sSee https://www.gnu.org/licenses/gpl-3.0.txt for terms.\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    }

    curl_easy_cleanup(curl);
    free(data.response);

    return FV_OK;

}

FunctionValue helpFunction(ScreenState *screen, FunctionValue topicArg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    UserInputState *userInput = screen->userInput;
    if (userInput->commands == NULL)
        return FV_NOTOK;

    char *topic = topicArg.charStarValue;
    if (topic == NULL || topic[0] == 0)
    {
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%shelp <topic> describes any of these topics\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  Calculator or calc\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  Data\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  API\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  Commands\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  Dates\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  Assists\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    }
    else if (strcasecmp("data", topic) == 0)
    {
        print(screen, screen->mainWindow, "%shelp <source> for information about commands for any of these data sources:\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  PIO (Polygon.IO stock, option, forex and crypto data)\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  FRED (St. Louis Federal Reserve Economic Data portal)\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s  Questrade\n", ON_READING_CUE);
    }
    else if (strcasecmp("calculator", topic) == 0 || strcasecmp("calc", topic) == 0)
    {
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "eo", "print book, time, and total value for a European option (Black-Scholes, no dividend)");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "ao", "print book, time, and total value for an American option including dividend (binomial model)");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "greeks", "option properties; type \"help greeks\" for details");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "time_value or tv", "time value of money");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "fees", "total trading fee and fee per share or contract");
        
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "a <operator> b", "evaluate a simple math expression on numbers a and b");
        print(screen, screen->mainWindow, "%s%20s   %s\n", ON_READING_CUE, "", "the whitespace is required; <operator> can be");
        print(screen, screen->mainWindow, "%s%20s    %s\n", ON_READING_CUE, "", "+ (a plus b)");
        print(screen, screen->mainWindow, "%s%20s    %s\n", ON_READING_CUE, "", "- (a minus b)");
        print(screen, screen->mainWindow, "%s%20s    %s\n", ON_READING_CUE, "", "* (a times b)");
        print(screen, screen->mainWindow, "%s%20s    %s\n", ON_READING_CUE, "", "/ (a divided by b)");
        print(screen, screen->mainWindow, "%s%20s    %s\n", ON_READING_CUE, "", "^ (a to the power of b)");
        print(screen, screen->mainWindow, "%s%20s    %s\n", ON_READING_CUE, "", "| (bit-wise logical OR of a and b, both interpreted as integers)");
        print(screen, screen->mainWindow, "%s%20s    %s\n", ON_READING_CUE, "", "& (bit-wise logical AND of a and b, both interpreted as integers)");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "<operator> b", "evaluate a simple math expression using the result of the previous math expression as the first operand");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "result = b", "set the calculator result buffer to the value b");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "0x...", "number entered in hexidecimal notation");
    }
    else if (strcasecmp("greeks", topic) == 0)
    {
        print(screen, screen->mainWindow, "%s  Option greeks estimated from the binomial option model using finite difference derivatives\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "t", "theta, the time rate of change of option value in $/day");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "v", "vega, the volatility rate of change of option value in $/%%");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "d", "delta, the stock price rate of change of option value in $/$");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "g", "gamma, time rate of change of maxwell $/$/$");
        print(screen, screen->mainWindow, "%s%20s - %s\n", ON_READING_CUE, "a", "all of the above");
    }
    else if (strcasecmp("api", topic) == 0)
    {
        print(screen, screen->mainWindow, "%s%25s - %s\n", ON_READING_CUE, "api_names", "list supported APIs by name");
        print(screen, screen->mainWindow, "%s%25s - %s\n", ON_READING_CUE, "list_key_names", "list the cached key names by API name");
        print(screen, screen->mainWindow, "%s%25s - %s\n", ON_READING_CUE, "add_key <api_name>", "cache a key on disk for the specified API name");
        print(screen, screen->mainWindow, "%s%25s - %s\n", ON_READING_CUE, "rm_key <api_name>", "remove a cached key for the specified API name");
    }
    else if (strcasecmp("assists", topic) == 0)
    {
        print(screen, screen->mainWindow, "%s%25s - %s\n", ON_READING_CUE, "<Ctrl>-F", "search output window for a word or phrase");
        print(screen, screen->mainWindow, "%s%25s - %s\n", ON_READING_CUE, "<Ctrl>-R", "repeat last search");
    }
    else if (strcasecmp("questrade", topic) == 0)
    {
        print(screen, screen->mainWindow, "%s  Questrade (get API key from https://questrade.com)\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s%25s - %s\n", ON_READING_CUE, "questrade_connect", "connect to Questrade API using an account number and personal API token");
    }
    else if (strcasecmp("fred", topic) == 0)
    {
        print(screen, screen->mainWindow, "%s  FRED (get API key from https://fred.stlouisfed.org)\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s%25s - %s\n", ON_READING_CUE, "fred_sofr or fs", "get the latest Secured Overnight Financing Rate (SOFR) from FRED");
    }
    else if (strcasecmp("pio", topic) == 0)
    {
        for (int i = 0; i < NCOMMANDS; i++)
        {
            if (strcmp("Polygon.IO", userInput->commands[i].commandGroup) == 0 && userInput->commands[i].helpMessage != NULL && !userInput->commands[i].isAlias)
                print(screen, screen->mainWindow, "%s%30s - %s\n", ON_READING_CUE, userInput->commands[i].longName, userInput->commands[i].helpMessage);
        }
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    }
    else if (strcasecmp("dates", topic) == 0)
    {
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sDates can be absolute (YYYY-MM-DD) or relative (+60d, -4w or +2m)\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sDate ranges are entered <date1>,<date2>\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sRelative dates specify intervals from today\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sRelative dates start with + or - followed by an integer and an interval unit\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sThere interval units are recognized: d (days), f (Fridays), w (calendar weeks), F (3rd Fridays of the month), m (calendar months), y (years)\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s\"today\" is an alias for today\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sSome dates are meaningless for certain commands; e.g., historical dates for an options_chain\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sDate range examples: \"2023-03-17,2023-04-21\", \"-1y,+3m\", and \"today,+5w\"\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    }
    else if (strcasecmp("commands", topic) == 0)
    {
        char * prevGroup = "";
        for (int i = 0; i < NCOMMANDS; i++)
        {
            if (strcmp(userInput->commands[i].commandGroup, prevGroup) != 0)
            {
                print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
                print(screen, screen->mainWindow, "%s%s\n", ON_READING_CUE, userInput->commands[i].commandGroup);
            }
            prevGroup = userInput->commands[i].commandGroup;
            if (userInput->commands[i].helpMessage != NULL && !userInput->commands[i].isAlias)
                print(screen, screen->mainWindow, "%s%30s - %s\n", ON_READING_CUE, userInput->commands[i].longName, userInput->commands[i].helpMessage);
        }
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%sexample <command> loads a template into the command history and runs it\n", ON_READING_CUE);
        print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
    }
    else // get help on a command
    {
        for (int i = 0; i < NCOMMANDS; i++)
        {
            if (strcasecmp(userInput->commands[i].longName, topic) == 0 || (userInput->commands[i].shortName != NULL && strcasecmp(userInput->commands[i].shortName, topic) == 0))
            {
                print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
                print(screen, screen->mainWindow, "%s%20s: %s\n", ON_READING_CUE, "Command", userInput->commands[i].longName);
                print(screen, screen->mainWindow, "%s%20s: %s\n", ON_READING_CUE, "Description", userInput->commands[i].helpMessage);
                print(screen, screen->mainWindow, "%s%20s: %s\n", ON_READING_CUE, "Short form", userInput->commands[i].shortName != NULL ? userInput->commands[i].shortName : "--");
                print(screen, screen->mainWindow, "%s%20s: %s\n", ON_READING_CUE, "Usage", userInput->commands[i].usage != NULL ? userInput->commands[i].usage : "--");
                print(screen, screen->mainWindow, "%s\n", ON_READING_CUE);
                break;
            }
        }
    }

    return FV_OK;
    
}

void api_names(ScreenState *screen)
{
    if (screen == NULL)
        return;

    print(screen, screen->mainWindow, "Supported API names (case sensitive) for data providers:\n");
    print(screen, screen->mainWindow, "  QUESTRADE (https://questrade.com)\n");
    print(screen, screen->mainWindow, "  FRED (https://fred.stlouisfed.org)\n");
    print(screen, screen->mainWindow, "  PIO (https://polygon.io)\n");

    return;
}

