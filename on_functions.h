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
FunctionValue echoFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

FunctionValue timeFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

FunctionValue helpFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

FunctionValue showCommandsFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

// Calculator
FunctionValue blackScholesOptionPriceFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue binomialOptionPriceFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue optionsTimeDecayFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

FunctionValue geeksFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue impliedVolatilityFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue impliedPriceFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

FunctionValue feesFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue timeValueOfMoneyFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

// Data
FunctionValue pioOptionsSearchFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue pioOptionsChainFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue pioPriceHistoryFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue pioVolatilityFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue pioLatestPriceFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);
FunctionValue pioPreviousCloseFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

FunctionValue fredSOFRFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

FunctionValue questradeConnectionFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

// What if?
FunctionValue optionsIncomeReinvestedFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

FunctionValue optionsExerciseChanceFunction(ScreenState *screen, UserInputState *userInput, FunctionValue arg);

#endif // _ON_FUNCTIONS_H
