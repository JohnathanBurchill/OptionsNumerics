/*
    Options Numerics: on_optionsmodels.c

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

#include "on_optionsmodels.h"
#include "on_data.h"
#include "on_optionstiming.h"
#include "on_screen_io.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ncurses.h>

extern WINDOW *mainWindow;
extern WINDOW *statusWindow;

// Black-Scholes European call
// From ChatGPT Jan 2023
double cdf(double x)
{
    return 0.5 * (1.0 + erf(x / sqrt(2.0)));
}

double d1(double S, double K, double r, double sigma, double t)
{
    return (log(S / K) + (r + sigma * sigma / 2.0) * t) / (sigma * sqrt(t));
}

double d2(double d1Val, double sigma, double t)
{
    return d1Val - sigma * sqrt(t);
}

double blackscholes_option_value(Option opt, OptionType type)
{
    double d_1 = d1(opt.S, opt.K, opt.r, opt.v, opt.T);
    double d_2 = d2(d_1, opt.v, opt.T);
    double price = 0.0;
    if (type == CALL)
        price = opt.S * cdf(d_1) - opt.K * exp(-opt.r * opt.T) * cdf(d_2);
    else
        price = opt.K * exp(-opt.r * opt.T) * cdf(-d_2) - opt.S * cdf(-d_1);

    return price;
}

// ChatGPT 14 Jan 2023
// Binomial call or put
double binomial_option_value(Option opt, OptionType type)
{
    double dt = opt.T / BINOMIAL_N_STEPS;
    double u = exp(opt.v * sqrt(dt));
    double d = 1 / u;
    double p_up = (exp(opt.r * dt) - d) / (u - d);
    double p_down = 1 - p_up;
    double price = 0.0;

    // CRR method from Wikipedia
    double q = opt.q; // Dividend yield
    double p0 = (u * exp(-q * dt) - exp(-opt.r * dt)) / (u * u - 1);
    double p1 = exp(-opt.r * dt) - p0;

    double *p = malloc((BINOMIAL_N_STEPS + 1) * sizeof *p);
    if (p == NULL)
        return nan(""); // Memory

    for (int i = 0; i <= BINOMIAL_N_STEPS; i++)
    {
        p[i] = opt.S * pow(u, 2 * i - BINOMIAL_N_STEPS) - opt.K;
        if (type == PUT)
            p[i] *= -1.0;

        if (p[i] < 0.0)
            p[i] = 0.0;
    }

    double exercise = 0.0;
    for (int j = BINOMIAL_N_STEPS - 1; j >= 0; j--)
    {
        for (int i = 0; i <= j; i++)
        {
            p[i] = p0 * p[i + 1] + p1 * p[i];
            exercise = opt.S * pow(u, 2 * i - j) - opt.K;
            if (type == PUT)
                exercise *= -1.0;

            if (p[i] < exercise)
                p[i] = exercise;
        }
    }
    price = p[0];

    free(p);

    return price;
}

int binomial_option_implied_volatility(Option opt, OptionType type, double actualPrice, double *impliedVolatility)
{
    if (impliedVolatility == NULL)
        return 1;

    Option searchOpt = opt;
    opt.v = 1.0;

    double theoreticalPrice = binomial_option_value(searchOpt, type);
    double lastTheoreticalPrice = -1.0;

    int iterations = 0;
    double deltaV = 0.1;
    double deltaVDecay = 0.9;

    double priceDiff = fabs(theoreticalPrice - actualPrice);
    while (iterations < IV_MAX_ITERATIONS && priceDiff > IV_MAX_PRICE_DIFFERENCE && fabs(theoreticalPrice - lastTheoreticalPrice) > IV_MIN_PRICE_CHANGE)
    {
        if (actualPrice > theoreticalPrice)
            deltaV = deltaVDecay * fabs(deltaV);
        else
            deltaV = -deltaVDecay * fabs(deltaV);
        
        if (deltaV < 0 && -deltaV >= searchOpt.v)
            searchOpt.v *= 0.9;
        else
            searchOpt.v += deltaV;

        lastTheoreticalPrice = theoreticalPrice;
        theoreticalPrice = binomial_option_value(searchOpt, type);
        priceDiff = fabs(theoreticalPrice - actualPrice);
        iterations++;
    }
    if (iterations == IV_MAX_ITERATIONS)
    {
        print(mainWindow, "No solution after %d iterations.\n", IV_MAX_ITERATIONS);
        return 2;
    }
    // No solution - market bid was less than book value?
    if (fabs(theoreticalPrice - lastTheoreticalPrice) < IV_MIN_PRICE_CHANGE)
        *impliedVolatility = nan("");
    else
        *impliedVolatility = searchOpt.v;
    return 0;
}

double option_geeks(Option opt, OptionType type, char *geek, double (*optionValueFunction)(Option, OptionType))
{
    if (geek == NULL)
        return nan("");

    Option derivOpt = opt;

    double value0 = optionValueFunction(derivOpt, type);
    double valueminus = 0;
    double valueplus = 0;
    double dx = 0;
    double factor = 1.0;

    if (strcasecmp("d$dt", geek) == 0)
    {
        dx = 0.5 * 1.0 / OPTIONS_TRADING_DAYS_PER_YEAR;
        factor = 1.0 / OPTIONS_TRADING_DAYS_PER_YEAR;
        derivOpt.T += dx;
        valueminus = optionValueFunction(derivOpt, type);
        derivOpt.T -= 2*dx;
        valueplus = optionValueFunction(derivOpt, type);
    }
    else if (strcasecmp("d$dV", geek) == 0)
    {
        dx = 0.001;
        factor = 1.0 / 100.0; // per %
        derivOpt.v += dx;
        valueplus = optionValueFunction(derivOpt, type);
        derivOpt.v -= 2*dx;
        valueminus = optionValueFunction(derivOpt, type);
    }
    else if (strcasecmp("d$dP", geek) == 0)
    {
        dx = fmax(0.01, fmin(0.1, 0.1 * opt.S));
        derivOpt.S += dx;
        valueplus = optionValueFunction(derivOpt, type);        
        derivOpt.S -= 2*dx;
        valueminus = optionValueFunction(derivOpt, type);        
    }
    else if (strcasecmp("d2$dP2", geek) == 0)
    {
        value0 = binomial_option_geeks(derivOpt, type, "d$dP");
        dx = fmax(0.25, fmin(1.0, 0.5 * opt.S));
        derivOpt.S += dx;
        valueplus = binomial_option_geeks(derivOpt, type, "d$dP");
        derivOpt.S -= 2*dx;
        valueminus = binomial_option_geeks(derivOpt, type, "d$dP");

    }
    else 
        return nan("");

    double deriv = factor * ((valueplus - value0)/dx + (value0 - valueminus)/dx) / 2.0;

    return deriv;
}

double binomial_option_geeks(Option opt, OptionType type, char *geek)
{
    return option_geeks(opt, type, geek, binomial_option_value);
}

double black_scholes_option_geeks(Option opt, OptionType type, char *geek)
{
    return option_geeks(opt, type, geek, blackscholes_option_value);
}
