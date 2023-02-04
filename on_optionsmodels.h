/*
    Options Numerics: on_optionsmodels.h

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

#ifndef _ON_OPTIONSMODELS_H
#define _ON_OPTIONSMODELS_H

#include "on_data.h"

typedef struct {
    double S; // Security price
    double K; // Strike
    double r; // risk-free interest rate (fraction)
    double q; // Annualized dividend yield (fraction)
    double v; // Annualized volatility (standard deviation of daily close times sqrt(number of trading days of the interval))
    double T; // Trading years until expiration (assume 251 trading days per year)
} Option;

// Black-Scholes
double cdf(double x);
double d1(double S, double K, double r, double sigma, double t);
double d2(double d1Val, double sigma, double t);
double blackscholes_option_value(Option opt, OptionType type);

// Binomial no dividend

#define BINOMIAL_N_STEPS 300
#define IV_MAX_ITERATIONS 100
#define IV_MAX_PRICE_DIFFERENCE 0.000001
#define IV_MIN_PRICE_CHANGE 0.000001

double binomial_option_value(Option opt, OptionType type);
double option_geeks(Option opt, OptionType type, char *geek, double (*optionValueFunction)(Option, OptionType));
double binomial_option_geeks(Option opt, OptionType type, char *geek);
double black_scholes_option_geeks(Option opt, OptionType type, char *geek);
int binomial_option_implied_volatility(Option opt, OptionType type, double actualPrice, double *impliedVolatility);

#endif // _ON_OPTIONSMODELS_H
