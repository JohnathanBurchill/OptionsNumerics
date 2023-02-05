/*
    Options Numerics: on_utilities.h

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

#ifndef _ON_UTILITIES_H
#define _ON_UTILITIES_H

#include "on_state.h"

#include <stdbool.h>

char continueOrQuit(ScreenState *screen, int maxLineLength, bool pageBreak);

void searchOutputHighlight(ScreenState *screen, char *forThisString);

char *nanoSecondsAsStringMustFree(double nanoseconds);

#endif // _ON_UTILITIES_H
