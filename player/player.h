/**
 * @file player.h
 * @brief Media display program of MPV Remote
 * 
 * Directly interacts with the MPV API, GUI and media. Unlike traditional
 * media players, the process is made easily accessible by external programs.
 * Status monitoring, pausing and stopping by an external program or command.
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#ifndef __MPV_REMOTE_PLAYER_H__
#define __MPV_REMOTE_PLAYER_H__ ///< Header guard

#include "../libremote/config.h"

#include <mpv/client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enables the libmpv context options suitable for the system
 * 
 * Sets up the options selected for the remote media playing system.
 * The selections are just hard coded.
 * 
 * @param ctx MPV Player context
 * 
 * @return Error code
 */
int remote_player_enable_preset_options(mpv_handle *ctx);

/**
 * @brief Process the MPV event
 * 
 * @param ctx MPV Player context
 * @param event MPV Player event
 */
void remote_player_event_process(mpv_handle *ctx, mpv_event *event);

/**
 * @brief Process the remote command
 * 
 * @param ctx MPV Player context
 * @param cmd Remote command ID
 */
void remote_player_command_process(mpv_handle *ctx, int cmd);

#ifdef __cplusplus
}
#endif

#endif
