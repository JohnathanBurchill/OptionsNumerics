/*
    Options Numerics: on_api.c

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

#include "on_api.h"
#include "on_config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int saveApiToken(char *name, char* token)
{
    char *homeDir = getenv("HOME");
    if (access(homeDir, F_OK) != 0)
        return 1;

    char configFile[FILENAME_MAX];
    snprintf(configFile, FILENAME_MAX, "%s/%s", homeDir, ON_OPTIONS_DIR);
    if (access(configFile, F_OK))
        if (mkdir(configFile, 0700))
            return 1;

    snprintf(configFile, FILENAME_MAX, "%s/%s/%s", homeDir, ON_OPTIONS_DIR, ON_API_TOKENS_DIR);
    if (access(configFile, F_OK))
        if (mkdir(configFile, 0700))
            return 1;

    snprintf(configFile, FILENAME_MAX, "%s/%s/%s/%s", homeDir, ON_OPTIONS_DIR, ON_API_TOKENS_DIR, name);

    FILE *f = fopen(configFile, "w");
    fprintf(f, "%s", token);
    fclose(f);

    if (chmod(configFile, 0600))
        return 1;

    return 0;

}

char * loadApiToken(char *name)
{
    char *homeDir = getenv("HOME");
    if (access(homeDir, F_OK) != 0)
        return NULL;

    char configFile[FILENAME_MAX];
    snprintf(configFile, FILENAME_MAX, "%s/%s/%s/%s", homeDir, ON_OPTIONS_DIR, ON_API_TOKENS_DIR, name);
    if (access(configFile, F_OK) != 0)
        return NULL;

    char *token = calloc(1024, 1);
    if (token == NULL)
        return NULL;

    FILE *f = fopen(configFile, "r");

    fscanf(f, "%s", token);

    fclose(f);

    return token;
}
