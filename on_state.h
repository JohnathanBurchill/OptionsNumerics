/*
    Options Numerics: on_state.h

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

#ifndef _ON_STATE_H
#define _ON_STATE_H

#include "on_config.h"
#include <ncurses.h>

typedef struct thingToRemember
{
    size_t timesRemembered;
    char *thing;
} ThingToRemember;

struct command;
typedef struct command Command;

typedef struct userInputState
{
    char *prompt;
    int promptLength;
    char cmd[ON_CMD_LENGTH];
    ThingToRemember thingsRemembered[ON_NUMBER_OF_THINGS_TO_REMEMBER];
    int numberOfThingsRemembered;
    int thinkingOf;
    int recallDirection;

    Command *commands;

} UserInputState;

typedef struct screenState
{
    WINDOW *mainWindow;
    long mainWindowLines;
    long mainWindowTopLine;
    int mainWindowViewHeight;

    WINDOW *statusWindow;
    int statusHeight;
    
    WINDOW *streamWindow;
    int streamWindowHeight;

    char searchText[ON_CMD_LENGTH];
    long lastSearchResultLine;    

    UserInputState *userInput;

} ScreenState;

#endif // _ON_STATE_H
