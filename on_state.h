#ifndef _ON_STATE_H
#define _ON_STATE_H

#include "on_config.h"
#include <ncurses.h>

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

} ScreenState;

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


#endif // _ON_STATE_H
