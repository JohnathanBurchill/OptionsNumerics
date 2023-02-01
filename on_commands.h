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

#include "on_functions.h"
#include "on_optionsmodels.h"
#include "on_config.h"

#include <stdbool.h>
#include <stdlib.h>

#define NCOMMANDS 26

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
    FunctionValue (*function)(FunctionValue);
    FunctionValueType functionArgumentType;
    FunctionValueType functionReturnType;
    CommandExample example;
    bool isAlias;
} Command;

typedef struct thingToRemember
{
    size_t timesRemembered;
    char *thing;
} ThingToRemember;

int initCommands(void);
void freeCommands(void);

void memorize(char *this);
void forgetEverything(void);
const char *recallPrevious(void);
const char *recallNext(void);
const char *recallMostUsed(void);
const char *recallMostRecent(void);

int writeDownThingsToRemember(void);
int reviseThingsToRemember(void);
void showRememberedThings(void);

#endif // _ON_COMMANDS_H
