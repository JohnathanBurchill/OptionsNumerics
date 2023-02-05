/*
    Options Numerics: on_parse.h

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

#ifndef _ON_PARSE_H
#define _ON_PARSE_H

#include "on_optionstiming.h"

#include <stdbool.h>

int dateRange(char *input, Date *date1, Date *date2);

int interpretDate(char *dateString, Date *date);

bool expectedKey(char *string, char *key);
bool expectedKeys(char **tokens, char **keys);

char **splitString(char *string, char delimiter, int *nWords);
char **splitStringByKeys(char *string, char **keys, char delimeter, int *nTokens);

void freeTokens(char **tokens, int ntokens);

#endif // _ON_PARSE_H
