/*
    Options Numerics: on_config.h

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

#ifndef _ON_CONFIG_H
#define _ON_CONFIG_H

#define ON_PROMPT ":) "
#define ON_READING_CUE ":  "

#define ON_OPTIONS_DIR ".optionsnumerics"
#define ON_API_TOKENS_DIR "secrets"

#define ON_SESSIONS_LOG "on_sessions_log.txt"
#define ON_STREAMS_LOG "on_streams.txt"

#define ON_CMD_LENGTH 1000

#define ON_BUFFERED_LINES 10000
#define ON_BUFFERED_LINE_LENGTH 256

#define ON_SCROLL_RATE 5

#define ON_NUMBER_OF_THINGS_TO_REMEMBER 2000
#define ON_THINGS_TO_REMEMBER_FILENAME "on_things_remembered.txt"



#endif // _ON_OPTIONS_CONFIG_H
