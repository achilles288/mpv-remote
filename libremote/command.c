/**
 * @file command.h
 * @brief Command-line-controlled media player
 * 
 * The module contains functions for parsing and writing commands.
 * Write command in a file and read command from the file to make a
 * communication from the remote service to display system.
 * 
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 * 
 * @license{This project is released under the GPL License.}
 */


#ifdef _WIN32
#define REMOTE_EXPORT __declspec(dllexport)
#else
#define REMOTE_EXPORT
#endif

#include "command.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#else
#include <linux/limits.h>
#endif
#define MESSAGE_MAX REMOTE_MESSAGE_MAX


static int command_tokenize(char *line, char *tokens[], int max) {
    for(int i=0; i<max; i++)
        tokens[i] = NULL;
    
    char *tok = strtok(line, " ");
    int i = 0;
    int quote = 0;
    while(tok != NULL && i < max) {
        if(!quote) {
            if(tok[0] == '"') {
                quote = 1;
                tokens[i] = tok + 1;
                size_t len = strlen(tok);
                if(tok[len-1] == '"') {
                    quote = 0;
                    tok[len-1] = '\0';
                    i++;
                }
            }
            else {
                tokens[i] = tok;
                i++;
            }
        }
        else {
            *(tok-1) = ' ';
            size_t len = strlen(tok);
            if(tok[len-1] == '"') {
                quote = 0;
                tok[len-1] = '\0';
                i++;
            }
        }
        tok = strtok(NULL, " ");
    }
    
    return i;
}


static void *options[4];
static int int1;
static char urlRequest[PATH_MAX];
static double timeRequest = 0.0;


/**
 * @brief Write a command in a file
 * 
 * Writes the given string in a temporary file. This function is usually
 * called by the remote program. The file the remote program writes is
 * read by the display program and the communication is done this way.
 * 
 * @param fmt Command string
 * @param ... Additional variables to be printed in the format like printf()
 */
REMOTE_EXPORT void remote_command_write(const char* fmt, ...) {
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    FILE *fp = fopen(cmdFile, "w");
    if(fp == NULL)
        return;
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fclose(fp);
}

/**
 * @brief Read a command in a file
 * 
 * Writes the given string in a temporary file. This function is usually
 * called by the remote program. The file the remote program writes is
 * read by the display program and the communication is done this way.
 * 
 * @return Command number (REMOTE_COMMAND_NONE, REMOTE_COMMAND_OPEN,
 *         REMOTE_COMMAND_PAUSE, REMOTE_COMMAND_MOVE, REMOTE_COMMAND_STOP,
 *         REMOTE_COMMAND_KILL)
 */
REMOTE_EXPORT int remote_command_read() {
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    FILE *fp = fopen(cmdFile, "r");
    if(fp == NULL)
        return REMOTE_COMMAND_NONE;
    
    // Loads the line
    char cmd[MESSAGE_MAX];
    fgets(cmd, MESSAGE_MAX, fp);
    fclose(fp);
    remove(cmdFile);
    
    // Tokenizes the command line
    char *tokens[4];
    int tokenCount = command_tokenize(cmd, tokens, 4);
    options[0] = NULL;
    
    // Sees the command checking the first token
    if(strcmp(tokens[0], "open") == 0) {
        if(tokenCount < 2)
            return REMOTE_COMMAND_NONE;
        strcpy(urlRequest, tokens[1]);
        if(tokenCount == 3) {
            if(strcmp(tokens[2], "--pause") == 0)
                int1 = 1;
        }
        else {
            int1 = 0;
        }
        options[0] = (void*) urlRequest;
        options[1] = (void*) &int1;
        options[2] = NULL;
        return REMOTE_COMMAND_OPEN;
    }
    else if(strcmp(tokens[0], "pause") == 0) {
        int1 = -1;
        if(tokenCount == 2) {
            if(strcmp(tokens[1], "0") == 0)
                int1 = 0;
            else if(strcmp(tokens[1], "1") == 0)
                int1 = 1;
        }
        options[0] = (void*) &int1;
        options[1] = NULL;
        return REMOTE_COMMAND_PAUSE;
    }
    else if(strcmp(tokens[0], "move") == 0 || strcmp(tokens[0], "seek") == 0)
    {
        if(tokenCount < 2)
            return REMOTE_COMMAND_NONE;
        sscanf(tokens[1], "%lf", &timeRequest);
        options[0] = (void*) &timeRequest;
        options[1] = NULL;
        if(strcmp(tokens[0], "move") == 0)
            return REMOTE_COMMAND_MOVE;
        else
            return REMOTE_COMMAND_SEEK;
    }
    else if(strcmp(tokens[0], "stop") == 0) {
        return REMOTE_COMMAND_STOP;
    }
    else if(strcmp(tokens[0], "kill") == 0) {
        return REMOTE_COMMAND_KILL;
    }
    else {
        return REMOTE_COMMAND_NONE;
    }
}

/**
 * @brief Gets the command options
 * 
 * The command options can be retrived after calling the remote_command_read()
 * function.
 * 
 * @return Array of pointers to options
 */
REMOTE_EXPORT void** remote_command_get_options() { return options; }
