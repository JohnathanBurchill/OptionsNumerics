/*
    Options Numerics: on_commands.c

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

#include "on_commands.h"
#include "on_status.h"
#include "on_examples.h"

#include "on_screen_io.h"

#include "on_info.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int initCommands(Command **commands)
{
    if (commands == NULL)
        return ON_MISSING_ARG_POINTER;

    CommandExample noExample = {0};
    Command initcommands[NCOMMANDS] = {
        // Info and help
        {"Info", "about", NULL, "prints information about Options Numerics", "about", aboutFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},
        {"Info", "license", NULL, "prints Options Numerics licensing terms", "license", licenseFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},
        {"Info", "help", "h", "prints information about a topic", "help <topic> (or h <topic> or ? <topic>)", helpFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample},
        {"Info", "help", "?", "prints information about a topic", "help <topic> (or h <topic> or ? <topic>)", helpFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, true},
        {"Info", "example", "ex", "an example is added to the command history and run", "example <command> (or ex <command>)", examplesFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},

        // Calculator
        {"Calculator", "european_option", "eo", "prints European option value for specified parameters; uses Black-Scholes equation (no dividend)", "european_option T:<C or P>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-\%>,R:<risk-free-rate-\%>,P:<underlying-share-price>", blackScholesOptionPriceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"European option price:", "T:C,S:16,E:%d-%02d-%02d,V:90,R:4.3,P:20", "+12f", true}, false},

        {"Calculator", "american_option", "ao", "prints American option value for specified parameters, including dividend yield; uses binomial model", "american_option T:<C or P>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-%%>,R:<risk-free-rate-%%>,Q:<dividend-yield-%%>,P:<underlying-share-price>", binomialOptionPriceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"American option price:", "T:C,S:16,E:%d-%02d-%02d,V:79.3,R:4.3,Q:0,P:20", "+12f", true}, false},

        {"Calculator", "time_decay", "td", "prints option price for each remaining trading day given specified parameters", "time_decay X:<A(merican) or E(european)>,T:<C or P>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-%%>,R:<risk-free-rate-%%>,Q:<dividend-yield-%%>,P:<underlying-share-price>", optionsTimeDecayFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"American-style exercise option price versus time:", "X:A,T:C,S:16,E:%d-%02d-%02d,V:79.3,R:4.3,Q:0,P:20", "+4f", true}, false},

        {"Calculator", "greeks", "gx", "prints greeks using binomial option model", "greeks G:<t(theta), v(ega), d(elta), g(amma), or a(ll)>,T:<C(all) or P(ut)>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-\%>,R:<risk-free-rate>,Q:<dividend-yield-\%>,P:<underlying-share-price>", geeksFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"All greeks:", "G:a,T:C,S:16,E:%d-%02d-%02d,V:80,R:4.31,Q:0,P:20", "+12f", true}, false},

        {"Calculator", "implied_volatility", "iv", "prints implied volatility using binomial option model", "implied_volatility T:<C(all) or P(ut)>,S:<strike-price>,E:<expiry-date>, R:<risk-free-rate>,Q:<dividend-yield-\%>,P:<underlying-share-price>,B:<underlying-share-bid>,A:<underlying-share-ask>", impliedVolatilityFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Bid and ask implied volatilities for an American option:", "T:C,S:16,E:%d-%02d-%02d,R:4.31,Q:0,P:20,B:5.70,A:6.30", "+12f", true}, false},

        {"Calculator", "implied_price", "ip", "prints implied price using binomial option model", "implied_price T:<C(all) or P(ut)>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-\%>,R:<risk-free-rate>,Q:<dividend-yield-\%>,O:<option-price>", impliedPriceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Implied price of underlying asset for an American option:", "T:C,S:16,E:%d-%02d-%02d,V:80,R:4.31,Q:0,O:6", "+12f", true}, false},

        {"Calculator", "fees", NULL, "prints total and per share trading fees", "fees U:<S(tock) or O(ption)>,N:<number-of-units>,F:<flat-fee>,P:<per-unit-fee>,X:<O(ne)- or T(wo)-way trip)>", feesFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Trading fees for 10 option contracts:", "U:O,N:10,F:9.99,P:1.24,X:T", NULL, true}, false},

        {"Calculator", "time_value", "tv", "prints the past, present or future value of money", "time_value $:<amount>,<reference-date>,<requested-date>,r:<annual-interest-rate-percent>", timeValueOfMoneyFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Present value of a $10 bill found six months from now, with 8\% inflation:", "$:10,+6m,today,r:8.0", NULL, true}, false},

        // // Strategy
        // {"Strategy", "time_value", "tv", "prints time value of money", "time_value", timeValueOfMoneyFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},

        // Polygon.IO
        {"Polygon.IO", "options_search", "os", "searches historical or current options contract ticker names", "options_search <ticker>,T:<C(all) or P(ut),s:<min-strike>,S:<max-strike>,e:<earliest-expiry>,E:<latest-expiry>,X:<expired only? T(rue) or F(alse)>", pioOptionsSearchFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Options contracts search at Polygon-IO:", "GME,T:C,s:20,S:25,e:+2f,E:+12f,X:N", NULL, true}, false},

        {"Polygon.IO", "options_chain", "oc", "searches a stock's current options chain", "options_chain <ticker>,T:<C(all) or P(ut),s:<min-strike>,S:<max-strike>,e:<earliest-expiry>,E:<latest-expiry>,v:<min-value>", pioOptionsChainFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Options chain search at Polygon-IO:", "GME,T:C,s:20,S:25,e:+2f,E:+12f,v:0", NULL, true}, false},

        {"Polygon.IO", "price_history", "ph", "prints a stock's or option's daily price history", "price_history <ticker>,<firstDate>,<lastDate>", pioPriceHistoryFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Print the price history for a ticker:", "GME,-1y,today", NULL, true}, false},

        {"Polygon.IO", "price_volatility", "pv", "prints a stock's or option's volatility", "price_volatility <ticker>,<firstDate>,<lastDate>", pioVolatilityFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Print the annualized price volatility for a ticker:", "GME,-1m,today", NULL, true}, false},

        {"Polygon.IO", "latest_price", "lp", "prints a stock's or option's latest price information", "latest_price <ticker>", pioLatestPriceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Latest price information for a stock:", "GME", NULL, true}, false},

        {"Polygon.IO", "previous_close", "pc", "prints a stock's or option's previous close", "previous_close <ticker>", pioPreviousCloseFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Previous close data for a stock:", "GME", NULL, true}, false},

        // FRED
        {"FRED", "fred_sofr", "fs", "prints the latest secured overnight financing rate (SOFR) from FRED", "fred_sofr", fredSOFRFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Latest SOFR from FRED:", NULL, NULL, true}, false},

        // Questrade
        {"Questrade", "qs_connection", "qsc", "connects or reconnects to Questrade's API for a specified account number and API token", "qs_connection", questradeConnectionFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},

        // What if?
        {"What if?", "options_income_reinvested", "oir", "prints extrapolated income from options assuming profits are re-invested in shares", "options_income_reinvested N:<starting-number-of-shares>D:<$/week after fees>,T:<tax-rate-\%>,R:<\%-reinvested>,P:<underlying-share-price>,Y:<number-of-years>", optionsIncomeReinvestedFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Extrapolated options income from reinvested profits:", "N:1000,D:0.2,T:40,R:50,P:30,Y:10", NULL, true}, false},

        {"What if?", "options_exercise_chance", "oec", "prints the rough chance that a short call or put option will be exercised (uses Polygon.IO)", "options_exercise_chance <optionsTicker>", optionsExerciseChanceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Rough chance that a short option contract will be exercised:", "O:GME%02d%02d%02dC00030000", "+3F", true}, false},

        // Misc
        // For information and help usage. No function call
        {"Miscellaneous", "exit", "q", "exits Options Numerics", "exit (or quit or q)", NULL, FUNCTION_NONE, FUNCTION_NONE, noExample, false},
        {"Miscellaneous", "quit", "q", "exits Options Numerics", "exit (or quit or q)", NULL, FUNCTION_NONE, FUNCTION_NONE, noExample, true},
        {"Miscellaneous", "commands", NULL, "list commands entered", "commands", showCommandsFunction, FUNCTION_NONE, FUNCTION_STATUS_CODE, noExample, false},
        {"Miscellaneous", "echo", NULL, "prints its arguments", "echo <arguments>", echoFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Print the argument verbatim:", ".sdrawkcab etirw ot nuf s`tI", NULL, true}, false},
        {"Miscellaneous", "time", NULL, "prints the current time", "time", timeFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Print the time according to your device:", "It\'s about ", NULL, true}, false},
        {"Miscellaneous", "run_all_command_examples", NULL, "runs all command examples", "run_all_command_examples", testCommandsFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false}

    };
    *commands = calloc(NCOMMANDS, sizeof(Command));
    if (*commands == NULL)
        return ON_HEAP_MEMORY_ERROR;

    memcpy(*commands, initcommands, NCOMMANDS * sizeof(Command));

    return ON_OK;
}

void freeCommands(Command *commands)
{
    free(commands);

    return;
}

void memorize(UserInputState *userInput, char *this)
{

    if (userInput == NULL || this == NULL || strlen(this) == 0)
        return;

    for (int i = 0; i < userInput->numberOfThingsRemembered; i++)
    {
        if (strcmp(this, userInput->thingsRemembered[i].thing) == 0)
        {
            if (i < userInput->numberOfThingsRemembered - 1)
            {
                ThingToRemember pinkBunnyCar = userInput->thingsRemembered[i];
                memmove(userInput->thingsRemembered + i, userInput->thingsRemembered + i + 1, (userInput->numberOfThingsRemembered - 1 - i) * sizeof(ThingToRemember));
                userInput->thingsRemembered[userInput->numberOfThingsRemembered - 1] = pinkBunnyCar;
            }
            userInput->thingsRemembered[userInput->numberOfThingsRemembered - 1].timesRemembered++;
            userInput->thinkingOf = userInput->numberOfThingsRemembered;
            userInput->recallDirection = -1;
            return;
        }
    }

    if (userInput->numberOfThingsRemembered >= ON_NUMBER_OF_THINGS_TO_REMEMBER - 1)
    {
        free(userInput->thingsRemembered[0].thing);
        memmove(userInput->thingsRemembered, userInput->thingsRemembered + 1, (userInput->numberOfThingsRemembered - 1) * sizeof userInput->thingsRemembered);
        userInput->numberOfThingsRemembered = ON_NUMBER_OF_THINGS_TO_REMEMBER - 1;
    }
    userInput->numberOfThingsRemembered++;
    userInput->thingsRemembered[userInput->numberOfThingsRemembered-1].timesRemembered = 1;
    userInput->numberOfThingsRemembered[userInput->thingsRemembered-1].thing = strdup(this);
    userInput->thinkingOf = userInput->numberOfThingsRemembered;
    userInput->recallDirection = -1;

    return;
}

void forgetEverything(UserInputState *userInput)
{
    if (userInput == NULL)
        return;

    for (int i = 0; i < userInput->numberOfThingsRemembered; i++)
    {
        free(userInput->thingsRemembered[i].thing);
        userInput->thingsRemembered[i].thing = NULL;
        userInput->thingsRemembered[i].timesRemembered = 0;
    }
    userInput->numberOfThingsRemembered = 0;
    userInput->thinkingOf = 0;
    return;
}

const char *recallPrevious(UserInputState *userInput)
{

    if (userInput == NULL)
        return NULL;

    const char *thing = NULL;
    if (userInput->recallDirection == 1)
    {
        userInput->recallDirection = -1;
        userInput->thinkingOf--;
    }
    if (userInput->thinkingOf > 0 && userInput->thinkingOf <= userInput->numberOfThingsRemembered)
        thing = userInput->thingsRemembered[userInput->thinkingOf-1].thing;

    userInput->thinkingOf--;
    if (userInput->thinkingOf < 0)
        userInput->thinkingOf = 0;

    return thing;
}

const char *recallNext(UserInputState *userInput)
{

    if (userInput == NULL)
        return NULL;

    const char *thing = NULL;
    if (userInput->recallDirection == -1)
    {
        userInput->recallDirection = 1;
        userInput->thinkingOf++;
    }
    if (userInput->thinkingOf > 0 && userInput->thinkingOf < userInput->numberOfThingsRemembered)
        thing = userInput->thingsRemembered[userInput->thinkingOf].thing;

    userInput->thinkingOf++;
    if (userInput->thinkingOf > userInput->numberOfThingsRemembered)
        userInput->thinkingOf = userInput->numberOfThingsRemembered + 1;

    return thing;
}

const char *recallMostUsed(UserInputState *userInput)
{
    if (userInput == NULL)
        return NULL;

    int mostUsed = userInput->thingsRemembered[0].timesRemembered;
    for (int i = 1; i < userInput->numberOfThingsRemembered; i++)
    {
        // If same number of times remembered, use most recent
        if (userInput->thingsRemembered[i].timesRemembered >= mostUsed)
        {
            mostUsed = userInput->thingsRemembered[i].timesRemembered;
            userInput->thinkingOf = i + 1;
        }
    }

    return userInput->thingsRemembered[userInput->thinkingOf - 1].thing;
}

const char *recallMostRecent(UserInputState *userInput)
{
    if (userInput == NULL)
        return NULL;

    userInput->thinkingOf = userInput->numberOfThingsRemembered;
    return recallPrevious(userInput);
}

int writeDownThingsToRemember(UserInputState *userInput)
{
    if (userInput == NULL)
        return ON_MISSING_ARG_POINTER;

    char *homeDir = getenv("HOME");
    if (access(homeDir, F_OK) != 0)
        return ON_FILE_WRITE_ERROR;

    char thingsToRememberFile[FILENAME_MAX];
    snprintf(thingsToRememberFile, FILENAME_MAX, "%s/%s/%s", homeDir, ON_OPTIONS_DIR, ON_THINGS_TO_REMEMBER_FILENAME);

    FILE *f = fopen(thingsToRememberFile, "w");
    if (f == NULL)
        return ON_FILE_WRITE_ERROR;

    for (int i = 0; i < userInput->numberOfThingsRemembered; i++)
        fprintf(f, "%lu %s\n", userInput->thingsRemembered[i].timesRemembered, userInput->thingsRemembered[i].thing);

    fclose(f);

    return ON_OK;
}

int reviseThingsToRemember(UserInputState *userInput)
{
    if (userInput == NULL)
        return ON_MISSING_ARG_POINTER;

    char *homeDir = getenv("HOME");
    if (access(homeDir, F_OK) != 0)
        return ON_FILE_READ_ERROR;

    char thingsToRememberFile[FILENAME_MAX];
    snprintf(thingsToRememberFile, FILENAME_MAX, "%s/%s/%s", homeDir, ON_OPTIONS_DIR, ON_THINGS_TO_REMEMBER_FILENAME);

    userInput->numberOfThingsRemembered = 0;

    FILE *f = fopen(thingsToRememberFile, "r");
    if (f == NULL)
    {
        bzero(userInput->thingsRemembered, sizeof(ThingToRemember) * ON_NUMBER_OF_THINGS_TO_REMEMBER);
        return ON_FILE_READ_ERROR;
    }

    char lineBuf[1000] = {0};
    size_t nTimes = 0;
    char thingBuf[1000] = {0};
    int results = 0;

    char *res = fgets(lineBuf, ON_BUFFERED_LINE_LENGTH, f);
    char *c = NULL;
    while (res != NULL && userInput->numberOfThingsRemembered < ON_NUMBER_OF_THINGS_TO_REMEMBER)
    {
        nTimes = atoi(lineBuf);
        c = lineBuf;
        while (*c != ' ' && c < lineBuf + 999)
            c++;
        c++;
        c[strlen(c)-1] = '\0';
        userInput->thingsRemembered[userInput->numberOfThingsRemembered].timesRemembered = nTimes;
        userInput->thingsRemembered[userInput->numberOfThingsRemembered].thing = strdup(c);
        res = fgets(lineBuf, ON_BUFFERED_LINE_LENGTH, f);
        userInput->numberOfThingsRemembered++;
    }

    userInput->thinkingOf = userInput->numberOfThingsRemembered;

    return ON_OK;
}

void showRememberedThings(ScreenState *screen)
{
    if (screen == NULL)
        return;
        
    print(screen, screen->mainWindow, "Remembered:\n");
    for (int i = 0; i < screen->userInput->numberOfThingsRemembered; i++)
        print(screen, screen->mainWindow, " %4d %s (%ld)\n", i + 1, screen->userInput->thingsRemembered[i].thing, screen->userInput->thingsRemembered[i].timesRemembered);

    return;
}

