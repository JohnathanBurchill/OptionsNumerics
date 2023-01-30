/*
    Options Numerics: on_calculate.c

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

#include "on_calculate.h"
#include "on_optionstiming.h"
#include "on_screen_io.h"

static double result = 0.0;

#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#include <ncurses.h>

extern WINDOW *mainWindow;
extern WINDOW *statusWindow;

void setResult(double newResult)
{
    result = newResult;
    return;
}

double getResult()
{
    return result;
}

void calculate(char *expression)
{
    if (expression == NULL)
        return;

    if (strcasecmp("result", expression) == 0)
    {
        print(mainWindow, "%lg\n", result);
        return;
    }

    char operator[32] = {0};
    char operand1Expression[100] = {0};
    double operand1 = 0;
    char operand2Expression[100] = {0};
    double operand2 = 0;

    regex_t reg;
    regmatch_t regmatch[4];
    int status;

    // operator
    regcomp(&reg, "([^ ]+ +)*([^ ]+) +([^ ]+)", REG_EXTENDED);

    // Execute the regular expression
    status = regexec(&reg, expression, 4, regmatch, 0);
    int nmatches = 0;
    for (int i = 1; i < 4; i++)
        if (regmatch[i].rm_eo > regmatch[i].rm_so)
            nmatches++;

    if (status == 0) {
        if (nmatches < 2 || nmatches > 3)
            goto cleanup;

        if (nmatches == 2)
        {
            // Get operand 1 from result of previous calculation
            operand1 = result;
        }
        else
        {
            strncpy(operand1Expression, expression + regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
            // Trim whitespace from end (https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way)
            char *lastChar = operand1Expression + strlen(operand1Expression) - 1;
            while(lastChar > operand1Expression && isspace((unsigned char)*lastChar))
                lastChar--;
            lastChar[1] = '\0';
            operand1 = atof(operand1Expression);
        }
        strncpy(operator, expression + regmatch[2].rm_so, regmatch[2].rm_eo - regmatch[2].rm_so);
        strncpy(operand2Expression, expression + regmatch[3].rm_so, regmatch[3].rm_eo - regmatch[3].rm_so);
        operand2 = atof(operand2Expression);

        if (strlen(operator) != 1)
            goto cleanup;

        switch (operator[0])
        {
            case '+':
                result = operand1 + operand2;
                break;
            case '-':
                result = operand1 - operand2;
                break;
            case '*':
                result = operand1 * operand2;
                break;
            case '/':
                result = operand1 / operand2;
                break;
            case '^':
                result = pow(operand1, operand2);
                break;
            case '|':
                result = (double)((int)operand1 | (int)operand2);
                break;
            case '&':
                result = (double)((int)operand1 & (int)operand2);
                break;
            case '=':
                if (strcasecmp("result", operand1Expression) == 0)
                    result = operand2;
                goto cleanup;
                break;
            default:
                goto cleanup;
        }
        print(mainWindow, "%lg\n", result);
    }

cleanup:
    regfree(&reg);

    return;

}

double timeValue(double amount, double annualRatePercent, Date date1, Date date2)
{
    // TODO fix historical dates
    wprintw(mainWindow, "%d-%02d-%02d, %d-%02d-%02d\n", date1.year, date1.month, date1.day, date2.year, date2.month, date2.day);
    struct tm d1 = {0};
    d1.tm_year = date1.year - 1900;
    d1.tm_mon = date1.month - 1;
    d1.tm_mday = date1.day;

    struct tm d2 = {0};
    d2.tm_year = date2.year - 1900;
    d2.tm_mon = date2.month - 1;
    d2.tm_mday = date2.day;

    time_t t1 = timegm(&d1);
    time_t t2 = timegm(&d2);
    time_t ta = -200L * 365L * 86400L;
    struct tm *d = gmtime(&ta);
    time_t tb = timegm(d);
    print(mainWindow, "tb: %ld\n", tb);
    print(mainWindow, "From gmtime: %d-%02d-%02d\n", d->tm_year+1900, d->tm_mon+1, d->tm_mday);
    print(mainWindow, "%ld %ld\n", timegm(&d1), timelocal(&d1));
    print(mainWindow, "t1: %ld, t2: %ld\n", t1, t2);
    double seconds = (double)t2 - (double)t1;
    double days = seconds / 86400.0; // Ignore leap seconds
    double years = days / 365.25; // Close enough
    double power = pow(1 + annualRatePercent / 100.0, years);
    double value = amount * power;

    print(mainWindow, "power: %lf\n", power);
    return value;

}
