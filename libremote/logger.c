/**
 * @file logger.c
 * @brief Reads and writes media player log
 * 
 * The activated media player cannot make console ouput on client terminal
 * directly. Instead, it prints a log file which can be read and printed out
 * by the client terminal running remote program.
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#ifdef _WIN32
#define REMOTE_EXPORT __declspec(dllexport)
#else
#define REMOTE_EXPORT
#endif

#include "logger.h"

#include "status.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#define PATH_MAX _MAX_PATH
#define sleep(X) Sleep(X)
#else
#include <unistd.h>
#include <linux/limits.h>
#define sleep(X) usleep((X)*1000)
#endif
#define MESSAGE_MAX REMOTE_MESSAGE_MAX


static char logMessage[MESSAGE_MAX];
static long logFilePos = 0L;


/**
 * @brief Appends a log in a logfile
 * 
 * The log file is also used to communicate between 2 programs.
 * It is mainly used for printing messages reported by the display system.
 * 
 * @param fmt Log message
 * @param ... Additional variables to be printed in the format like printf()
 */
REMOTE_EXPORT void remote_log_write(const char *fmt, ...) {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    FILE *fp = fopen(logFile, "a");
    assert(fp != NULL);
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fclose(fp);
}

/**
 * @brief Reads a newly added log from a logfile
 * 
 * The log file is also used to communicate between 2 programs.
 * It is mainly used for printing messages reported by the display system.
 * 
 * @return NULL if there is no new log. Else, a log message as char array.
 */
REMOTE_EXPORT const char *remote_log_read() {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    FILE *fp = fopen(logFile, "r");
    if(fp == NULL)
        return NULL;
    
    // Start reading from previous ended position
    fseek(fp, logFilePos, SEEK_SET);
    int i;
    i = 0;
    char ch = getc(fp);
    while(ch != EOF) {
        logMessage[i] = ch;
        ch = getc(fp);
        i++;
    }
    logMessage[i] = '\0';
    logFilePos = ftell(fp);
    fclose(fp);
    
    if(i == 0)
        return NULL;
    else
        return logMessage;
}

/**
 * @brief Sets the current log position to the end of the file
 */
REMOTE_EXPORT void remote_log_seek_end() {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    FILE *fp = fopen(logFile, "r");
    if(fp == NULL) {
        logFilePos = 0L;
        return;
    }
    
    fseek(fp, 0L, SEEK_END);
    logFilePos = ftell(fp);
    fclose(fp);
}

/**
 * @brief Clears the logfile
 */
REMOTE_EXPORT void remote_log_clear() {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    remove(logFile);
    logFilePos = 0;
}

/**
 * @brief Waits for the response
 * 
 * Stays idle until remote_log_read() gives a new message. When a message
 * is available, printf it out, checks the error code and wait 1 more second
 * for another message.
 * 
 * @param timeout Amount of time to wait
 * 
 * @return Error code
 */
REMOTE_EXPORT int remote_log_wait_response(double timeout) {
    const char *log = NULL;
    int responded = 0;
    int status = 0;
    int k = (int)(timeout / 0.1);
    for(int i=0; i<k; i++) {
        sleep(100);
        log = remote_log_read();
        if(log != NULL) {
            sleep(10);
            printf("%s", log);
            responded = 1;
            k = i + 4;
        }
    }
    
    if(!responded) {
        printf("Media player is not responding\n");
        return 1;
    }
    
    return status;
}
