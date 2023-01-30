
# Options Numerics: CMakeLists.txt

# Copyright (C) 2023  Johnathan K Burchill

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10)
project(on)

# MATH
find_library(MATH m)

# JSON lib (ChatGPT 15 Jan 2023)
# Find jansson library
find_library(JANSSON jansson HINTS /usr/local/lib)

# Find the cURL library
find_library(CURL curl HINTS /usr/local/lib)
# include_directories(${CURL_INCLUDE_DIRS})

# CURSES
find_library(CURSES ncursesw HINTS /usr/local/lib)
include_directories(/usr/local/include)

add_executable(on main.c on_commands.c on_api.c on_optionsmodels.c on_optionstiming.c on_dataproviders.c on_statistics.c on_utilities.c on_parse.c on_calculate.c on_info.c on_websocket.c on_screen_io.c on_examples.c on_functions.c)
target_link_libraries(on ${History} ${CURSES} ${CURL} ${JANSSON} ${MATH})

install(TARGETS on RUNTIME DESTINATION bin)
