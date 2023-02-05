/*
    Options Numerics: on_functions.h

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

#ifndef _ON_FUNCTIONS_H
#define _ON_FUNCTIONS_H

#include "on_state.h"

typedef union functionValue
{
    char charValue;
    char *charStarValue;
    int intValue;
    int *intStarValue;
    long longValue;
    long *longStarValue;
    double doubleValue;
    double *doubleStarValue;
    void *voidStarValue;
} FunctionValue;

#define FV_OK ((FunctionValue)0);
#define FV_NOTOK ((FunctionValue)1);

typedef enum functionValueType 
{
    FUNCTION_CHAR,
    FUNCTION_CHARSTAR,
    FUNCTION_INT,
    FUNCTION_INTSTAR,
    FUNCTION_LONG,
    FUNCTION_LONGSTAR,
    FUNCTION_DOUBLE,
    FUNCTION_DOUBLESTAR,
    FUNCTION_VOIDSTAR,
    FUNCTION_STATUS_CODE,
    FUNCTION_FUNCTION,
    FUNCTION_NONE
} FunctionValueType;

// for testing, maybe other uses
FunctionValue echoFunction(ScreenState *screen, FunctionValue arg);
FunctionValue testCommandsFunction(ScreenState *screenState, FunctionValue arg);

FunctionValue timeFunction(ScreenState *screen, FunctionValue arg);

FunctionValue helpFunction(ScreenState *screen, FunctionValue arg);

FunctionValue showCommandsFunction(ScreenState *screen, FunctionValue arg);

// Calculator
FunctionValue blackScholesOptionPriceFunction(ScreenState *screen, FunctionValue arg);
FunctionValue binomialOptionPriceFunction(ScreenState *screen, FunctionValue arg);
FunctionValue optionsTimeDecayFunction(ScreenState *screen, FunctionValue arg);

FunctionValue geeksFunction(ScreenState *screen, FunctionValue arg);
FunctionValue impliedVolatilityFunction(ScreenState *screen, FunctionValue arg);
FunctionValue impliedPriceFunction(ScreenState *screen, FunctionValue arg);

FunctionValue feesFunction(ScreenState *screen, FunctionValue arg);
FunctionValue timeValueOfMoneyFunction(ScreenState *screen, FunctionValue arg);

// Data
FunctionValue pioOptionsSearchFunction(ScreenState *screen, FunctionValue arg);
FunctionValue pioOptionsChainFunction(ScreenState *screen, FunctionValue arg);
FunctionValue pioPriceHistoryFunction(ScreenState *screen, FunctionValue arg);
FunctionValue pioVolatilityFunction(ScreenState *screen, FunctionValue arg);
FunctionValue pioLatestPriceFunction(ScreenState *screen, FunctionValue arg);
FunctionValue pioPreviousCloseFunction(ScreenState *screen, FunctionValue arg);

FunctionValue fredSOFRFunction(ScreenState *screen, FunctionValue arg);

FunctionValue questradeConnectionFunction(ScreenState *screen, FunctionValue arg);

// What if?
FunctionValue optionsIncomeReinvestedFunction(ScreenState *screen, FunctionValue arg);

FunctionValue optionsExerciseChanceFunction(ScreenState *screen, FunctionValue arg);

#endif // _ON_FUNCTIONS_H
