/**
 * @file libremote.h
 * @brief Command-line-controlled media player
 * 
 * Functions sync and update the status attributes, read and write commands,
 * making a communication between the remote service and display system by
 * files. To be able to control a running media, the command prompt should
 * not be blocked by the running media and two separate programs are to run
 * concurrently.
 * 
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 * 
 * @license{This project is released under the GPL License.}
 */


#ifndef __MPV_REMOTE_H__
#define __MPV_REMOTE_H__ ///< Header guard

#ifndef REMOTE_EXPORT
#ifdef _WIN32
#define REMOTE_EXPORT __declspec(dllimport)
#else
#define REMOTE_EXPORT
#endif
#endif

#include "config.h"
#include "command.h"
#include "logger.h"
#include "status.h"

#include "cmd_rsp/cmd_rsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Translates the url with variable names to the actual file path
 *
 * @param src Input string
 * @param dest Output string
 */
REMOTE_EXPORT void remote_environment_process_variables(const char* src,
                                                        char* dest);

#ifdef __cplusplus
}
#endif

#endif
