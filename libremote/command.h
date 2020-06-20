/**
 * @file command.h
 * @brief Reads and writes remote commands
 * 
 * The module contains functions for parsing and writing commands.
 * Writes a command in a file and reads the command from the file to make a
 * communication from the remote service to display system.
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#ifndef __MPV_REMOTE_COMMAND_H__
#define __MPV_REMOTE_COMMAND_H__ ///< Header guard

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

#define REMOTE_COMMAND_NONE  0 ///< Null remote command
#define REMOTE_COMMAND_OPEN  1 ///< Opens the media
#define REMOTE_COMMAND_PAUSE 2 ///< Pauses command
#define REMOTE_COMMAND_MOVE  3 ///< Rewinds or skips the media by some time
#define REMOTE_COMMAND_STOP  4 ///< Stops the media
#define REMOTE_COMMAND_KILL  5 ///< Kills the media player

#ifndef REMOTE_MESSAGE_MAX
#define REMOTE_MESSAGE_MAX 1024 ///< Maximum size of a log message
#endif


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
REMOTE_EXPORT void remote_command_write(const char *fmt, ...);

/**
 * @brief Read a command in a file
 * 
 * Writes the given string in a temporary file. This function is usually
 * called by the remote program. The file the remote program writes is
 * read by the display program and the communication is done this way.
 * 
 * @return Command number (REMOTE_COMMAND_NONE, REMOTE_COMMAND_PAUSE,
 *         REMOTE_COMMAND_MOVE or REMOTE_COMMAND_STOP)
 */
REMOTE_EXPORT int remote_command_read();

/**
 * @brief Tokenizes the command line
 * 
 * @param line Command line
 * @param tokens Pointer of token array to assign
 * @param max Size of token array. Maximum count plus one.
 * 
 * @return Number of tokens read
 */
REMOTE_EXPORT int _remote_command_tokenize(char *line, char *tokens[],
                                           int max);

/**
 * @brief Gets the amount of time to rewind or skip
 * 
 * The time value stored is updated when the remote_command_read() function
 * is called and returns REMOTE_COMMAND_MOVE.
 * 
 * @return Time in seconds
 */
REMOTE_EXPORT double remote_command_get_move_request();

/**
 * @brief Gets the requested media URL
 * 
 * The string is updated by the remote_command_read() function with
 * REMOTE_COMMAND_OPEN macro returned.
 * 
 * @return URL string
 */
REMOTE_EXPORT const char* remote_command_get_url_request();

#ifdef __cplusplus
}
#endif

#endif
