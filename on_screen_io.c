/*
    Options Numerics: on_screen_io.c

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

#include "on_screen_io.h"
#include "on_config.h"
#include "on_commands.h"
#include "on_utilities.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include <ncurses.h>

extern WINDOW *mainWindow;
extern WINDOW *statusWindow;

extern long mainWindowLines;
extern long mainWindowTopLine;

extern int statusHeight;
extern int mainWindowViewHeight;

extern char cmd[ON_CMD_LENGTH];

extern volatile sig_atomic_t running;

extern char searchText[ON_CMD_LENGTH];

void prepareForALotOfOutput(long nLines)
{
    if (mainWindowLines >= ON_BUFFERED_LINES - nLines)
    {
        long freeLines = ON_BUFFERED_LINES - mainWindowLines;
        long shift = nLines - freeLines;
        int y = 0, x = 0;
        getyx(mainWindow, y, x);
        wscrl(mainWindow, shift);
        wmove(mainWindow, y - shift, x);
        mainWindowLines -= shift;
    }

    return;
}

int print(WINDOW *window, const char *fmt, ...)
{
    int y0 = 0, x0 = 0;
    int y1 = 0, x1 = 0;
    getyx(window, y0, x0);

    va_list args;
    va_start(args, fmt);
    int res = vw_printw(window, fmt, args);
    va_end(args);

    getyx(window, y1, x1);
    if (y1 > y0)
    {
        if (mainWindowLines < ON_BUFFERED_LINES)
            mainWindowLines += y1 - y0;
        else
        {
            wscrl(window, y1 - y0);
            wmove(window, y0, x1);
        }
    }

    return res;
}

int mvprint(WINDOW *window, int row, int col, const char *fmt, ...)
{
    int y1 = 0, x1 = 0;

    va_list args;
    va_start(args, fmt);
    wmove(window, row, col);
    int res = vw_printw(window, fmt, args);
    va_end(args);

    getyx(window, y1, x1);
    if (y1 > row)
    {
        if (mainWindowLines < ON_BUFFERED_LINES)
            mainWindowLines += y1 - row;
        else
        {
            wscrl(window, y1 - row);
            wmove(window, row, x1);
        }
    }

    return res;
}

void resetPromptPosition(bool toBottom)
{
    int y = 0, x = 0;
    getyx(mainWindow, y, x);
    if (y - mainWindowTopLine > mainWindowViewHeight - 1 || toBottom)
        mainWindowTopLine = mainWindowLines - (mainWindowViewHeight);
    if (mainWindowTopLine < 0)
        mainWindowTopLine = 0;

    return;
}

char *readInput(WINDOW *win, char *prompt, int flags)
{
    int cury = 0;
    int curx = 0;

    int key = 0;
    
    int inputLength = 0;
    int promptLength = (int)strlen(prompt);

    const char *recalled = NULL;

    print(win, "%s", prompt);
    if (ON_READINPUT_SEARCH & flags)
        wattroff(statusWindow, A_REVERSE);

    wclrtoeol(win);
    if (!(ON_READINPUT_SEARCH & flags))
        resetPromptPosition(false);

    cmd[0] = '\0';

    bool canScroll = false;
    double scrollRate = ON_SCROLL_RATE;
    double deltaScrollRate = 2;
    double maxScrollRate = LINES - 2;
    bool scrolling = false;
    bool scrollPrevious = false;
    bool scrollNext = false;
    int previousScroll = -1;
    int startLine = 0;
    int stopLine = 0;

    bool statusLineMsg = false;

    while (running)
    {

        canScroll = (((ON_READINPUT_SCROLL & flags) || (ON_READINPUT_ALL & flags)) && mainWindowLines > LINES);

        if (!statusLineMsg && win != statusWindow && ((ON_READINPUT_STATUS_WINDOW & flags) || (ON_READINPUT_ALL & flags)))
        {
            startLine = mainWindowTopLine + 1;
            if (mainWindowLines - mainWindowTopLine > mainWindowViewHeight)
                stopLine = mainWindowTopLine + mainWindowViewHeight;
            else
                stopLine = mainWindowLines;
            mvwprintw(statusWindow, 0, 0, "Options Numerics; showing lines %d - %d of %ld", startLine, stopLine, mainWindowLines);
            if (startLine > 1)
                wprintw(statusWindow, " <output above>");
            wclrtoeol(statusWindow);
            wrefresh(statusWindow);
        }
        if (ON_READINPUT_SEARCH & flags)
            prefresh(mainWindow, mainWindowTopLine, 0, 1, 0, LINES - 1, COLS - 1);
        if (is_pad(win))
            prefresh(win, mainWindowTopLine, 0, 1, 0, LINES - 1, COLS - 1);
        else
            wrefresh(win);

        key = wgetch(win);
        if (key == ('C' & 0x1f) || key == ('V' & 0x1f))
            continue;
        else if (key == KEY_RESIZE)
        {
            mainWindowViewHeight = LINES - statusHeight;
            continue;
        }
        else if (key == ERR && !scrolling)
        {
            scrollRate = ON_SCROLL_RATE;
            previousScroll = -1;
            usleep(250);
            continue;
        }
        // Find
        else if ((key == ('F' & 0x1f)) && (win != statusWindow))
        {
            wmove(statusWindow, 0, 0);
            char * cmdold = strdup(cmd);
            char * ans = readInput(statusWindow, "Search for: ", ON_READINPUT_ALL | ON_READINPUT_SEARCH);
            memorize(ans);
            free(ans);
            size_t l = strlen(cmdold);
            memcpy(cmd, cmdold, l < ON_CMD_LENGTH - 1 ? l + 1 : ON_CMD_LENGTH);
            free(cmdold);
            statusLineMsg = true;
            continue;
        }
        // Find again
        else if ((key == ('R' & 0x1f)) && (win != statusWindow))
        {
            getyx(mainWindow, cury, curx);
            searchOutputHighlight("");
            wmove(mainWindow, cury, curx);
            continue;
        }
        else if (key == KEY_PPAGE || key == KEY_NPAGE)
        {
            scrollNext = key == KEY_NPAGE;
            scrollPrevious = key == KEY_PPAGE;
            if (scrollPrevious && canScroll)
            {
                if (scrolling && (scrollPrevious == previousScroll))
                {
                    if (scrollRate < maxScrollRate)
                        scrollRate += deltaScrollRate;
                }
                else
                {
                    scrollRate = ON_SCROLL_RATE;
                    scrolling = true;
                }
                previousScroll = scrollPrevious;
                mainWindowTopLine -= (int)ceil(scrollRate);
                if (mainWindowTopLine < 0)
                    mainWindowTopLine = 0;
            }
            else if (scrollNext && canScroll)
            {
                if (scrolling && (scrollNext == previousScroll))
                {
                    if (scrollRate < maxScrollRate)
                        scrollRate += deltaScrollRate;
                }
                else
                {
                    scrollRate = ON_SCROLL_RATE;
                    scrolling = true;
                }
                previousScroll = scrollNext;
                mainWindowTopLine += (int)ceil(scrollRate);
                if (mainWindowTopLine > mainWindowLines - mainWindowViewHeight)
                {
                    mainWindowTopLine = mainWindowLines - mainWindowViewHeight;
                }
            }
            continue;
        }
        statusLineMsg = false;
        if (ON_READINPUT_ONESHOT & flags)
        {
            char keyStr[2] = {0};
            keyStr[0] = key & A_CHARTEXT;
            return strdup(keyStr);
        }
        else if (key == KEY_UP)
        {
            if (!((ON_READINPUT_HISTORY & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            recalled = recallPrevious();
            if (recalled)
            {
                (void)snprintf(cmd, ON_CMD_LENGTH-1, "%s", recalled);
                inputLength = strlen(recalled);
                if (inputLength >= ON_CMD_LENGTH)
                    inputLength = ON_CMD_LENGTH - 1;
                cmd[inputLength] = '\0';
                getyx(win, cury, curx);
                mvwprintw(win, cury, 0, "%s%s", prompt, cmd);
                wclrtoeol(win);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(false);
        }
        else if (key == KEY_DOWN)
        {
            if (!((ON_READINPUT_HISTORY & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            recalled = recallNext();
            if (recalled)
            {
                (void)snprintf(cmd, ON_CMD_LENGTH-1, "%s", recalled);
                inputLength = strlen(recalled);
                if (inputLength >= ON_CMD_LENGTH)
                    inputLength = ON_CMD_LENGTH - 1;
                cmd[inputLength] = '\0';
                getyx(win, cury, curx);
                mvwprintw(win, cury, 0, "%s%s", prompt, cmd);
                wclrtoeol(win);
            }
            else
            {
                cmd[0] = '\0';
                inputLength = 0;
                getyx(win, cury, curx);
                wmove(win, cury, strlen(prompt));
                wclrtoeol(win);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(false);
        }
        else if (key == '\b' || key == KEY_BACKSPACE || key == 127)
        {
            if (!((ON_READINPUT_EDIT & flags) || (ON_READINPUT_ALL & flags)))
                continue;
            
            getyx(win, cury, curx);
            if (curx > promptLength)
            {
                curx--;
                if (curx - promptLength > inputLength)
                    inputLength = curx - promptLength;
                else
                    inputLength--;
                memmove(cmd + curx - promptLength, cmd + curx - promptLength + 1, strlen(cmd + curx - promptLength + 1));
                cmd[inputLength] = '\0';
                mvwprintw(win, cury, curx, "%s ", cmd + curx - promptLength);
                wmove(win, cury, curx);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(false);
        }
        else if (key == KEY_LEFT)
        {
            if (!((ON_READINPUT_EDIT & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            getyx(win, cury, curx);
            if (curx > promptLength)
            {
                curx--;
                wmove(win, cury, curx);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(false);
            continue;
        }
        else if (key == KEY_RIGHT)
        {
            if (!((ON_READINPUT_EDIT & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            getyx(win, cury, curx);
            if (curx - strlen(prompt) < inputLength)
            {
                curx++;
                wmove(win, cury, curx);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(false);
        }
        else if (key == '\t')
        {
            if (!((ON_READINPUT_COMPLETION & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            (void)recallMostRecent();
            do
            {
                recalled = recallPrevious();
            } 
            while (recalled && strncasecmp(cmd, recalled, strlen(cmd)) != 0);

            if (recalled)
            {
                (void)snprintf(cmd, ON_CMD_LENGTH-1, "%s", recalled);
                inputLength = strlen(recalled);
                if (inputLength >= ON_CMD_LENGTH)
                    inputLength = ON_CMD_LENGTH - 1;
                cmd[inputLength] = '\0';
                getyx(win, cury, curx);
                mvwprintw(win, cury, 0, "%s%s", prompt, recalled);
                wclrtoeol(win);
                resetPromptPosition(false);
            }
        }
        else if (inputLength == ON_CMD_LENGTH - 1 || key == '\n')
        {
            cmd[inputLength] = '\0';
            getyx(win, cury, curx);
            if (curx - promptLength < strlen(cmd))
                wmove(win, cury, strlen(cmd) + promptLength);

            print(win, "\n");
            if (ON_READINPUT_SEARCH & flags)
            {
                // Highlight search term if found
                getyx(mainWindow, cury, curx);
                searchOutputHighlight(cmd);
                wmove(mainWindow, cury, curx);

            }
            else
                resetPromptPosition(false);

            break;
        }
        else
        {
            // Insert the character
            getyx(win, cury, curx);
            if (curx - promptLength < inputLength)
            {
                char *p = cmd + inputLength + 1;
                if (p >= cmd + ON_CMD_LENGTH)
                    p = cmd + ON_CMD_LENGTH - 1;
                while (p > cmd + curx - promptLength && p > cmd)
                {
                    *p = *(p-1);
                    p--;
                }
                *p = key;
                if (!(ON_READINPUT_HIDDEN & flags))
                {
                    waddch(win, key);
                    mvwprintw(win, cury, curx + 1, "%s", cmd + curx - promptLength + 1);
                    wmove(win, cury, curx + 1);
                }
            }
            else
            {
                cmd[inputLength] = key;
                if (!(ON_READINPUT_HIDDEN & flags))
                {
                    waddch(win, key);
                }
            }
            inputLength++;
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(false);
        }
    }    

    if (ON_READINPUT_NO_COPY & flags)
        return NULL;
    else
        return strdup(cmd);

}


int restoreScreenHistory(void)
{
    char sessionsFile[FILENAME_MAX] = {0};
    char *home = getenv("HOME");
    if (home != NULL && strlen(home) > 0)
        snprintf(sessionsFile, FILENAME_MAX, "%s/%s/%s", home, ON_OPTIONS_DIR, ON_SESSIONS_LOG);
    FILE *sessionsLog = NULL;
    mainWindowLines = 1; // For the prompt
    int nLines = 0;
    int status = 0;
    if (access(sessionsFile, R_OK) == 0)
    {
        sessionsLog = fopen(sessionsFile, "r");
        if (sessionsLog != NULL)
        {
            char lineBuf[ON_BUFFERED_LINE_LENGTH] = {0};
            char *res = fgets(lineBuf, ON_BUFFERED_LINE_LENGTH, sessionsLog);
            while (res != NULL)
            {
                if (mainWindowLines >= ON_BUFFERED_LINES)
                    wscrl(mainWindow, 1);
                print(mainWindow, "%s", lineBuf);
                res = fgets(lineBuf, ON_BUFFERED_LINE_LENGTH, sessionsLog);
                nLines++;
            }
            fclose(sessionsLog);
        }
    }

    return nLines;
}

int saveScreenHistory(void)
{
    int status = 0;

    char sessionsFile[FILENAME_MAX] = {0};
    char *home = getenv("HOME");
    if (home != NULL && strlen(home) > 0)
        snprintf(sessionsFile, FILENAME_MAX, "%s/%s/%s", home, ON_OPTIONS_DIR, ON_SESSIONS_LOG);

    FILE *sessionsLog = fopen(sessionsFile, "w");
    if (sessionsLog != NULL)
    {
        int status = 0;
        int lineLength = 0;
        char lineBuf[ON_BUFFERED_LINE_LENGTH] = {0};
        for (int i = mainWindowLines > ON_BUFFERED_LINES ? mainWindowLines - ON_BUFFERED_LINES : 0; i < mainWindowLines; i++)
        {
            mvwinnstr(mainWindow, i, 0, lineBuf, ON_BUFFERED_LINE_LENGTH);
            // Do not print traling spaces
            lineLength = ON_BUFFERED_LINE_LENGTH;
            while (lineLength > 0 && lineBuf[lineLength-1] == ' ')
                lineLength--;
            lineBuf[lineLength] = '\0';
            fprintf(sessionsLog, "%s\n", lineBuf);
        }
        fclose(sessionsLog);
    }
    else
        status = 2;

    return status;
}
