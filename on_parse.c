/*
    Options Numerics: on_parse.c

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

#include "on_parse.h"
#include "on_optionstiming.h"

#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

int dateRange(char *input, Date *date1, Date *date2)
{

    // Adapted from regex.h example by ChatGPT 17 Jan 2023
    char d1[100] = {0};
    char d2[100] = {0};

    regex_t reg;
    regmatch_t regmatch[3];
    int status;

    // Compile the regular expression
    regcomp(&reg, "([^,]+),([^,]+)", REG_EXTENDED);

    // Execute the regular expression
    status = regexec(&reg, input, 3, regmatch, 0);

    if (status == 0) {
        // Extract the first date
        strncpy(d1, input + regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
        // Extract the second date
        strncpy(d2, input + regmatch[2].rm_so, regmatch[2].rm_eo - regmatch[2].rm_so);
        // Add null-terminating character to the extracted strings
        d1[regmatch[1].rm_eo - regmatch[1].rm_so] = '\0';
        d2[regmatch[2].rm_eo - regmatch[2].rm_so] = '\0';

        status = interpretDate(d1, date1);
        if (status != 0)
            goto cleanup;
        status = interpretDate(d2, date2);
    }

cleanup:
    regfree(&reg);

    return status;
}

int interpretDate(char *dateString, Date *date)
{
    if (dateString == NULL || strlen(dateString) == 0 || date == NULL)
        return 1;

    int status = 0;

    time_t today = time(NULL);
    struct tm *tmToday = localtime(&today);
    int year = 0;
    int month = 0;
    int day = 0;
    
    unsigned char timeUnit = 0;
    int nUnits = 0;
    int res = 0;

    if (strcasecmp("today", dateString) == 0)
    {
        year = tmToday->tm_year + 1900;
        month = tmToday->tm_mon + 1;
        day = tmToday->tm_mday;
    }
    else if (dateString[0] == '+' || dateString[0] == '-')
    {
        // relative date
        nUnits = atoi(dateString);
        timeUnit = dateString[strlen(dateString) - 1];
        if (timeUnit == 'F')
        {
            date->year = tmToday->tm_year + 1900;
            date->month = tmToday->tm_mon + 1;
            date->day = tmToday->tm_mday;
            return advanceN3rdFridaysOfTheMonth(date, nUnits);
        }
        switch(timeUnit)
        {
            case 'd':
                tmToday->tm_mday = tmToday->tm_mday + nUnits;
                break;
            case 'f': // # of Fridays
                tmToday->tm_mday = tmToday->tm_mday + nUnits * 7;
                if (nUnits > 0)
                    tmToday->tm_mday = tmToday->tm_mday - 7;
                break;
            case 'w':
                tmToday->tm_mday = tmToday->tm_mday + nUnits * 7;
                break;
            case 'm':
                tmToday->tm_mon = tmToday->tm_mon + nUnits;
                break;
            case 'y':
                tmToday->tm_year = tmToday->tm_year + nUnits;
                break;            
            default:
                // Error. Sets today's date with nonzero return
                status = 4;
        }
        (void)mktime(tmToday);
        year = tmToday->tm_year + 1900;
        month = tmToday->tm_mon + 1;
        day = tmToday->tm_mday;
    }
    else if (sscanf(dateString, "%d-%d-%d", &year, &month, &day) != 3)
    {
        year = 0;
        month = 0;
        day = 0;
        status = 2;
    }

    date->year = year;
    date->month = month;
    date->day = day;

    if (timeUnit == 'f')
        makeSureItsAFriday(date);

    return status;

}


// Checks if string starts with key and is longer than key
bool expectedKey(char *string, char *key)
{
    bool ok = true;

    ok &= strncmp(key, string, strlen(key)) == 0;
    ok &= strlen(string) > strlen(key);

    return ok;
}

bool expectedKeys(char **tokens, char **keys)
{
    int t = 0;
    bool expected = true;
    while (keys[t] != NULL && tokens[t] != NULL)
    {
        expected &= expectedKey(tokens[t], keys[t]);
        t++;
    }

    return expected;

}
// Caller must free memory at returned pointer
char **splitString(char *string, char delimiter, int *nWords)
{
    if (string == NULL)
        return (char **)NULL;

    char *workingString = strdup(string);
    if (workingString == NULL)
        return (char **)NULL;

    // Count number of delimeters
    int n = 1;
    char *p = workingString;
    while (*p != 0)
    {
        if (*p == delimiter)
        {
            *p = '\0';
            n++;
        }
        p++;
    }

    char **words = malloc(n * sizeof(char*));
    if (words == NULL)
        return (char **)NULL;

    p = workingString;
    for (int i = 0; i < n; i++)
    {
        words[i] = strdup(p);
        p += strlen(p) + 1;
    }
    free(workingString);

    if (nWords != NULL)
        *nWords = n;

    return words;
}

// Caller frees memory at returned pointer
char **splitStringByKeys(char *string, char **keys, char delimeter, int *nTokens)
{
    if (string == NULL || keys == NULL || nTokens == NULL)
        return (char **)NULL;

    int nTokensExpected = 1;
    while (keys[nTokensExpected] != 0)
        nTokensExpected++;

    int n = 0;

    char **tokens = splitString(string, delimeter, &n);
    if (tokens == NULL)
        return (char **)NULL;

    if ((n != nTokensExpected) || !expectedKeys(tokens, keys))
    {
        free(tokens);
        return (char **)NULL;
    }

    *nTokens = n;

    return tokens;

}

void freeTokens(char **tokens, int nTokens)
{
    if (tokens == NULL)
        return;

    for (int i = 0; i < nTokens; i++)
        free(tokens[i]);

    free(tokens);

    return;
}