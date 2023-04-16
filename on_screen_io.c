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
#include "on_status.h"
#include "on_config.h"
#include "on_commands.h"
#include "on_utilities.h"
#include "on_websocket.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include <ncurses.h>

extern volatile sig_atomic_t running;

int initScreen(ScreenState *screen)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    int status = OK;
    initscr();
    screen->streamWindowHeight = 1;
    screen->statusHeight = 1;
    screen->mainWindowViewHeight = LINES - screen->statusHeight - screen->streamWindowHeight;

    screen->streamWindow = newwin(LINES / 2, COLS, 0, 0);
    status |= (screen->streamWindow == NULL);

    screen->statusWindow = newwin(screen->statusHeight, COLS, screen->streamWindowHeight, 0);
    status |= (screen->statusWindow == NULL);
    status |= wattron(screen->statusWindow, A_REVERSE);
    status |= wrefresh(screen->statusWindow);
    // status |= nodelay(screen->statusWindow, true);

    screen->mainWindow = newpad(ON_BUFFERED_LINES + 500, ON_BUFFERED_LINE_LENGTH);
    status |= (screen->mainWindow == NULL);
    // status |= nodelay(screen->mainWindow, true);

    status |= noecho();
    status |= raw();
    status |= keypad(screen->statusWindow, true);
    status |= keypad(screen->mainWindow, true);
    status |= intrflush(NULL, false);
    status |= scrollok(screen->mainWindow, true);
    status |= scrollok(screen->streamWindow, true);

    screen->lastSearchResultLine = 0;

    status |= initUserInput(screen->userInput);

    curs_set(2);

    return (status << 1);

}

int initUserInput(UserInputState *userInput)
{
    if (userInput == NULL)
        return ON_NO_USERINPUT;

    userInput->prompt = ON_PROMPT;
    userInput->promptLength = (int)strlen(userInput->prompt);

    userInput->numberOfThingsRemembered = 0;
    userInput->thinkingOf = 0;
    userInput->recallDirection = -1;

    int res = initCommands(&userInput->commands);

    return res;
}

void prepareForALotOfOutput(ScreenState *screen, long nLines)
{
    if (screen == NULL)
        return;

    if (screen->mainWindowLines >= ON_BUFFERED_LINES - nLines)
    {
        long freeLines = ON_BUFFERED_LINES - screen->mainWindowLines;
        long shift = nLines - freeLines;
        int y = 0, x = 0;
        getyx(screen->mainWindow, y, x);
        wscrl(screen->mainWindow, shift);
        wmove(screen->mainWindow, y - shift, x);
        screen->mainWindowLines -= shift;
    }

    return;
}

int print(ScreenState *screen, WINDOW *window, const char *fmt, ...)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (window == NULL)
        return ON_NO_WINDOW;

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
        if (screen->mainWindowLines < ON_BUFFERED_LINES)
            screen->mainWindowLines += y1 - y0;
        else
        {
            wscrl(window, y1 - y0);
            wmove(window, y0, x1);
        }
    }

    return res;
}

int mvprint(ScreenState *screen, WINDOW *window, int row, int col, const char *fmt, ...)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    if (window == NULL)
        return ON_NO_WINDOW;

    if (fmt == NULL)
        return ON_MISSING_ARG_POINTER;

    int y1 = 0, x1 = 0;

    va_list args;
    va_start(args, fmt);
    wmove(window, row, col);
    int res = vw_printw(window, fmt, args);
    va_end(args);

    getyx(window, y1, x1);
    if (y1 > row)
    {
        if (screen->mainWindowLines < ON_BUFFERED_LINES)
            screen->mainWindowLines += y1 - row;
        else
        {
            wscrl(window, y1 - row);
            wmove(window, row, x1);
        }
    }

    return res;
}

void resetPromptPosition(ScreenState *screen, bool toBottom)
{
    if (screen == NULL)
        return;

    int y = 0, x = 0;
    getyx(screen->mainWindow, y, x);
    if (y - screen->mainWindowTopLine > screen->mainWindowViewHeight - 1 || toBottom)
        screen->mainWindowTopLine = screen->mainWindowLines - (screen->mainWindowViewHeight);
    if (screen->mainWindowTopLine < 0)
        screen->mainWindowTopLine = 0;

    return;
}

char *readInput(ScreenState *screen, WINDOW *win, char *prompt, int flags)
{

    if (screen == NULL || win == NULL || prompt == NULL)
        return NULL;

    int cury = 0;
    int curx = 0;

    int key = 0;
    
    int inputLength = 0;
    int promptLength = (int)strlen(prompt);

    const char *recalled = NULL;

    print(screen, win, "%s", prompt);
    if (ON_READINPUT_SEARCH & flags)
        wattroff(screen->statusWindow, A_REVERSE);

    wclrtoeol(win);
    if (!(ON_READINPUT_SEARCH & flags))
        resetPromptPosition(screen, false);

    UserInputState *userInput = screen->userInput;

    userInput->cmd[0] = '\0';

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

    bool recalling = false;
    char *recallRequest = NULL;

    while (running)
    {
        canScroll = (((ON_READINPUT_SCROLL & flags) || (ON_READINPUT_ALL & flags)) && screen->mainWindowLines > LINES);

        if (!statusLineMsg && win != screen->statusWindow && ((ON_READINPUT_STATUS_WINDOW & flags) || (ON_READINPUT_ALL & flags)))
        {
            startLine = screen->mainWindowTopLine + 1;
            if (screen->mainWindowLines - screen->mainWindowTopLine > screen->mainWindowViewHeight)
                stopLine = screen->mainWindowTopLine + screen->mainWindowViewHeight;
            else
                stopLine = screen->mainWindowLines;
            mvwprintw(screen->statusWindow, 0, 0, "Options Numerics; showing lines %d - %d of %ld", startLine, stopLine, screen->mainWindowLines);
            if (startLine > 1)
                wprintw(screen->statusWindow, " <output above>");
            wclrtoeol(screen->statusWindow);
            wrefresh(screen->statusWindow);
        }

        if (ON_READINPUT_SEARCH & flags)
            prefresh(screen->mainWindow, screen->mainWindowTopLine, 0, screen->statusHeight + screen->streamWindowHeight, 0, LINES - 1, COLS - 1);
        if (is_pad(win))
            prefresh(win, screen->mainWindowTopLine, 0, screen->statusHeight + screen->streamWindowHeight, 0, LINES - 1, COLS - 1);
        else
            wrefresh(win);

        key = wgetch(win);

        // mvwprintw(screen->statusWindow, 0, 0, "%d", key);
        // wclrtoeol(screen->statusWindow);
        // wrefresh(screen->statusWindow);

        if (key == KEY_RESIZE)
        {
            screen->mainWindowViewHeight = LINES - screen->statusHeight;
            continue;
        }
        else if (key == ERR)
        {
            if (!scrolling)
            {
                scrollRate = ON_SCROLL_RATE;
                previousScroll = -1;
            }
            continue;
        }
        else if (ON_READINPUT_ONESHOT & flags)
        {
            if (recalling)
            {
                free(recallRequest);
                recallRequest = NULL;
            }
            char keyStr[2] = {0};
            keyStr[0] = key & A_CHARTEXT;
            return strdup(keyStr);
        }
        if (key == ('C' & 0x1f) || key == ('V' & 0x1f))
            continue;
        // Find
        else if ((key == ('F' & 0x1f)) && (win != screen->statusWindow))
        {
            if (recalling)
            {
                recalling = false;
                free(recallRequest);
                recallRequest = NULL;
                (void)recallMostRecent(userInput);
            }
            wmove(screen->statusWindow, 0, 0);
            char * cmdold = strdup(userInput->cmd);
            char * ans = readInput(screen, screen->statusWindow, "Search for: ", ON_READINPUT_ALL | ON_READINPUT_SEARCH);
            memorize(userInput, ans);
            free(ans);
            size_t l = strlen(cmdold);
            memcpy(userInput->cmd, cmdold, l < ON_CMD_LENGTH - 1 ? l + 1 : ON_CMD_LENGTH);
            free(cmdold);
            statusLineMsg = true;
            continue;
        }
        // Find again
        else if ((key == ('R' & 0x1f)) && (win != screen->statusWindow))
        {
            if (recalling)
            {
                recalling = false;
                free(recallRequest);
                recallRequest = NULL;
                (void)recallMostRecent(userInput);
            }
            getyx(screen->mainWindow, cury, curx);
            searchOutputHighlight(screen, "");
            wmove(screen->mainWindow, cury, curx);
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
                screen->mainWindowTopLine -= (int)ceil(scrollRate);
                if (screen->mainWindowTopLine < 0)
                    screen->mainWindowTopLine = 0;
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
                screen->mainWindowTopLine += (int)ceil(scrollRate);
                if (screen->mainWindowTopLine > screen->mainWindowLines - screen->mainWindowViewHeight)
                {
                    screen->mainWindowTopLine = screen->mainWindowLines - screen->mainWindowViewHeight;
                }
            }
            continue;
        }
        statusLineMsg = false;
        if (key == KEY_UP)
        {
            if (!((ON_READINPUT_HISTORY & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            if (recalling)
            {
                recalling = false;
                free(recallRequest);
                recallRequest = NULL;
                (void)recallMostRecent(userInput);
            }

            recalled = recallPrevious(userInput);
            if (recalled)
            {
                (void)snprintf(userInput->cmd, ON_CMD_LENGTH-1, "%s", recalled);
                inputLength = strlen(recalled);
                if (inputLength >= ON_CMD_LENGTH)
                    inputLength = ON_CMD_LENGTH - 1;
                userInput->cmd[inputLength] = '\0';
                getyx(win, cury, curx);
                mvwprintw(win, cury, 0, "%s%s", prompt, userInput->cmd);
                wclrtoeol(win);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(screen, false);
        }
        else if (key == KEY_DOWN)
        {
            if (!((ON_READINPUT_HISTORY & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            if (recalling)
            {
                recalling = false;
                free(recallRequest);
                recallRequest = NULL;
                (void)recallMostRecent(userInput);
            }

            recalled = recallNext(userInput);
            if (recalled)
            {
                (void)snprintf(userInput->cmd, ON_CMD_LENGTH-1, "%s", recalled);
                inputLength = strlen(recalled);
                if (inputLength >= ON_CMD_LENGTH)
                    inputLength = ON_CMD_LENGTH - 1;
                userInput->cmd[inputLength] = '\0';
                getyx(win, cury, curx);
                mvwprintw(win, cury, 0, "%s%s", prompt, userInput->cmd);
                wclrtoeol(win);
            }
            else
            {
                userInput->cmd[0] = '\0';
                inputLength = 0;
                getyx(win, cury, curx);
                wmove(win, cury, strlen(prompt));
                wclrtoeol(win);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(screen, false);
        }
        else if (key == '\b' || key == KEY_BACKSPACE || key == 127)
        {
            if (!((ON_READINPUT_EDIT & flags) || (ON_READINPUT_ALL & flags)))
                continue;
            
            if (recalling)
            {
                recalling = false;
                free(recallRequest);
                recallRequest = NULL;
                (void)recallMostRecent(userInput);
            }

            getyx(win, cury, curx);
            if (curx > promptLength)
            {
                curx--;
                if (curx - promptLength > inputLength)
                    inputLength = curx - promptLength;
                else
                    inputLength--;
                memmove(userInput->cmd + curx - promptLength, userInput->cmd + curx - promptLength + 1, strlen(userInput->cmd + curx - promptLength + 1));
                userInput->cmd[inputLength] = '\0';
                mvwprintw(win, cury, curx, "%s ", userInput->cmd + curx - promptLength);
                wmove(win, cury, curx);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(screen, false);
        }
        else if (key == KEY_LEFT)
        {
            if (!((ON_READINPUT_EDIT & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            if (recalling)
            {
                recalling = false;
                free(recallRequest);
                recallRequest = NULL;
                (void)recallMostRecent(userInput);
            }

            getyx(win, cury, curx);
            if (curx > promptLength)
            {
                curx--;
                wmove(win, cury, curx);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(screen, false);
            continue;
        }
        else if (key == KEY_RIGHT)
        {
            if (!((ON_READINPUT_EDIT & flags) || (ON_READINPUT_ALL & flags)))
                continue;

            if (recalling)
            {
                recalling = false;
                free(recallRequest);
                recallRequest = NULL;
                (void)recallMostRecent(userInput);
            }

            getyx(win, cury, curx);
            if (curx - strlen(prompt) < inputLength)
            {
                curx++;
                wmove(win, cury, curx);
            }
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(screen, false);
        }
        else if (key == '\t')
        {
            if (!((ON_READINPUT_COMPLETION & flags) || (ON_READINPUT_ALL & flags)))
                continue;
            if (!recalling)
            {
                recalling = true;
                recallRequest = strdup(userInput->cmd);
                (void)recallMostRecent(userInput);
            }
            do
            {
                recalled = recallPrevious(userInput);
            }
            while (recalled && recallRequest != NULL && strncasecmp(recallRequest, recalled, strlen(recallRequest)) != 0);

            if (recalled)
            {
                (void)snprintf(userInput->cmd, ON_CMD_LENGTH-1, "%s", recalled);
                inputLength = strlen(recalled);
                if (inputLength >= ON_CMD_LENGTH)
                    inputLength = ON_CMD_LENGTH - 1;
                userInput->cmd[inputLength] = '\0';
                getyx(win, cury, curx);
                mvwprintw(win, cury, 0, "%s%s", prompt, recalled);
                wclrtoeol(win);
                resetPromptPosition(screen, false);
            }
            else
            {
                recalling = false;
                free(recallRequest);
                recallRequest = NULL;
                recallMostRecent(userInput);
            }
        }
        else if (inputLength == ON_CMD_LENGTH - 1 || key == '\n')
        {
            if (recalling)
            {
                recalling = false;
                (void)recallMostRecent(userInput);
            }
            userInput->cmd[inputLength] = '\0';
            getyx(win, cury, curx);
            if (curx - promptLength < strlen(userInput->cmd))
                wmove(win, cury, strlen(userInput->cmd) + promptLength);

            print(screen, win, "\n");
            if (ON_READINPUT_SEARCH & flags)
            {
                // Highlight search term if found
                getyx(screen->mainWindow, cury, curx);
                searchOutputHighlight(screen, userInput->cmd);
                wmove(screen->mainWindow, cury, curx);

            }
            else
                resetPromptPosition(screen, false);

            break;
        }
        else
        {
            if (recalling)
            {
                recalling = false;
                (void)recallMostRecent(userInput);
            }

            // Insert the character
            getyx(win, cury, curx);
            if (!(ON_READINPUT_HIDDEN & flags) && (curx - promptLength < inputLength))
            {
                char *p = userInput->cmd + inputLength + 1;
                if (p >= userInput->cmd + ON_CMD_LENGTH)
                    p = userInput->cmd + ON_CMD_LENGTH - 1;
                while (p > userInput->cmd + curx - promptLength && p > userInput->cmd)
                {
                    *p = *(p-1);
                    p--;
                }
                *p = key;
                waddch(win, key);
                mvwprintw(win, cury, curx + 1, "%s", userInput->cmd + curx - promptLength + 1);
                wmove(win, cury, curx + 1);
            }
            else
            {
                userInput->cmd[inputLength] = key;
                if (!(ON_READINPUT_HIDDEN & flags))
                {
                    waddch(win, key);
                }
            }
            inputLength++;
            if (!(ON_READINPUT_SEARCH & flags))
                resetPromptPosition(screen, false);
        }
    }    

    if (ON_READINPUT_NO_COPY & flags)
    {
        if (ON_READINPUT_HIDDEN & flags)
            bzero(userInput->cmd, ON_CMD_LENGTH);
        return NULL;
    }
    else
    {
        char *result = strdup(userInput->cmd);
        if (ON_READINPUT_HIDDEN & flags)
        {
            bzero(userInput->cmd, ON_CMD_LENGTH);
        }
        return result;
    }

}

int restoreScreenHistory(ScreenState *screen)
{

    if (screen == NULL)
        return ON_OK;

    char sessionsFile[FILENAME_MAX] = {0};
    char *home = getenv("HOME");
    if (home != NULL && strlen(home) > 0)
        snprintf(sessionsFile, FILENAME_MAX, "%s/%s/%s", home, ON_OPTIONS_DIR, ON_SESSIONS_LOG);
    FILE *sessionsLog = NULL;
    screen->mainWindowLines = 1; // For the prompt
    int nLines = 0;
    int linesToSkip = 0;
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
                nLines++;
                res = fgets(lineBuf, ON_BUFFERED_LINE_LENGTH, sessionsLog);
            }

            fseek(sessionsLog, 0, SEEK_SET);
            linesToSkip = nLines - ON_BUFFERED_LINES;
            if (linesToSkip < 0)
                linesToSkip = 0;
            for (int l = 0; l < nLines; l++)
            {
                res = fgets(lineBuf, ON_BUFFERED_LINE_LENGTH, sessionsLog);
                if (l >= linesToSkip)
                    print(screen, screen->mainWindow, "%s", lineBuf);
            }
            fclose(sessionsLog);
        }
    }

    return nLines - linesToSkip;
}

int saveScreenHistory(ScreenState *screen)
{
    if (screen == NULL)
        return ON_NO_SCREEN;

    int status = ON_OK;

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
        for (int i = screen->mainWindowLines > ON_BUFFERED_LINES ? screen->mainWindowLines - ON_BUFFERED_LINES : 0; i < screen->mainWindowLines; i++)
        {
            mvwinnstr(screen->mainWindow, i, 0, lineBuf, ON_BUFFERED_LINE_LENGTH);
            // Do not print trailing spaces
            lineLength = ON_BUFFERED_LINE_LENGTH;
            while (lineLength > 0 && lineBuf[lineLength-1] == ' ')
                lineLength--;
            lineBuf[lineLength] = '\0';
            if (strlen(lineBuf) > 0)
                fprintf(sessionsLog, "%s\n", lineBuf);
        }
        fclose(sessionsLog);
    }
    else
        status = ON_FILE_WRITE_ERROR;

    return status;
}
