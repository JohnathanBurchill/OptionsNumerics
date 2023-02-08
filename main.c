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

#include "on_state.h"
#include "on_status.h"
#include "on_config.h"
#include "on_commands.h"
#include "on_calculate.h"
#include "on_info.h"
#include "on_websocket.h"
#include "on_screen_io.h"

#include <signal.h>
#include <string.h>

volatile sig_atomic_t running = 1;

static void interrupthandler(int sig)
{
    (void)sig;
    running = 0;
}

static void check_wss(int sig)
{
    (void)sig;
    updateWssStreamContent();
    clearWssStreamStatusLine();
    return;
}

int main(void)
{
    int res = 0;

    ScreenState screen = {0};
    UserInputState userInput = {0};
    screen.userInput = &userInput;

    res = initScreen(&screen);
    if (res != 0)
        return 2;

    bool handledCommand = false;

    struct sigaction intact = {0};
    intact.sa_handler = interrupthandler;
    sigaction(SIGINT, &intact, NULL);

    struct sigaction wssact = {0};
    wssact.sa_handler = check_wss;
    sigaction(SIGALRM, &wssact, NULL);

    CURLcode curlGlobal = curl_global_init(CURL_GLOBAL_ALL);
    if (curlGlobal != 0)
        mvwprintw(screen.statusWindow, 0, 1, "Internet unavailable. Some functions will not work.\n");
    wrefresh(screen.statusWindow);

    FunctionValue argument = FV_OK;

    reviseThingsToRemember(screen.userInput);

    checkWebSocketSupport(&screen);

    int linesRestored = restoreScreenHistory(&screen);
    // Show about message if there is no previous log
    if (linesRestored == 0)
        aboutFunction(&screen, argument);
    else
        print(&screen, screen.mainWindow, "%s\n", ON_READING_CUE);

    timeFunction(&screen, (FunctionValue)"Options Numerics started at ");
    print(&screen, screen.mainWindow, "%s\n", ON_READING_CUE);

    restoreWssStreamList(&screen);

    while (running)
    {
        handledCommand = false;

        readInput(&screen, screen.mainWindow, userInput.prompt, ON_READINPUT_ALL | ON_READINPUT_STATUS_WINDOW | ON_READINPUT_NO_COPY);

        if (strcmp("exit", userInput.cmd) == 0 || strcmp("quit", userInput.cmd) == 0 || strcmp("q", userInput.cmd) == 0)
            break;
            
        for (int i = 0; i < NCOMMANDS; i++)
        {
            char *longName = userInput.commands[i].longName;
            char *shortName = userInput.commands[i].shortName;
            
            if (strncasecmp(longName, userInput.cmd, strlen(longName)) == 0 || (shortName != NULL && strncasecmp(shortName, userInput.cmd, strlen(shortName)) == 0))
            {
                if (strlen(userInput.cmd) > strlen(longName) + 1 && userInput.cmd[strlen(longName)] == ' ')
                    argument.charStarValue = userInput.cmd + strlen(longName) + 1;
                else if (shortName != NULL && strlen(userInput.cmd) > strlen(shortName) + 1 && userInput.cmd[strlen(shortName)] == ' ')
                    argument.charStarValue = userInput.cmd + strlen(shortName) + 1;
                else
                    argument.charStarValue = NULL;
                if (userInput.commands[i].function != NULL)
                {
                    memorize(&userInput, userInput.cmd);
                    (void) userInput.commands[i].function(&screen, argument);
                    handledCommand = true;
                }
                break;
            }
        }
        if (!handledCommand)
        {
            // Try evaluating a math expression
            if (strlen(userInput.cmd) > 0)
            {
                memorize(&userInput, userInput.cmd);
                calculate(&screen, userInput.cmd);
            }
        }
        userInput.cmd[0] = '\0';
    }

    saveWssStreamList();

    wssCleanup();

    curl_global_cleanup();

    writeDownThingsToRemember(&userInput);
    forgetEverything(&userInput);

    freeCommands(userInput.commands);

    print(&screen, screen.mainWindow, "%s\n", ON_READING_CUE);
    timeFunction(&screen, (FunctionValue)"Options Numerics stopped at ");

    saveScreenHistory(&screen);

    endwin();

    printf("Happy trading!\n");

    return ON_OK;
}
