/**
 * @file logger.h
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


#ifndef __MPV_REMOTE_LOGGER_H__
#define __MPV_REMOTE_LOGGER_H__ ///< Header guard

#ifndef REMOTE_EXPORT
#ifdef _WIN32
#define REMOTE_EXPORT __declspec(dllimport)
#else
#define REMOTE_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef REMOTE_MESSAGE_MAX
#define REMOTE_MESSAGE_MAX 1024 ///< Maximum size of a log message
#endif


/**
 * @brief Appends a log in a logfile
 * 
 * The log file is also used to communicate between 2 programs.
 * It is mainly used for printing messages reported by the display system.
 * 
 * @param fmt Log message
 * @param ... Additional variables to be printed in the format like printf()
 */
REMOTE_EXPORT void remote_log_write(const char *msg, ...);

/**
 * @brief Reads a newly added log from a logfile
 * 
 * The log file is also used to communicate between 2 programs.
 * It is mainly used for printing messages reported by the display system.
 * 
 * @return NULL if there is no new log. Else, a log message as char array.
 */
REMOTE_EXPORT const char *remote_log_read();

/**
 * @brief Sets the current log position to the end of the file
 */
REMOTE_EXPORT void remote_log_seek_end();

/**
 * @brief Clears the logfile
 */
REMOTE_EXPORT void remote_log_clear();

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
REMOTE_EXPORT int remote_log_wait_response(double timeout);

#ifdef __cplusplus
}
#endif

#endif
