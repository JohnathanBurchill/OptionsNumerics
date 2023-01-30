/*
    Options Numerics: on_statistics.c

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

#include "on_statistics.h"
#include "on_optionstiming.h"

#include <stdlib.h>
#include <math.h>

// From ChatGPT 15 January 2023
double calculate_stddev(double *data, int n)
{
    if (data == NULL || n <= 0)
        return nan("");

    double mean = 0;
    double sum = 0;
    double stddev = 0;
    int i;

    // Calculate the mean value
    for (i = 0; i < n; i++) {
        mean += data[i];
    }
    mean /= n;

    // Calculate the standard deviation
    for (i = 0; i < n; i++) {
        sum += pow(data[i] - mean, 2);
    }
    stddev = sqrt(sum / (n + 1));

    return stddev;
}

double calculate_volatility(double *closes, int nCloses)
{
    if (closes == NULL || nCloses < 3)
        return nan("");

    // Daily log returns
    double *logreturns = malloc((nCloses - 1) * sizeof *logreturns);
    if (logreturns == NULL)
        return nan("");

    for (int i = 1; i < nCloses; i++)
        logreturns[i-1] = log(closes[i] / closes[i-1]);

    double stddev = calculate_stddev(logreturns, nCloses - 1);
    return stddev * sqrt(OPTIONS_TRADING_DAYS_PER_YEAR);

}
