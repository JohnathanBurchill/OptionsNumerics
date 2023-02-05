/*
    Options Numerics: on_screen_io.h

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

#ifndef _ON_SCREEN_IO_H
#define _ON_SCREEN_IO_H

#include "on_state.h"

#include <stdbool.h>

#include <ncurses.h>

int initScreen(ScreenState *screen);

void prepareForALotOfOutput(ScreenState *screen, long nLines);
int print(ScreenState *screen, WINDOW *window, const char *fmt, ...);
int mvprint(ScreenState *screen, WINDOW *window, int row, int col, const char *fmt, ...);

void resetPromptPosition(ScreenState *screen, bool toBottom);

enum ReadInputFlags
{
    ON_READINPUT_NONE = 0,
    ON_READINPUT_SCROLL = 1 << 0,
    ON_READINPUT_EDIT = 1 << 1,
    ON_READINPUT_HISTORY = 1 << 2,
    ON_READINPUT_COMPLETION = 1 << 3,
    ON_READINPUT_ALL = 1 << 4,
    ON_READINPUT_STATUS_WINDOW = 1 << 5,
    ON_READINPUT_HIDDEN = 1 << 6,
    ON_READINPUT_NO_COPY = 1 << 7,
    ON_READINPUT_ONESHOT = 1 << 8,
    ON_READINPUT_AT_BOTTOM = 1 << 9,
    ON_READINPUT_SEARCH = 1 << 10
};

int initUserInput(UserInputState *userInput);

char *readInput(ScreenState *screen, UserInputState *userInput, WINDOW *win, char *prompt, int flags);

int restoreScreenHistory(ScreenState *screen);
int saveScreenHistory(ScreenState *screen);

#endif // _ON_SCREEN_IO_H

