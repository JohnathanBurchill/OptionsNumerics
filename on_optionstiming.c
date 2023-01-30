/*
    Options Numerics: on_optionstiming.c

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

#include "on_optionstiming.h"

#include <stdio.h>
#include <time.h>

// Function created by ChatGPT from
// "Can you show this code as a function called daysToTrade that takes the year, month and day of expiry as input?"
// on 11 Jan 2023
int tradingDaysToExpiry(Date date)
{
    int trading_days = 0;
    time_t now;
    struct tm expiry_tm;
    struct tm *now_tm;

    // Set the expiry date using the input year, month, and day
    expiry_tm.tm_year = date.year - 1900;
    expiry_tm.tm_mon = date.month - 1;
    expiry_tm.tm_mday = date.day;
    expiry_tm.tm_hour = 0;
    expiry_tm.tm_min = 0;
    expiry_tm.tm_sec = 0;
    // First try by ChatGPT did not initialize tm_yday. It corrected this when I pointed it out.
    expiry_tm.tm_isdst = -1;
    mktime(&expiry_tm);

    time(&now);
    now_tm = localtime(&now);

    // go through each date between now and expiry,
    // counting the trading days
    while (now_tm->tm_year < expiry_tm.tm_year ||
           (now_tm->tm_year == expiry_tm.tm_year &&
            now_tm->tm_yday <= expiry_tm.tm_yday))
    {
        // if not a weekend
        if (now_tm->tm_wday != 0 && now_tm->tm_wday != 6)
        {
            trading_days++;
        }
        now_tm->tm_mday++;
        mktime(now_tm);
    }
    return trading_days;
}

// If date is not a Friday, advances date to the next Friday
// Returns the date difference in days
int makeSureItsAFriday(Date *date)
{
    struct tm d = {0};
    d.tm_year = date->year - 1900;
    d.tm_mon = date->month - 1;
    d.tm_mday = date->day;

    int days = 0;
    while (d.tm_wday != 5)
    {
        d.tm_mday += 1;
        days++;
        mktime(&d);
    }
    date->year = d.tm_year + 1900;
    date->month = d.tm_mon + 1;
    date->day = d.tm_mday;

    return days;
}

int advanceN3rdFridaysOfTheMonth(Date *date, int n)
{
    if (n == 0)
        return 1;

    struct tm day = {0};
    day.tm_year = date->year - 1900;
    day.tm_mon = date->month - 1;
    day.tm_mday = date->day;

    struct tm firstOfTheMonth = {0};
    firstOfTheMonth.tm_year = date->year - 1900;
    firstOfTheMonth.tm_mon = date->month - 1;
    firstOfTheMonth.tm_mday = 0;

    int nFridays = 0;
    do
    {
        firstOfTheMonth.tm_mday = firstOfTheMonth.tm_mday + 1;
        (void)timegm(&firstOfTheMonth);
        if (firstOfTheMonth.tm_wday == 5)
            nFridays++;
    } while (nFridays < 3);

    int nMonths = n;
    if (nMonths < 0 && day.tm_mday > firstOfTheMonth.tm_mday)
        nMonths++;
    else if (nMonths > 0 && day.tm_mday < firstOfTheMonth.tm_mday)
        nMonths--;

    firstOfTheMonth.tm_mon = firstOfTheMonth.tm_mon + nMonths;
    firstOfTheMonth.tm_mday = 0;
    nFridays = 0;
    do
    {
        firstOfTheMonth.tm_mday = firstOfTheMonth.tm_mday + 1;
        (void)mktime(&firstOfTheMonth);
        if (firstOfTheMonth.tm_wday == 5)
            nFridays++;
    } while (nFridays < 3);

    date->year = firstOfTheMonth.tm_year + 1900;
    date->month = firstOfTheMonth.tm_mon + 1;
    date->day = firstOfTheMonth.tm_mday;

    return 0;
}
