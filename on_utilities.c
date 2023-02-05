/*
    Options Numerics: on_utilities.c

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

#include "on_utilities.h"
#include "on_status.h"
#include "on_info.h"
#include "on_config.h"
#include "on_screen_io.h"

#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include <ncurses.h>

char continueOrQuit(ScreenState *screen, int maxLineLength, bool pageBreak)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    char template[20] = {0};
    char action = 0;
    sprintf(template, "%%%ds", maxLineLength + (int)strlen(ON_READING_CUE));
    char prompt[256] = {0};
    snprintf(prompt, 256, template, "[any key but q for next page...]");

    char *a = readInput(screen, screen->mainWindow, prompt, ON_READINPUT_ALL | ON_READINPUT_HIDDEN | ON_READINPUT_ONESHOT);
    if (a != NULL)
    {
        action = a[0];
        free(a);
    }
    wprintw(screen->mainWindow, "\r");
    wprintw(screen->mainWindow, template, "");
    if (pageBreak)
        print(screen, screen->mainWindow, "\n");
    else
        wprintw(screen->mainWindow, "\r");

    return action;
}

void searchOutputHighlight(ScreenState *screen, char *forThisString)
{
    if (screen == NULL || forThisString == NULL)
        return;

    int sLen = strlen(forThisString);
    if (sLen > 0)
    {
        snprintf(screen->searchText, ON_CMD_LENGTH, "%s", forThisString);
        screen->searchText[sLen < ON_CMD_LENGTH ? sLen : ON_CMD_LENGTH-1] = '\0';
        screen->lastSearchResultLine = screen->mainWindowLines - 1;
    }
    else if (strlen(screen->searchText) > 0)
        forThisString = screen->searchText;

    if (strlen(forThisString) == 0)
        return;

    char lineBuf[ON_BUFFERED_LINE_LENGTH+1] = {0};

    int status = 0;
    int lineLength = 0;
    size_t searchLen = strlen(forThisString);
    int i = screen->lastSearchResultLine - 1;

    for (; i >= 0; i--)
    {
        mvwinnstr(screen->mainWindow, i, 0, lineBuf, ON_BUFFERED_LINE_LENGTH);
        // Do not print traling spaces
        for (lineLength = ON_BUFFERED_LINE_LENGTH; lineLength > 0 && lineBuf[lineLength-1] == ' '; lineLength--);
        lineBuf[lineLength] = '\0';
        for (int c = 0; c < lineLength - 1 - (int)searchLen; c++)
        {
            if (strlen(lineBuf) > 0 && strncmp(forThisString, lineBuf + c, searchLen) == 0)
            {
                wattron(screen->statusWindow, A_REVERSE);
                mvwprintw(screen->statusWindow, 0, 0, "Options Numerics; found at line %d, column %d:", i + 1, c + 1);
                wattroff(screen->statusWindow, A_REVERSE);
                wprintw(screen->statusWindow, " %s", lineBuf + c);
                wclrtoeol(screen->statusWindow);
                wattron(screen->statusWindow, A_REVERSE);
                wrefresh(screen->statusWindow);

                screen->mainWindowTopLine = i;
                wmove(screen->mainWindow, i, c);
                wattron(screen->mainWindow, A_BOLD | A_UNDERLINE);
                waddstr(screen->mainWindow, forThisString);
                wattroff(screen->mainWindow, A_BOLD | A_UNDERLINE);
                screen->lastSearchResultLine = i;
                return;
            }
        }
    }
    wattron(screen->statusWindow, A_REVERSE);
    mvwprintw(screen->statusWindow, 0, 0, "Text not found");
    wclrtoeol(screen->statusWindow);
    wrefresh(screen->statusWindow);    

    return;

}

// Caller must free memory of returned char *
char *nanoSecondsAsStringMustFree(double nanoseconds)
{
    char d[50] = {0};

    double seconds = nanoseconds / 1e9;
    time_t secs = (time_t)floor(seconds);
    double subSecs = floor(1e9 * (seconds - (double)secs));

    struct tm *t = localtime(&secs);

    struct timezone tz = {0};
    int r = gettimeofday(NULL, &tz);
    int utchrdiff = 0;
    if (r == 0)
        utchrdiff = -(int)floor((double)tz.tz_minuteswest / 60.0);
    else
        utchrdiff = -25;

    if (utchrdiff != -25)
        snprintf(d, 50, "%d-%02d-%02d %02d:%02d:%02d.%09.0lf (UTC%+02d)", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, subSecs, utchrdiff);
    else
        snprintf(d, 50, "%d-%02d-%02d %02d:%02d:%02d.%09.0lf (UTC+?)", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, subSecs);

    return strdup(d);
}
