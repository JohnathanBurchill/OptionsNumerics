/*
    Options Numerics: on_data.h

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

#ifndef _ON_DATA_H
#define _ON_DATA_H

#include "on_optionstiming.h"

typedef enum optionType
{
    CALL = 1 << 0,
    PUT = 1 << 1
} OptionType;

typedef struct tickerQuote
{
    char *ticker;
    double lastUpdatedSeconds;

    double bid;
    long bidsize;
    double ask;
    long asksize;

    double midpoint;

} TickerQuote;

typedef struct tickerData
{
    char *ticker;
    double lastUpdatedSeconds;

    double volume;
    double vwap;
    double open;
    double high;
    double low;
    double close;
    int nTransactions;

    TickerQuote quote;

} TickerData;

typedef struct optionsData
{
    char *ticker;
    TickerData tickerData;
    double lastUpdatedSeconds;

    TickerData underlyingTickerData;

    OptionType type;
    double strike;
    Date expiry;

    double theta;
    double vega;
    double delta;
    double gamma;

    TickerQuote quote;
    long openInterest;
    double impliedVolatility;

} OptionsData;

#endif // _ON_DATA_H
