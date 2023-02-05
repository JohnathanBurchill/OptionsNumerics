/*
    Options Numerics: on_examples.c

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

#include "on_examples.h"
#include "on_config.h"
#include "on_parse.h"
#include "on_screen_io.h"

#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

FunctionValue examplesFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg)
{
    char *topic = arg.charStarValue;
    if (topic == NULL || topic[0] == 0)
        return FV_NOTOK;

    for (int i = 0; i < NCOMMANDS; i++)
    {
        if (strcasecmp(userInput->commands[i].longName, topic) == 0 || (userInput->commands[i].shortName != NULL && strcasecmp(userInput->commands[i].shortName, topic) == 0))
        {
            CommandExample *example = &userInput->commands[i].example;

            if (example->summary == NULL)
            {
                print(screen, screen->mainWindow, "No example available for %s\n", topic);
                return FV_NOTOK;
            }

            print(screen, screen->mainWindow, "%s\n", example->summary);

            char cmd[512] = {0};
            char parameters[400] = {0};
            if (example->template != NULL)
            {
                if (example->date != NULL)
                {
                    Date d = {0};
                    interpretDate(example->date, &d);
                    // Option tickers start with O: and have a 2-digit year
                    if (strncmp("O:", example->template, 2) == 0)
                        sprintf(parameters, example->template, d.year % 100, d.month, d.day);
                    else
                        sprintf(parameters, example->template, d.year, d.month, d.day);

                }
                else
                    sprintf(parameters, "%s", example->template);
            }

            if (example->prependComand)
                sprintf(cmd, "%s %s", topic, parameters);
            else
                sprintf(cmd, "%s", parameters);

            print(screen, screen->mainWindow, "%s\n", cmd);
            memorize(userInput, cmd);
            FunctionValue exampleArg = {0};
            exampleArg.charStarValue = parameters;
            (void)userInput->commands[i].function(screen, userInput, exampleArg);
        }

    }

    return FV_OK;
}

