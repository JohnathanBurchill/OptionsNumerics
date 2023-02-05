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
#include "on_examples.h"

#include "on_screen_io.h"

#include "on_info.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

Command *commands = NULL;

extern WINDOW *mainWindow;

static ThingToRemember thingsRemembered[ON_NUMBER_OF_THINGS_TO_REMEMBER] = {0};
static int numberOfThingsRemembered = 0;
static int thinkingOf = 0;
static int recallDirection = -1;

int initCommands(void)
{
    CommandExample noExample = {0};
    Command initcommands[NCOMMANDS] = {
        // Info and help
        {"Info", "about", NULL, "prints information about Options Numerics", "about", aboutFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},
        {"Info", "license", NULL, "prints Options Numerics licensing terms", "license", licenseFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},
        {"Info", "help", "h", "prints information about a topic", "help <topic> (or h <topic> or ? <topic>)", helpFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample},
        {"Info", "help", "?", "prints information about a topic", "help <topic> (or h <topic> or ? <topic>)", helpFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, true},
        {"Info", "example", "ex", "an example is added to the command history and run", "example <command> (or ex <command>)", examplesFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},

        // Calculator
        {"Calculator", "european_option", "eo", "prints European option value for specified parameters; uses Black-Scholes equation (no dividend)", "european_option T:<C or P>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-%%>,R:<risk-free-rate-%%>,P:<underlying-share-price>", blackScholesOptionPriceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"European option price:", "T:C,S:16,E:%d-%02d-%02d,V:90,R:4.3,P:20", "+12f", true}, false},

        {"Calculator", "american_option", "ao", "prints American option value for specified parameters, including dividend yield; uses binomial model", "american_option T:<C or P>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-%%>,R:<risk-free-rate-%%>,Q:<dividend-yield-%%>,P:<underlying-share-price>", binomialOptionPriceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"American option price:", "T:C,S:16,E:%d-%02d-%02d,V:79.3,R:4.3,Q:0,P:20", "+12f", true}, false},

        {"Calculator", "time_decay", "td", "prints option price for each remaining trading day given specified parameters", "time_decay X:<A(merican) or E(european)>,T:<C or P>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-%%>,R:<risk-free-rate-%%>,Q:<dividend-yield-%%>,P:<underlying-share-price>", optionsTimeDecayFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"American-style exercise option price versus time:", "X:A,T:C,S:16,E:%d-%02d-%02d,V:79.3,R:4.3,Q:0,P:20", "+4f", true}, false},

        {"Calculator", "greeks", "gx", "prints greeks using binomial option model", "greeks G:<t(theta), v(ega), d(elta), g(amma), or a(ll)>,T:<C(all) or P(ut)>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-%%>,R:<risk-free-rate>,Q:<dividend-yield-%%>,P:<underlying-share-price>", geeksFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"All greeks:", "G:a,T:C,S:16,E:%d-%02d-%02d,V:80,R:4.31,Q:0,P:20", "+12f", true}, false},

        {"Calculator", "implied_volatility", "iv", "prints implied volatility using binomial option model", "implied_volatility T:<C(all) or P(ut)>,S:<strike-price>,E:<expiry-date>, R:<risk-free-rate>,Q:<dividend-yield-%%>,P:<underlying-share-price>,B:<underlying-share-bid>,A:<underlying-share-ask>", impliedVolatilityFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Bid and ask implied volatilities for an American option:", "T:C,S:16,E:%d-%02d-%02d,R:4.31,Q:0,P:20,B:5.70,A:6.30", "+12f", true}, false},

        {"Calculator", "implied_price", "ip", "prints implied price using binomial option model", "implied_price T:<C(all) or P(ut)>,S:<strike-price>,E:<expiry-date>,V:<underlying-share-volatility-%%>,R:<risk-free-rate>,Q:<dividend-yield-%%>,O:<option-price>", impliedPriceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Implied price of underlying asset for an American option:", "T:C,S:16,E:%d-%02d-%02d,V:80,R:4.31,Q:0,O:6", "+12f", true}, false},

        {"Calculator", "fees", NULL, "prints total and per share trading fees", "fees U:<S(tock) or O(ption)>,N:<number-of-units>,F:<flat-fee>,P:<per-unit-fee>,X:<O(ne)- or T(wo)-way trip)>", feesFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Trading fees for 10 option contracts:", "U:O,N:10,F:9.99,P:1.24,X:T", NULL, true}, false},

        {"Calculator", "time_value", "tv", "prints time value of money", "time_value", timeValueOfMoneyFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},

        // Polygon.IO
        {"Polygon.IO", "options_search", "os", "searches historical or current options contract ticker names", "options_search <ticker>,T:<C(all) or P(ut),s:<min-strike>,S:<max-strike>,e:<earliest-expiry>,E:<latest-expiry>,X:<expired only? T(rue) or F(alse)>", pioOptionsSearchFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Options contracts search at Polygon-IO:", "GME,T:C,s:20,S:25,e:+2f,E:+12f,X:N", NULL, true}, false},

        {"Polygon.IO", "options_chain", "oc", "searches a stock's current options chain", "options_chain <ticker>,T:<C(all) or P(ut),s:<min-strike>,S:<max-strike>,e:<earliest-expiry>,E:<latest-expiry>,v:<min-value>", pioOptionsChainFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Options chain search at Polygon-IO:", "GME,T:C,s:20,S:25,e:+2f,E:+12f,v:0", NULL, true}, false},

        {"Polygon.IO", "price_history", "ph", "prints a stock's or option's daily price history", "price_history", pioPriceHistory, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},

        {"Polygon.IO", "price_volatility", "pv", "prints a stock's or option's volatility", "price_volatility", pioVolatilityFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},

        {"Polygon.IO", "latest_price", "lp", "prints a stock's or option's latest price information", "latest_price <ticker>", pioLatestPriceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Latest price information for a stock:", "GME", NULL, true}, false},

        {"Polygon.IO", "previous_close", "pc", "prints a stock's or option's previous close", "previous_close <ticker>", pioPreviousCloseFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Previous close data for a stock:", "GME", NULL, true}, false},

        // FRED
        {"FRED", "fred_sofr", "fs", "prints the latest secured overnight financing rate (SOFR) from FRED", "fred_sofr", fredSOFRFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Latest SOFR from FRED:", NULL, NULL, true}, false},

        // Questrade
        {"Questrade", "qs_connection", "qsc", "connects or reconnects to Questrade's API for a specified account number and API token", "qs_connection", questradeConnectionFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},

        // What if?
        {"What if?", "options_income_reinvested", "oir", "prints extrapolated income from options assuming profits are re-invested in shares", "options_income_reinvested N:<starting-number-of-shares>D:<$/week after fees>,T:<tax-rate-%%>,R:<%%-reinvested>,P:<underlying-share-price>,Y:<number-of-years>", optionsIncomeReinvestedFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Extrapolated options income from reinvested profits:", "N:1000,D:0.2,T:40,R:50,P:30,Y:10", NULL, true}, false},

        {"What if?", "options_exercise_chance", "oec", "prints the rough chance that a short call or put option will be exercised (uses Polygon.IO)", "options_exercise_chance <optionsTicker>", optionsExerciseChanceFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, {"Rough chance that a short option contract will be exercised:", "O:GME%02d%02d%02dC00030000", "+3F", true}, false},

        // Misc
        // For information and help usage. No function call
        {"Miscellaneous", "exit", "q", "exits Options Numerics", "exit (or quit or q)", NULL, FUNCTION_NONE, FUNCTION_NONE, noExample, false},
        {"Miscellaneous", "quit", "q", "exits Options Numerics", "exit (or quit or q)", NULL, FUNCTION_NONE, FUNCTION_NONE, noExample, true},
        {"Miscellaneous", "commands", NULL, "list commands entered", "commands", showCommandsFunction, FUNCTION_NONE, FUNCTION_STATUS_CODE, noExample, false},
        {"Miscellaneous", "echo", NULL, "prints its arguments", "echo <arguments>", echoFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false},
        {"Miscellaneous", "time", NULL, "prints the current time", "time", timeFunction, FUNCTION_CHARSTAR, FUNCTION_STATUS_CODE, noExample, false}

    };
    commands = calloc(NCOMMANDS, sizeof *commands);
    if (commands == NULL)
        return 1;

    memcpy(commands, initcommands, NCOMMANDS * sizeof *commands);

    return 0;    
}

void freeCommands(void)
{
    free(commands);

    return;
}

void memorize(char *this)
{
    if (this == NULL || strlen(this) == 0)
        return;

    for (int i = 0; i < numberOfThingsRemembered; i++)
    {
        if (strcmp(this, thingsRemembered[i].thing) == 0)
        {
            if (i < numberOfThingsRemembered - 1)
            {
                ThingToRemember pinkBunnyCar = thingsRemembered[i];
                memmove(thingsRemembered + i, thingsRemembered + i + 1, (numberOfThingsRemembered - 1 - i) * sizeof(ThingToRemember));
                thingsRemembered[numberOfThingsRemembered - 1] = pinkBunnyCar;
            }
            thingsRemembered[numberOfThingsRemembered - 1].timesRemembered++;
            thinkingOf = numberOfThingsRemembered;
            recallDirection = -1;
            return;
        }
    }

    if (numberOfThingsRemembered >= ON_NUMBER_OF_THINGS_TO_REMEMBER - 1)
    {
        free(thingsRemembered[0].thing);
        memmove(thingsRemembered, thingsRemembered + 1, (numberOfThingsRemembered - 1) * sizeof thingsRemembered);
        numberOfThingsRemembered = ON_NUMBER_OF_THINGS_TO_REMEMBER - 1;
    }
    numberOfThingsRemembered++;
    thingsRemembered[numberOfThingsRemembered-1].timesRemembered = 1;
    numberOfThingsRemembered[thingsRemembered-1].thing = strdup(this);
    thinkingOf = numberOfThingsRemembered;
    recallDirection = -1;

    return;
}

void forgetEverything(void)
{
    for (int i = 0; i < numberOfThingsRemembered; i++)
    {
        free(thingsRemembered[i].thing);
        thingsRemembered[i].thing = NULL;
        thingsRemembered[i].timesRemembered = 0;
    }
    numberOfThingsRemembered = 0;
    thinkingOf = 0;
    return;
}

const char *recallPrevious(void)
{
    const char *thing = NULL;
    if (recallDirection == 1)
    {
        recallDirection = -1;
        thinkingOf--;
    }
    if (thinkingOf > 0 && thinkingOf <= numberOfThingsRemembered)
        thing = thingsRemembered[thinkingOf-1].thing;

    thinkingOf--;
    if (thinkingOf < 0)
        thinkingOf = 0;

    return thing;
}

const char *recallNext(void)
{
    const char *thing = NULL;
    if (recallDirection == -1)
    {
        recallDirection = 1;
        thinkingOf++;
    }
    if (thinkingOf > 0 && thinkingOf < numberOfThingsRemembered)
        thing = thingsRemembered[thinkingOf].thing;

    thinkingOf++;
    if (thinkingOf > numberOfThingsRemembered)
        thinkingOf = numberOfThingsRemembered + 1;

    return thing;
}

const char *recallMostUsed(void)
{
    int mostUsed = thingsRemembered[0].timesRemembered;
    for (int i = 1; i < numberOfThingsRemembered; i++)
    {
        // If same number of times remembered, use most recent
        if (thingsRemembered[i].timesRemembered >= mostUsed)
        {
            mostUsed = thingsRemembered[i].timesRemembered;
            thinkingOf = i + 1;
        }
    }

    return thingsRemembered[thinkingOf - 1].thing;
}

const char *recallMostRecent(void)
{
    thinkingOf = numberOfThingsRemembered;
    return recallPrevious();
}

int writeDownThingsToRemember(void)
{
    char *homeDir = getenv("HOME");
    if (access(homeDir, F_OK) != 0)
        return 0;

    char thingsToRememberFile[FILENAME_MAX];
    snprintf(thingsToRememberFile, FILENAME_MAX, "%s/%s/%s", homeDir, ON_OPTIONS_DIR, ON_THINGS_TO_REMEMBER_FILENAME);

    FILE *f = fopen(thingsToRememberFile, "w");
    if (f == NULL)
        return 1;

    for (int i = 0; i < numberOfThingsRemembered; i++)
        fprintf(f, "%lu %s\n", thingsRemembered[i].timesRemembered, thingsRemembered[i].thing);

    fclose(f);

    return 0;
}

int reviseThingsToRemember(void)
{
    char *homeDir = getenv("HOME");
    if (access(homeDir, F_OK) != 0)
        return 0;

    char thingsToRememberFile[FILENAME_MAX];
    snprintf(thingsToRememberFile, FILENAME_MAX, "%s/%s/%s", homeDir, ON_OPTIONS_DIR, ON_THINGS_TO_REMEMBER_FILENAME);

    numberOfThingsRemembered = 0;

    FILE *f = fopen(thingsToRememberFile, "r");
    if (f == NULL)
    {
        bzero(thingsRemembered, sizeof(ThingToRemember) * ON_NUMBER_OF_THINGS_TO_REMEMBER);
        return 1;
    }

    char lineBuf[1000] = {0};
    size_t nTimes = 0;
    char thingBuf[1000] = {0};
    int results = 0;

    char *res = fgets(lineBuf, ON_BUFFERED_LINE_LENGTH, f);
    char *c = NULL;
    while (res != NULL && numberOfThingsRemembered < ON_NUMBER_OF_THINGS_TO_REMEMBER)
    {
        nTimes = atoi(lineBuf);       
        c = lineBuf;
        while (*c != ' ' && c < lineBuf + 999)
            c++;
        c++;
        c[strlen(c)-1] = '\0';
        thingsRemembered[numberOfThingsRemembered].timesRemembered = nTimes;
        thingsRemembered[numberOfThingsRemembered].thing = strdup(c);
        res = fgets(lineBuf, ON_BUFFERED_LINE_LENGTH, f);
        numberOfThingsRemembered++;
    }

    thinkingOf = numberOfThingsRemembered;

    return 0;
}

void showRememberedThings(void)
{
    print(mainWindow, "Remembered:\n");
    for (int i = 0; i < numberOfThingsRemembered; i++)
        print(mainWindow, " %4d %s (%ld)\n", i + 1, thingsRemembered[i].thing, thingsRemembered[i].timesRemembered);

    return;
}

