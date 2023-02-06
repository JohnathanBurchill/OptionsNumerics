/*
    Options Numerics: on_functions.c

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

#include "on_functions.h"
#include "on_status.h"
#include "on_commands.h"
#include "on_info.h"
#include "on_examples.h"

#include "on_optionsmodels.h"
#include "on_optionstiming.h"
#include "on_dataproviders.h"
#include "on_utilities.h"
#include "on_parse.h"
#include "on_calculate.h"
#include "on_data.h"
#include "on_screen_io.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <sys/ioctl.h>

#include <curl/curl.h>

#include <ncurses.h>

FunctionValue echoFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    if (arg.charStarValue != NULL)
        print(screen, screen->mainWindow, "%s\n", arg.charStarValue);

    return FV_OK;
}

FunctionValue testCommandsFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    (void)arg;

    char command[256] = {0};
    FunctionValue argument = {0};

    for (int i = 0; i < NCOMMANDS; i++)
    {
        char *longName = screen->userInput->commands[i].longName;
        argument.charStarValue = longName;
        print(screen, screen->mainWindow, "---- Testing \'%s\' example ----\n", longName);
        if (screen->userInput->commands[i].function != NULL)
            (void) examplesFunction(screen, argument);
    }

    return FV_OK;
}

FunctionValue timeFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *input = arg.charStarValue;

    struct timeval t = {0};
    struct timezone z = {0};
    int res = gettimeofday(&t, &z);
    if (res != 0)
        return FV_NOTOK;

    time_t tnow = t.tv_sec;

    struct tm *utc = gmtime(&tnow);
    if (utc == NULL)
        return FV_NOTOK;

    struct tm *lt = localtime(&tnow);
    if (lt == NULL)
        return FV_NOTOK;

    char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

    print(screen, screen->mainWindow, "%s%s%02d:%02d:%02d %s on %d %s %d (computer reports %d-%02d-%02d %02d:%02d:%02d.%03d UTC)\n", ON_READING_CUE, input != NULL ? input : "", lt->tm_hour % 12, lt->tm_min, lt->tm_sec, lt->tm_hour > 12 ? "pm" : "am", lt->tm_mday, months[lt->tm_mon], lt->tm_year + 1900, utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday, utc->tm_hour, utc->tm_min, utc->tm_sec, (int)floor(t.tv_usec/1000.0));

    return FV_OK;
}

FunctionValue showCommandsFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    (void)arg;
    showRememberedThings(screen);
    return FV_OK;
}

FunctionValue blackScholesOptionPriceFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    double S, K, r, sigma, optionValue, bookValue, timeValue;
    int year = 0, month = 0, day = 0;
    int daysToExpire = 0;
    double yearsToExpire = 0.0;
    char type = 0;

    char *params = arg.charStarValue;

    // Estimate call or put value from parameters
    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    int nParams = sscanf(parameters, "T:%c,S:%lf,E:%4d-%02d-%02d,V:%lf,R:%lf,P:%lf", &type, &K, &year, &month, &day, &sigma, &r, &S);
    free(parameters);
    if (nParams != 8)
    {
        print(screen, screen->mainWindow, "parameters: T:<C or P>,S:<strike>,E:<yyyy-mm-dd>,V:<volatility %%>,R:<risk-free-rate %%>,P:<underlying-price>\n");
        return FV_NOTOK;
    }

    // Not counting weekends. Does not account for holidays
    Date date = {year, month, day};
    daysToExpire = tradingDaysToExpiry(date);

    print(screen, screen->mainWindow, "%25s: $%.2lf\n", "Strike", K);
    print(screen, screen->mainWindow, "%25s: %4d-%02d-%02d (in %d trading days, %0.1lf weeks)\n", "Expiry", year, month, day, daysToExpire, (double)daysToExpire / 5.0);
    print(screen, screen->mainWindow, "%25s: %.1lf%%\n", "Volatility", sigma);
    print(screen, screen->mainWindow, "%25s: %.2lf%%\n", "Risk-free rate", r);
    print(screen, screen->mainWindow, "%25s: $%.2lf\n", "Share price", S);

    yearsToExpire = (double)daysToExpire / (double)OPTIONS_TRADING_DAYS_PER_YEAR;

    OptionType otype = CALL;
    if (type == 'P')
        otype = PUT;
    Option opt = {S, K, r/100.0, 0.0, sigma/100.0, yearsToExpire};
    optionValue = blackscholes_option_value(opt, otype);

    bookValue = S - K;
    if (type == PUT)
        bookValue *= -1.0;
    if (bookValue < 0.0)
        bookValue = 0.0;
    timeValue = optionValue - bookValue;
    print(screen, screen->mainWindow, "%25s: $%.2lf (%.0lf%%)\n", "Book value", bookValue, bookValue / optionValue * 100.0);

    print(screen, screen->mainWindow, "%25s: $%.2lf (%.0lf%%)\n", "Time value", timeValue, timeValue / optionValue * 100.0);
    print(screen, screen->mainWindow, "%25s: $%.2lf\n", otype == CALL ? "Call value" : "Put value", optionValue);

    return FV_OK;
}

FunctionValue binomialOptionPriceFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    double S, K, r, q, sigma, optionValue, bookValue, timeValue;
    int year = 0, month = 0, day = 0;
    int daysToExpire = 0;
    double yearsToExpire = 0.0;
    char type = 0;

    char *params = arg.charStarValue;

    // Estimate call or put value from parameters
    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    int nParams = sscanf(parameters, "T:%c,S:%lf,E:%4d-%02d-%02d,V:%lf,R:%lf,Q:%lf,P:%lf", &type, &K, &year, &month, &day, &sigma, &r, &q, &S);
    free(parameters);
    if (nParams != 9)
    {
        print(screen, screen->mainWindow, "parameters: T:<C or P>,S:<strike>,E:<yyyy-mm-dd>,V:<volatility %%>,R:<risk-free-rate %%>,Q:<dividend-yield %%>,P:<underlying-price>\n");
        return FV_NOTOK;
    }

    // Not counting weekends. Does not account for holidays
    Date date = {year, month, day};
    daysToExpire = tradingDaysToExpiry(date);

    print(screen, screen->mainWindow, "%25s: $%.2lf\n", "Strike", K);
    print(screen, screen->mainWindow, "%25s: %4d-%02d-%02d (in %d trading days, %0.1lf weeks)\n", "Expiry", year, month, day, daysToExpire, (double)daysToExpire / 5.0);
    print(screen, screen->mainWindow, "%25s: %.1lf%%\n", "Volatility", sigma);
    print(screen, screen->mainWindow, "%25s: %.2lf%%\n", "Risk-free rate", r);
    print(screen, screen->mainWindow, "%25s: %.2lf%%\n", "Dividend yield", q);
    print(screen, screen->mainWindow, "%25s: $%.2lf\n", "Share price", S);

    OptionType otype = CALL;
    if (type == 'P')
        otype = PUT;
    yearsToExpire = (double)daysToExpire / (double)OPTIONS_TRADING_DAYS_PER_YEAR;
    Option opt = {S, K, r / 100.0, q / 100.0, sigma / 100.0, yearsToExpire};
    optionValue = binomial_option_value(opt, otype);

    bookValue = S - K;
    if (type == PUT)
        bookValue *= -1.0;
    if (bookValue < 0.0)
        bookValue = 0.0;
    timeValue = optionValue - bookValue;
    print(screen, screen->mainWindow, "%25s: $%.2lf (%.1lf%%)\n", "Book value", bookValue, bookValue / optionValue * 100.0);

    print(screen, screen->mainWindow, "%25s: $%.2lf (%.1lf%%)\n", "Time value", timeValue, timeValue / optionValue * 100.0);
    print(screen, screen->mainWindow, "%25s: $%.4lf\n", otype == CALL ? "Call value" : "Put value", optionValue);

    return FV_OK;
}

FunctionValue optionsTimeDecayFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *params = arg.charStarValue;
    double S, K, r, q, sigma, optionValue, bookValue, timeValue;
    int year = 0, month = 0, day = 0;
    int daysToExpire = 0;
    double yearsToExpire = 0.0;
    char type = 0;
    char exerciseMethod = 0;

    // Estimate call or put value from parameters
    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    int nParams = sscanf(parameters, "X:%c,T:%c,S:%lf,E:%4d-%02d-%02d,V:%lf,R:%lf,Q:%lf,P:%lf", &exerciseMethod, &type, &K, &year, &month, &day, &sigma, &r, &q, &S);
    free(parameters);
    if (nParams != 10)
    {
        print(screen, screen->mainWindow, "parameters: X:<A(merican) or E(european)>,T:<C or P>,S:<strike>,E:<yyyy-mm-dd>,V:<volatility %%>,R:<risk-free-rate %%>,Q:<dividend-yield %%>,P:<underlying-price>\n");
        return FV_NOTOK;
    }

    double (*algorithm) (Option, OptionType) = NULL;
    switch(exerciseMethod)
    {
        case 'E':
            algorithm = blackscholes_option_value;
            break;

        case 'A':
            algorithm = binomial_option_value;
            break;

        default:
            print(screen, screen->mainWindow, "Supported exercise methods: E for European and A for American\n");
            return FV_NOTOK;
    }

    // Not counting weekends. Does not account for holidays
    Date date = {year, month, day};
    daysToExpire = tradingDaysToExpiry(date);

    if (exerciseMethod == 'E')
        q = 0.0;

    Option opt = {S, K, r / 100.0, q / 100.0, sigma / 100.0, 0};
    double price = 0;

    OptionType otype = CALL;
    if (type == 'P')
        otype = PUT;

    prepareForALotOfOutput(screen, daysToExpire + 1);
    print(screen, screen->mainWindow, "Days to go\tprice\n");
    while (daysToExpire > 0)
    {
        opt.T = (double)daysToExpire / (double)OPTIONS_TRADING_DAYS_PER_YEAR;
        price = algorithm(opt, otype);
        print(screen, screen->mainWindow, "%10d\t%8.3lf\n", daysToExpire, price);
        daysToExpire--;
    }

    return FV_OK;
}

FunctionValue geeksFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    double S, K, r, q, sigma, optionValue, bookValue, timeValue;
    int year = 0, month = 0, day = 0;
    int daysToExpire = 0;
    double yearsToExpire = 0.0;
    char type = 0;

    char *params = arg.charStarValue;

    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;
    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    char geek = 0;

    int nParams = sscanf(parameters, "G:%c,T:%c,S:%lf,E:%4d-%02d-%02d,V:%lf,R:%lf,Q:%lf,P:%lf", &geek, &type, &K, &year, &month, &day, &sigma, &r, &q, &S);
    free(parameters);
    if (nParams != 10)
    {
        print(screen, screen->mainWindow, "parameters: G:<t, d, g, v or a>,T:<C or P>,S:<strike>,E:<yyyy-mm-dd>,V:<volatility %%>,R:<risk-free-rate %%>,Q:<dividend-yield %%>,P:<underlying-price>\n");
        return FV_NOTOK;
    }

    // Not counting weekends. Does not account for holidays
    Date date = {year, month, day};
    daysToExpire = tradingDaysToExpiry(date);

    print(screen, screen->mainWindow, "%25s: $%.2lf\n", "Strike", K);
    print(screen, screen->mainWindow, "%25s: %4d-%02d-%02d (in %d trading days, %0.1lf weeks)\n", "Expiry", year, month, day, daysToExpire, (double)daysToExpire / 5.0);
    print(screen, screen->mainWindow, "%25s: %.1lf%%\n", "Volatility", sigma);
    print(screen, screen->mainWindow, "%25s: %.2lf%%\n", "Risk-free rate", r);
    print(screen, screen->mainWindow, "%25s: %.2lf%%\n", "Dividend yield", q);
    print(screen, screen->mainWindow, "%25s: $%.2lf\n", "Share price", S);
    OptionType otype = CALL;
    if (type == 'P')
        otype = PUT;

    yearsToExpire = (double)daysToExpire / (double)OPTIONS_TRADING_DAYS_PER_YEAR;
    Option opt = {S, K, r / 100.0, q / 100.0, sigma / 100.0, yearsToExpire};
    optionValue = binomial_option_value(opt, otype);
    bookValue = S - K;
    if (otype == PUT)
        bookValue *= -1.0;
    if (bookValue < 0.0)
        bookValue = 0.0;
    timeValue = optionValue - bookValue;
    print(screen, screen->mainWindow, "%25s: $%.2lf (%.1lf%%)\n", "Book value", bookValue, bookValue / optionValue * 100.0);

    print(screen, screen->mainWindow, "%25s: $%.2lf (%.1lf%%)\n", "Time value", timeValue, timeValue / optionValue * 100.0);
    print(screen, screen->mainWindow, "%25s: $%.2lf\n", otype == CALL ? "Call value" : "Put value", optionValue);

    double result = 0;
    bool all = geek == 'a';
    if (geek == 't' || all)
    {
        result = binomial_option_geeks(opt, otype, "d$dt");
        print(screen, screen->mainWindow, "%25s: $%.4lf/day\n", "theta", result);
    }
    if (geek == 'v' || all)
    {
        result = binomial_option_geeks(opt, otype, "d$dV");
        print(screen, screen->mainWindow, "%25s: $%.4lf/%%\n", "vega", result);
    }
    if (geek == 'd' || all)
    {
        result = binomial_option_geeks(opt, otype, "d$dP");
        print(screen, screen->mainWindow, "%25s: $%.4lf/$\n", "delta", result);
    }
    if (geek == 'g' || all)
    {
        result = binomial_option_geeks(opt, otype, "d2$dP2");
        print(screen, screen->mainWindow, "%25s: $%.6lf/$/$\n", "gamma", result);
    }

    return FV_OK;
}

FunctionValue impliedVolatilityFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    double S, K, r, q, bid, ask, bidImpliedVolatility = 0, askImpliedVolatility = 0;
    int year = 0, month = 0, day = 0;
    int daysToExpire = 0;
    double yearsToExpire = 0.0;
    char type = 0;

    char *params = arg.charStarValue;

    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);
        
    int nParams = sscanf(parameters, "T:%c,S:%lf,E:%4d-%02d-%02d,R:%lf,Q:%lf,P:%lf,B:%lf,A:%lf", &type, &K, &year, &month, &day, &r, &q, &S, &bid, &ask);
    free(parameters);
    if (nParams != 10)
    {
        print(screen, screen->mainWindow, "parameters: T:<type (C or P)>,S:<strike>,E:<yyyy-mm-dd>,R:<risk-free-rate %%>,Q:<dividend-yield %%>,P:<underlying-price>,B:<option-bid-price>,A:<option-ask-price>\n");
        return FV_NOTOK;
    }

    // Not counting weekends. Does not account for holidays
    Date date = {year, month, day};
    daysToExpire = tradingDaysToExpiry(date);

    print(screen, screen->mainWindow, "%25s: %4d-%02d-%02d (in %d trading days, %0.1lf weeks)\n", "Expiry", year, month, day, daysToExpire, (double)daysToExpire / 5.0);

    yearsToExpire = (double)daysToExpire / (double)OPTIONS_TRADING_DAYS_PER_YEAR;
    Option opt = {S, K, r / 100.0, q / 100.0, 0.0, yearsToExpire};
    int res = binomial_option_implied_volatility(opt, type, bid, &bidImpliedVolatility);
    if (res != 0)
        return (FunctionValue)res;
    res = binomial_option_implied_volatility(opt, type, ask, &askImpliedVolatility);
    if (res != 0)
        return (FunctionValue)res;

    print(screen, screen->mainWindow, "%25s: bid: %.1lf%%, ask %.1lf%%\n", "Implied volatility", bidImpliedVolatility * 100.0, askImpliedVolatility * 100.0);

    return FV_OK;
}

FunctionValue impliedPriceFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    int status = 0;

    double K, v, r, q, optionPrice, impliedPriceOfUnderlying;
    Date expiry = {0};
    int daysToExpire = 0;
    double yearsToExpire = 0.0;
    char type = 0;

    char *params = arg.charStarValue;
    char **tokens = NULL;
    int nTokens = 0;

    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);
        
    char *keys[] = {"T:", "S:", "E:", "V:", "R:", "Q:", "O:", 0};
    tokens = splitStringByKeys(parameters, keys, ',', &nTokens);
    if (tokens == NULL)
    {
        status = 2;
        goto cleanup;
    }

    type = tokens[0][strlen(keys[0])];
    K = atof(tokens[1]+strlen(keys[1]));
    interpretDate(tokens[2]+strlen(keys[2]), &expiry);
    v = atof(tokens[3]+strlen(keys[3]));
    r = atof(tokens[4]+strlen(keys[4]));
    q = atof(tokens[5]+strlen(keys[5]));
    optionPrice = atof(tokens[6]+strlen(keys[6]));

    // Not counting weekends. Does not account for holidays
    daysToExpire = tradingDaysToExpiry(expiry);

    print(screen, screen->mainWindow, "%25s: %4d-%02d-%02d (in %d trading days, %0.1lf weeks)\n", "Expiry", expiry.year, expiry.month, expiry.day, daysToExpire, (double)daysToExpire / 5.0);

    yearsToExpire = (double)daysToExpire / (double)OPTIONS_TRADING_DAYS_PER_YEAR;
    Option opt = {0.0, K, r / 100.0, q / 100.0, v / 100.0, yearsToExpire};
    int res = binomial_option_implied_price_of_underlying(opt, type, optionPrice, &impliedPriceOfUnderlying);
    if (res != 0)
        return (FunctionValue)res;

    print(screen, screen->mainWindow, "%25s: $%.3lf\n", "Implied price", impliedPriceOfUnderlying);

cleanup:
    if (status == 2)
        print(screen, screen->mainWindow, "parameters: T:<type (C or P)>,S:<strike>,E:<yyyy-mm-dd>,V:<underlying-volatility-%%>,R:<risk-free-rate %%>,Q:<dividend-yield %%>,O:<option-price>\n");

    freeTokens(tokens, nTokens);
    free(parameters);

    return (FunctionValue)status;
}

FunctionValue feesFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char unitType = 0;
    int nunits = 0;
    double flatfee = 0.0;
    double perunitfee = 0.0;
    char oneOrTwo = 0;

    char *params = arg.charStarValue;

    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    int nParams = sscanf(parameters, "U:%c,N:%d,F:%lf,P:%lf,X:%c", &unitType, &nunits, &flatfee, &perunitfee, &oneOrTwo);
    free(parameters);
    if (nParams != 5)
    {
        print(screen, screen->mainWindow, "parameters: U:<S(tock) or O(ption)>,N:<number-of-units>,F:<flat-fee>,P:<per-unit-fee>,X:<O(ne)- or T(wo)- way trip)>\n");
        print(screen, screen->mainWindow, " where U is the unit type (stock or option)\n");
        print(screen, screen->mainWindow, "       N is the number of shares or options contracts\n");
        print(screen, screen->mainWindow, "       X is the transaction type, one of:\n");
        print(screen, screen->mainWindow, "         O (one-way buy or sell)\n");
        print(screen, screen->mainWindow, "         T (two-way buy then sell or sell then buy)\n");
        return FV_NOTOK;
    }

    double totalfee = flatfee + perunitfee * (double)nunits;
    if (oneOrTwo == 'T')
        totalfee *=2.0;

    print(screen, screen->mainWindow, "%25s: $%.2lf\n", "Total fee", totalfee);
    print(screen, screen->mainWindow, "%25s: $%.4lf\n", "Per share", totalfee / (double)(nunits * (unitType == 'O' ? 100 : 1)));

    return FV_OK;

}

FunctionValue timeValueOfMoneyFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *input = NULL;
    int status = 0;
    char **tokens = NULL;
    int nTokens = 0;

    double amount = 0;
    double rate = 0;
    Date date1 = {0};
    Date date2 = {0};

    char *params = arg.charStarValue;

    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;
    if (parameters[0] == 0)
    {
        status = 2;
        goto cleanup;
    }

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    char *keys[] = {"$:", "", "", "r:", 0};
    tokens = splitStringByKeys(parameters, keys, ',', &nTokens);
    if (tokens == NULL)
    {
        status = 2;
        goto cleanup;
    }

    amount = atof(tokens[0]+strlen(keys[0]));
    interpretDate(tokens[1], &date1);
    interpretDate(tokens[2], &date2);
    rate = atof(tokens[3]+strlen(keys[3]));

    double newAmount = timeValue(screen, amount, rate, date1, date2);
    print(screen, screen->mainWindow, "Assuming you can get %.2lf%% on your money, $%.2lf on %d-%02d-%02d is worth $%.2lf on %d-%02d-%02d\n", rate, amount, date1.year, date1.month, date1.day, newAmount, date2.year, date2.month, date2.day);

cleanup:
    if (status == 2)
        print(screen, screen->mainWindow, "parameters: $:<amount>,<reference-date>,<requested-date>,r:<annual-interest-rate-percent>\n");

    freeTokens(tokens, nTokens);
    free(parameters);

    return FV_OK;

}

FunctionValue pioOptionsSearchFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *input = NULL;
    char *ticker = NULL;
    char type = 0;
    double minstrike = 0;
    double maxstrike = 0;
    // Expiry date
    Date date1 = {0};
    Date date2 = {0};
    
    bool expired = false;

    char action = 0;
    int status = 0;

    char *params = arg.charStarValue;

    char *parameters = NULL;
    char **tokens = NULL;
    int nTokens = 0;
    
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;
    if (parameters[0] == 0)
    {
        status = 2;
        goto cleanup;
    }

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    char *keys[] = {"", "T:", "s:", "S:", "e:", "E:", "X:", 0};
    tokens = splitStringByKeys(parameters, keys, ',', &nTokens);
    if (tokens == NULL)
    {
        status = 2;
        goto cleanup;
    }

    ticker = strdup(tokens[0]);
    type = tokens[1][strlen(keys[1])];
    minstrike = atof(tokens[2]+strlen(keys[2]));
    maxstrike = atof(tokens[3]+strlen(keys[3]));
    interpretDate(tokens[4]+strlen(keys[4]), &date1);
    interpretDate(tokens[5]+strlen(keys[5]), &date2);
    expired = tokens[6][strlen(keys[6])] == 'Y';

    // Call PIO
    char *nextPagePtr = NULL;
    print(screen, screen->mainWindow, "%s: %s $%.2lf - $%.2lf, %d-%02d-%02d - %d-%02d-%02d\n", ticker, type == 'C' ? "calls" : "puts", minstrike, maxstrike, date1.year, date1.month, date1.day, date2.year, date2.month, date2.day);
    do
    {
        polygonIoOptionsSearch(screen, ticker, type, minstrike, maxstrike, date1, date2, expired, &nextPagePtr);
        if (nextPagePtr != NULL)
            action = continueOrQuit(screen, 50, false);
    } while (nextPagePtr != NULL && action != 'q');
    free(nextPagePtr);

cleanup:
    if (status == 2)
        print(screen, screen->mainWindow, "parameters: <ticker>,T:<C(all) or P(ut),s:<min-strike>,S:<max-strike>,e:<earliest-expiry>,E:<latest-expiry>,X:<expired? Y(es) or N(o)>\n");

    freeTokens(tokens, nTokens);
    free(parameters);
    free(ticker);


    return FV_OK;
}

FunctionValue pioOptionsChainFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char type = 0;
    double minstrike = 0;
    double maxstrike = 0;
    double minpremium = 0;

    // Expiry date range
    Date date1 = {0};
    Date date2 = {0};

    int status = 0;

    char *input = NULL;
    char *ticker = NULL;
    char **tokens = NULL;
    int nTokens = 0;
    int action = 0;

    char *params = arg.charStarValue;

    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;
    if (parameters[0] == 0)
    {
        status = 2;
        goto cleanup;
    }

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    char *keys[] = {"", "T:", "s:", "S:", "e:", "E:", "v:", 0};
    tokens = splitStringByKeys(parameters, keys, ',', &nTokens);
    if (tokens == NULL)
    {
        status = 2;
        goto cleanup;
    }

    ticker = strdup(tokens[0]);
    type = tokens[1][strlen(keys[1])];
    minstrike = atof(tokens[2]+strlen(keys[2]));
    maxstrike = atof(tokens[3]+strlen(keys[3]));
    interpretDate(tokens[4]+strlen(keys[4]), &date1);
    interpretDate(tokens[5]+strlen(keys[5]), &date2);
    minpremium = atof(tokens[6]+strlen(keys[6]));

    // Call PIO
    char *nextPagePtr = NULL;
    print(screen, screen->mainWindow, "%s: %s $%.2lf - $%.2lf expiring %d-%02d-%02d - %d-%02d-%02d, premium >= $%.2lf\n", ticker, type == 'C' ? "calls" : "puts", minstrike, maxstrike, date1.year, date1.month, date1.day, date2.year, date2.month, date2.day, minpremium);
    do
    {
        polygonIoOptionsChain(screen, ticker, type, minstrike, maxstrike, date1, date2, minpremium, &nextPagePtr);
        if (nextPagePtr != NULL)
            action = continueOrQuit(screen, 50, false);
    } while (nextPagePtr != NULL && action != 'q');
    free(nextPagePtr);

cleanup:
    if (status == 2)
        print(screen, screen->mainWindow, "parameters: <ticker>,T:<C(all) or P(ut),s:<min-strike>,S:<max-strike>,e:<earliest-expiry>,E:<latest-expiry>,v:<min-value>\n");

    freeTokens(tokens, nTokens);
    free(parameters);
    free(ticker);

    return FV_OK;
}

FunctionValue pioPriceHistoryFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *input = NULL;
    char *ticker = NULL;
    int status = 0;
    char **tokens = NULL;
    int nTokens = 0;
    Date date1 = {0};
    Date date2 = {0};

    resetPromptPosition(screen, false);

    char *params = arg.charStarValue;

    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;
    if (parameters[0] == 0)
    {
        status = 2;
        goto cleanup;
    }

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    char *keys[] = {"", "", "", 0};
    tokens = splitStringByKeys(parameters, keys, ',', &nTokens);
    if (tokens == NULL)
    {
        status = 2;
        goto cleanup;
    }

    ticker = strdup(tokens[0]);
    interpretDate(tokens[1], &date1);
    interpretDate(tokens[2], &date2);

    // Call PIO
    PriceData data = {0};
    polygonIoPriceHistory(screen, ticker, date1, date2, &data);

cleanup:
    if (status == 2)
        print(screen, screen->mainWindow, "parameters: <ticker>,<startDate>,<endDate>\n");

    freeTokens(tokens, nTokens);
    free(parameters);
    free(ticker);

    return FV_OK;
}

FunctionValue pioVolatilityFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *input = NULL;
    char *ticker = NULL;
    int status = 0;
    char **tokens = NULL;
    int nTokens = 0;
    Date date1 = {0};
    Date date2 = {0};

    char *params = arg.charStarValue;

    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;
    if (parameters[0] == 0)
    {
        status = 2;
        goto cleanup;
    }

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    char *keys[] = {"", "", "", 0};
    tokens = splitStringByKeys(parameters, keys, ',', &nTokens);
    if (tokens == NULL)
    {
        status = 2;
        goto cleanup;
    }

    ticker = strdup(tokens[0]);
    interpretDate(tokens[1], &date1);
    interpretDate(tokens[2], &date2);

    // Call PIO
    polygonIoVolatility(screen, ticker, date1, date2, NULL);

cleanup:
    if (status == 2)
        print(screen, screen->mainWindow, "parameters: <ticker>,<startDate>,<endDate>\n");

    freeTokens(tokens, nTokens);
    free(parameters);
    free(ticker);

    return FV_OK;

}

FunctionValue pioLatestPriceFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *input = NULL;
    char *ticker = NULL;

    char *tkr = arg.charStarValue;

    if (tkr != NULL)
        ticker = strdup(tkr);
    else
        ticker = readInput(screen, screen->mainWindow, "  ticker: ", ON_READINPUT_ALL);
    if (!ticker)
        return FV_NOTOK;

    if (tkr == NULL && ticker[0] != 0)
        memorize(screen->userInput, ticker);

    if (ticker[0] == 0)
    {
        free(ticker);
        return FV_NOTOK;
    }

    // Call PIO
    polygonIoLatestPrice(screen, ticker, NULL, NULL, true);

    free(ticker);

    return FV_OK;
}

FunctionValue pioPreviousCloseFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *input = NULL;
    char *ticker = NULL;
    char type = 0;
    double strike = 0;
    // Expiry date
    int year = 0;
    int month = 0;
    int day = 0;

    char *tkr = arg.charStarValue;

    if (tkr != NULL)
        ticker = strdup(tkr);
    else
        ticker = readInput(screen, screen->mainWindow, "  ticker: ", ON_READINPUT_ALL);
    if (!ticker)
        return FV_NOTOK;

    if (tkr == NULL && ticker[0] != 0)
        memorize(screen->userInput, ticker);

    if (ticker[0] == 0)
    {
        print(screen, screen->mainWindow, "  Ticker symbol formats:\n");
        print(screen, screen->mainWindow, "  %15s : %s\n", "Stock", "sym");
        print(screen, screen->mainWindow, "  %15s   %5s %s\n", "", "where", "sym is the ticker symbol");
        print(screen, screen->mainWindow, "  %15s : %s\n", "Option", "O:symYYMMDDXdddddccc");
        print(screen, screen->mainWindow, "  %15s   %5s %s\n", "", "where", "sym is the ticker symbol");
        print(screen, screen->mainWindow, "  %15s   %5s %s\n", "", "", "YY is last two digits of expiry year");
        print(screen, screen->mainWindow, "  %15s   %5s %s\n", "", "", "MM and DD are expiry month and day");
        print(screen, screen->mainWindow, "  %15s   %5s %s\n", "", "", "X is either C for call or P for put");
        print(screen, screen->mainWindow, "  %15s   %5s %s\n", "", "", "ddddd is strike price dollars (up to 99999, i.e. $99,999)");
        print(screen, screen->mainWindow, "  %15s   %5s %s\n", "", "", "ccc is strike price cents times 10");
        print(screen, screen->mainWindow, "  %15s   %5s %s\n", "", "e.g.", "O:GME230317C00040000 is a GME $40.00 call expiring 17 March 2023");

        free(ticker);
        return FV_NOTOK;
    }

    // Call PIO
    polygonIoPreviousClose(screen, ticker);

    free(ticker);

    return FV_OK;
}

// FRED
FunctionValue fredSOFRFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    (void)arg;

    int res = fredSOFR(screen, NULL);

    return (FunctionValue)res;
}

// Questrade
FunctionValue questradeConnectionFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    (void)arg;

    int res = updateQuestradeAccessToken(screen);

    return (FunctionValue)res;
}


// What if?
FunctionValue optionsIncomeReinvestedFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *params = arg.charStarValue;
    int startingShares = 0;
    double weeklyProfitPerShare = 0;
    long tradingWeeksPerYear = 50;
    double taxRate = 0;
    double percentReinvested = 0;
    double sharePrice = 0;
    int years = 0;

    // Estimate call or put value from parameters
    char *parameters = NULL;
    if (params != NULL)
        parameters = strdup(params);
    else
        parameters = readInput(screen, screen->mainWindow, "  parameters: ", ON_READINPUT_ALL);
    if (!parameters)
        return FV_NOTOK;

    if (params == NULL && parameters[0] != 0)
        memorize(screen->userInput, parameters);

    int nParams = sscanf(parameters, "N:%d,D:%lf,T:%lf,R:%lf,P:%lf,Y:%d", &startingShares, &weeklyProfitPerShare, &taxRate, &percentReinvested, &sharePrice, &years);
    free(parameters);
    if (nParams != 6)
    {
        print(screen, screen->mainWindow, "parameters: N:<starting-number-of-shares>D:<$/week after fees>,T:<tax-rate-%%>,R:<%%-reinvested>, P:<underlying-share-price>,Y:<number-of-years>\n");
        return FV_NOTOK;
    }

    long nShares = startingShares;
    long nContracts = nShares / 100;
    double income = 0;
    double netIncome = 0;
    char amt[100] = {0};
    prepareForALotOfOutput(screen, years + 2);
    print(screen, screen->mainWindow, "Assumptions: ...\n");
    print(screen, screen->mainWindow, "%4s%15s%15s%15s\n", "Year",  "Shares",  "Contracts",  "Net income");    
    for (int i = 0; i < years; i++)
    {
        income = weeklyProfitPerShare * (double)100 * (double)nContracts * (double)tradingWeeksPerYear;
        netIncome = income * (1.0 - taxRate / 100.0);
        sprintf(amt, "$ %.0lf", netIncome);
        print(screen, screen->mainWindow, "%4d%15ld%15ld%15s\n", i+1, nShares, nContracts, amt);
        nShares += (long) floor((netIncome * percentReinvested / 100) / sharePrice);
        nContracts = nShares / 100;
    }

    return FV_OK;
}

FunctionValue optionsExerciseChanceFunction(ScreenState *screen, FunctionValue arg)
{
    if (screen == NULL)
        return (FunctionValue)ON_NO_SCREEN;

    char *input = NULL;
    char *ticker = NULL;

    char *tkr = arg.charStarValue;

    if (tkr != NULL)
        ticker = strdup(tkr);
    else
        ticker = readInput(screen, screen->mainWindow, "  ticker: ", ON_READINPUT_ALL);
    if (!ticker)
        return FV_NOTOK;

    if (tkr == NULL && ticker[0] != 0)
        memorize(screen->userInput, ticker);

    if (ticker[0] == 0)
    {
        free(ticker);
        return FV_NOTOK;
    }

    // Call PIO
    double close = 0;
    double openInterest = 0;
    char chance[50] = {0};
    OptionsData data = {0};

    int res = polygonIoLatestPrice(screen, ticker, NULL, &data, false);
    if (res == 0)
    {
        double strike = data.strike;
        int daysLeft = tradingDaysToExpiry(data.expiry);
        double underlyingClose = data.underlyingTickerData.close;
        double close = data.tickerData.close;
        double bookValue = data.type == CALL ? underlyingClose - strike : strike - underlyingClose;
        if (bookValue < 0)
            bookValue = 0;
        double timeValue = close - bookValue;
        double ratio = timeValue / close;
        double iv = 100.0 * data.impliedVolatility;
        long openInterest = data.openInterest;
        char assumptions[512] = {0};

        if (bookValue == 0)
            sprintf(chance, "%s", "extremely unlikely");
        else if (daysLeft > 5)
        {
            if (ratio < 0.25)
                sprintf(chance, "%s", "fairly unlikely");
            else
                sprintf(chance, "%s", "unlikely");
        }
        else if (daysLeft > 1)
        {
            if (ratio < 0.1)
                sprintf(chance, "%s", "fairly likely");
        }
        else
            sprintf(chance, "%s", "almost certain");

        print(screen, screen->mainWindow, "%35s : %s\n", "Ticker", data.ticker);
        print(screen, screen->mainWindow, "%35s : %s (%s the money)\n", "Type", data.type == CALL ? "call" : "put", bookValue > 0 ? "in" : "out of");
        print(screen, screen->mainWindow, "%35s : %d-%02d-%02d (in %d trading days)\n", "Expiry", data.expiry.year, data.expiry.month, data.expiry.day, daysLeft);
        print(screen, screen->mainWindow, "%35s : $%.2lf\n", "Strike price", strike);
        print(screen, screen->mainWindow, "%35s : %s\n", "Underlying ticker", data.underlyingTickerData.ticker);
        print(screen, screen->mainWindow, "%35s : $%.2lf\n", "Underlying price", underlyingClose);
        print(screen, screen->mainWindow, "%35s : $%.2lf\n", "Total value (last close)", close);
        print(screen, screen->mainWindow, "%35s : $%.2lf\n", "Book value", bookValue);
        print(screen, screen->mainWindow, "%35s : $%.2lf\n", "Time value", timeValue);
        print(screen, screen->mainWindow, "%35s : %.1lf%%\n", "(Time value)/(Total value)", ratio * 100.0);
        print(screen, screen->mainWindow, "%35s : %ld\n", "Open interest", openInterest);
        print(screen, screen->mainWindow, "%35s : %.1lf%%\n", "Implied volatility", iv);
        sprintf(assumptions, "%s remains at $%.2lf through expiry; no dividends", data.underlyingTickerData.ticker, underlyingClose);
        print(screen, screen->mainWindow, "%35s : %s\n", "Assumptions", assumptions);
        print(screen, screen->mainWindow, "%35s : %s\n", "Excercise chance now", chance);
        print(screen, screen->mainWindow, "%35s : %s\n", "Excercise chance at expiry", bookValue > 0 ? "certain" : "unlikely");
    }
    else
    {
        print(screen, screen->mainWindow, "Unable to get contract details from Polygon.IO. Calculating with american_option...");
        print(screen, screen->mainWindow, "<TODO>\n");
    }

    free(ticker);
    return FV_OK;
}

