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
FunctionValue echoFunction(FunctionValue arg);

FunctionValue timeFunction(FunctionValue arg);

FunctionValue helpFunction(FunctionValue arg);

FunctionValue showCommandsFunction(FunctionValue arg);

// Calculator
FunctionValue blackScholesOptionPriceFunction(FunctionValue arg);
FunctionValue binomialOptionPriceFunction(FunctionValue arg);
FunctionValue optionsTimeDecayFunction(FunctionValue arg);

FunctionValue geeksFunction(FunctionValue arg);
FunctionValue impliedVolatilityFunction(FunctionValue arg);

FunctionValue feesFunction(FunctionValue arg);
FunctionValue timeValueOfMoneyFunction(FunctionValue arg);

// Data
FunctionValue pioOptionsSearchFunction(FunctionValue arg);
FunctionValue pioOptionsChainFunction(FunctionValue arg);
FunctionValue pioPriceHistory(FunctionValue arg);
FunctionValue pioVolatilityFunction(FunctionValue arg);
FunctionValue pioLatestPriceFunction(FunctionValue arg);
FunctionValue pioPreviousCloseFunction(FunctionValue arg);

FunctionValue fredSOFRFunction(FunctionValue arg);

FunctionValue questradeConnectionFunction(FunctionValue arg);

// What if?
FunctionValue optionsIncomeReinvestedFunction(FunctionValue arg);

FunctionValue optionsExerciseChanceFunction(FunctionValue arg);

#endif // _ON_FUNCTIONS_H
