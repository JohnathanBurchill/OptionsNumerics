/*
    Options Numerics: main.c

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

#include "on_config.h"
#include "on_commands.h"
#include "on_dataproviders.h"
#include "on_optionsmodels.h"
#include "on_calculate.h"
#include "on_info.h"
#include "on_examples.h"
#include "on_websocket.h"
#include "on_screen_io.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/errno.h>

#include <langinfo.h>

#include <curl/curl.h>

#include <ncurses.h>

extern Command *commands;

WINDOW *mainWindow = NULL;
long mainWindowLines = 0;
long mainWindowTopLine = 0;
int statusHeight = 0;
int mainWindowViewHeight = 0;

WINDOW *statusWindow = NULL;
WINDOW *calculatorWindow = NULL; 

volatile sig_atomic_t running = 1;
char *prompt = NULL;
char cmd[ON_CMD_LENGTH] = {0};
char searchText[ON_CMD_LENGTH] = {0};
long lastSearchResultLine = 0;

static void interrupthandler(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    int res = initCommands();
    if (res != 0)
    {
        printf("Error inializing commands\n");
        return 1;
    }

    bool handledCommand = false;

    // NCurses setup
    initscr();

    statusHeight = 1;
    statusWindow = newwin(statusHeight, COLS, 0, 0);
    wattron(statusWindow, A_REVERSE);
    wrefresh(statusWindow);

    mainWindowViewHeight = LINES - statusHeight;

    mainWindow = newpad(ON_BUFFERED_LINES + 100, ON_BUFFERED_LINE_LENGTH);
    noecho();
    raw();
    keypad(statusWindow, true);
    keypad(mainWindow, true);
    intrflush(NULL, false);
    scrollok(mainWindow, true);

    struct sigaction intact = {0};
    intact.sa_handler = interrupthandler;
    sigaction(SIGINT, &intact, NULL);

    CURLcode curlGlobal = curl_global_init(CURL_GLOBAL_ALL);
    if (curlGlobal != 0)
        mvwprintw(statusWindow, 0, 1, "Internet unavailable. Some functions will not work.\n");
    wrefresh(statusWindow);

    FunctionValue argument = FV_OK;

    res = restoreScreenHistory();
    // Show about message if there is no previous log
    if (res != 0)
        aboutFunction(argument);


    reviseThingsToRemember();

    checkWebSocketSupport();

    timeFunction((FunctionValue)"New session started at ");
    prompt = ":) ";
    int promptLength = (int)strlen(prompt);
    print(mainWindow, "%s\n", READING_CUE);

    while (running)
    {
        handledCommand = false;

        readInput(mainWindow, prompt, ON_READINPUT_ALL | ON_READINPUT_STATUS_WINDOW | ON_READINPUT_NO_COPY);

        if (strcmp("exit", cmd) == 0 || strcmp("quit", cmd) == 0 || strcmp("q", cmd) == 0)
            break;

        for (int i = 0; i < NCOMMANDS; i++)
        {
            char *longName = commands[i].longName;
            char *shortName = commands[i].shortName;

            if (strncasecmp(longName, cmd, strlen(longName)) == 0 || (shortName != NULL && strncasecmp(shortName, cmd, strlen(shortName)) == 0))
            {
                if (strlen(cmd) > strlen(longName) + 1 && cmd[strlen(longName)] == ' ')
                    argument.charStarValue = cmd + strlen(longName) + 1;
                else if (shortName != NULL && strlen(cmd) > strlen(shortName) + 1 && cmd[strlen(shortName)] == ' ')
                    argument.charStarValue = cmd + strlen(shortName) + 1;
                else
                    argument.charStarValue = NULL;
                if (commands[i].function != NULL)
                {
                    memorize(cmd);
                    (void) commands[i].function(argument);
                    handledCommand = true;
                }
                break;
            }
        }
        if (!handledCommand)
        {
            // Try evaluating a math expression
            if (strlen(cmd) > 0)
            {
                memorize(cmd);
                calculate(cmd);
            }
        }
        cmd[0] = '\0';
        resetPromptPosition(false);

    }

    curl_global_cleanup();

    writeDownThingsToRemember();
    forgetEverying();

    freeCommands();

    timeFunction((FunctionValue)"Session stopped at ");

    saveScreenHistory();

    endwin();

    printf("Happy trading!\n");

    return 0;
}
