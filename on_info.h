/*
    Options Numerics: on_info.h

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

#ifndef _ON_INFO_H
#define _ON_INFO_H

#include "on_state.h"
#include "on_commands.h"

FunctionValue aboutFunction(ScreenState *screen, FunctionValue topic);
FunctionValue licenseFunction(ScreenState *screen, FunctionValue topic);
FunctionValue helpFunction(ScreenState *screen, FunctionValue topic);
void api_names(ScreenState *screen);

#endif // _ON_INFO_H
