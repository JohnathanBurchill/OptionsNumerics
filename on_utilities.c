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
#include "on_info.h"
#include "on_config.h"
#include "on_screen_io.h"

#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <ncurses.h>

extern WINDOW *mainWindow;
extern WINDOW *statusWindow;
extern WINDOW *streamWindow;

extern long mainWindowLines;
extern long mainWindowTopLine;

extern char searchText[ON_CMD_LENGTH];
extern long lastSearchResultLine;

char continueOrQuit(int maxLineLength, bool pageBreak)
{
    char template[20] = {0};
    char action = 0;
    sprintf(template, "%%%ds", maxLineLength + (int)strlen(READING_CUE));
    char prompt[256] = {0};
    snprintf(prompt, 256, template, "[any key but q for next page...]");
    // wprintw(mainWindow, template, "[any key but q for next page...]");
    // // From ChatGPT 16 Jan 2023, to hide echo when getting user input
    // struct termios old_tio, new_tio;
    // tcgetattr(STDIN_FILENO, &old_tio);
    // new_tio = old_tio;
    // new_tio.c_lflag &= (~ICANON & ~ECHO);
    // tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    // action = getchar();
    // tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

    char *a = readInput(mainWindow, prompt, ON_READINPUT_ALL | ON_READINPUT_HIDDEN | ON_READINPUT_ONESHOT);
    if (a != NULL)
    {
        action = a[0];
        free(a);
    }
    wprintw(mainWindow, "\r");
    wprintw(mainWindow, template, "");
    if (pageBreak)
        print(mainWindow, "\n");
    else
        wprintw(mainWindow, "\r");

    return action;
}

void searchOutputHighlight(char *forThisString)
{
    if (forThisString == NULL)
        return;

    int sLen = strlen(forThisString);
    if (sLen > 0)
    {
        snprintf(searchText, ON_CMD_LENGTH, "%s", forThisString);
        searchText[sLen < ON_CMD_LENGTH ? sLen : ON_CMD_LENGTH-1] = '\0';
        lastSearchResultLine = mainWindowLines - 1;
    }
    else if (strlen(searchText) > 0)
        forThisString = searchText;

    if (strlen(forThisString) == 0)
        return;

    char lineBuf[ON_BUFFERED_LINE_LENGTH+1] = {0};

    int status = 0;
    int lineLength = 0;
    size_t searchLen = strlen(forThisString);
    int i = lastSearchResultLine - 1;

    for (; i >= 0; i--)
    {
        mvwinnstr(mainWindow, i, 0, lineBuf, ON_BUFFERED_LINE_LENGTH);
        // Do not print traling spaces
        for (lineLength = ON_BUFFERED_LINE_LENGTH; lineLength > 0 && lineBuf[lineLength-1] == ' '; lineLength--);
        lineBuf[lineLength] = '\0';
        for (int c = 0; c < lineLength - 1 - (int)searchLen; c++)
        {
            if (strlen(lineBuf) > 0 && strncmp(forThisString, lineBuf + c, searchLen) == 0)
            {
                wattron(statusWindow, A_REVERSE);
                mvwprintw(statusWindow, 0, 0, "Options Numerics; found at line %d, column %d:", i + 1, c + 1);
                wattroff(statusWindow, A_REVERSE);
                wprintw(statusWindow, " %s", lineBuf + c);
                wclrtoeol(statusWindow);
                wattron(statusWindow, A_REVERSE);
                wrefresh(statusWindow);

                mainWindowTopLine = i;
                wmove(mainWindow, i, c);
                wattron(mainWindow, A_BOLD | A_UNDERLINE);
                waddstr(mainWindow, forThisString);
                wattroff(mainWindow, A_BOLD | A_UNDERLINE);
                lastSearchResultLine = i;
                return;
            }
        }
    }
    wattron(statusWindow, A_REVERSE);
    mvwprintw(statusWindow, 0, 0, "Text not found");
    wclrtoeol(statusWindow);
    wrefresh(statusWindow);    

    return;

}
