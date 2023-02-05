/*
    Options Numerics: on_commands.h

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

#ifndef _ON_COMMANDS_H
#define _ON_COMMANDS_H

#include "on_state.h"
#include "on_functions.h"

#include <stdbool.h>
#include <stdlib.h>

#define NCOMMANDS 28

typedef struct commandExample
{
    char *summary;
    char *template;
    char *date;
    bool prependComand;
} CommandExample;

typedef struct command
{
    char *commandGroup;
    char *longName;
    char *shortName;
    char *helpMessage;
    char *usage;
    FunctionValue (*function)(ScreenState *, UserInputState *, FunctionValue);
    FunctionValueType functionArgumentType;
    FunctionValueType functionReturnType;
    CommandExample example;
    bool isAlias;
} Command;

int initCommands(Command **commands);
void freeCommands(Command *commands);

void memorize(UserInputState *userInput, char *this);
void forgetEverything(UserInputState *userInput);
const char *recallPrevious(UserInputState *userInput);
const char *recallNext(UserInputState *userInput);
const char *recallMostUsed(UserInputState *userInput);
const char *recallMostRecent(UserInputState *userInput);

int writeDownThingsToRemember(UserInputState *userInput);
int reviseThingsToRemember(UserInputState *userInput);
void showRememberedThings(ScreenState *screen, UserInputState *userInput);

#endif // _ON_COMMANDS_H
