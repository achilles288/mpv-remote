/**
 * @file player.c
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


#include "player.h"

#include "../libremote/libremote.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#define PATH_MAX _MAX_PATH
#else
#include <unistd.h>
#include <linux/limits.h>
#endif

#include <mpv/client.h>


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
int remote_player_enable_preset_options(mpv_handle *ctx) {
    int res;
    int flagVal;
    
    // Forces full screen mode
    flagVal = 1;
    mpv_set_option(ctx, "fs", MPV_FORMAT_FLAG, &flagVal);
    
    // Enables GPU hardware decoding
    res = mpv_set_option_string(ctx, "hwdec", "auto");
    if(res != 0)
        return res;
    
    // Sets default key input
    flagVal = 1;
    mpv_set_option(ctx, "input-default-bindings", MPV_FORMAT_FLAG, &flagVal);
    
    // Allows keyboard input on host device
    flagVal = 1;
    mpv_set_option(ctx, "input-vo-keyboard", MPV_FORMAT_FLAG, &flagVal);
    
    mpv_set_option(ctx, "osc", MPV_FORMAT_FLAG, &flagVal);
    
    return 0;
}

/**
 * @brief Process the MPV event
 * 
 * @param ctx MPV Player context
 * @param event MPV Player event
 */
void remote_player_event_process(mpv_handle *ctx, mpv_event *event) {
    static double time;
    
    if(event->event_id == MPV_EVENT_FILE_LOADED) {
        char *name;
        name = mpv_get_property_string(ctx, "media-title");
        double duration;
        mpv_get_property(ctx, "duration", MPV_FORMAT_DOUBLE, &duration);
        remote_status_set_name(name);
        remote_status_set_duration(duration);
        remote_status_set_loaded(1);
        remote_log_write("Playing media `%s`\n", name);
    }
    else if(event->event_id == MPV_EVENT_PAUSE) {
        if(!remote_status_get_paused()) {
            remote_log_write("Paused the media\n");
            remote_status_set_paused(1);
        }
    }
    else if(event->event_id == MPV_EVENT_UNPAUSE) {
        if(remote_status_get_paused()) {
            remote_log_write("Resumed the media\n");
            remote_status_set_paused(0);
        }
    }
    else if(event->event_id == MPV_EVENT_SEEK) {
        double t2;
        mpv_get_property(ctx, "time-pos", MPV_FORMAT_DOUBLE, &t2);
        double moveTime = t2 - time;
        if(moveTime < 0) {
            remote_log_write("Rewinded the media by %d seconds\n",
                             (int) (-moveTime+0.5));
        }
        else {
            remote_log_write("Skipped the media by %d seconds\n",
                             (int) (moveTime+0.5));
        }
    }
    else if(remote_status_get_loaded()) {
        mpv_get_property(ctx, "time-pos", MPV_FORMAT_DOUBLE, &time);
        remote_status_set_time(time);
    }
}

/**
 * @brief Process the remote command
 * 
 * @param ctx MPV Player context
 * @param cmd Remote command ID
 */
void remote_player_command_process(mpv_handle *ctx, int cmd) {
    if(cmd == REMOTE_COMMAND_PAUSE) {
        if(!remote_status_get_loaded())
            return;
        int paused;
        mpv_get_property(ctx, "pause", MPV_FORMAT_FLAG, &paused);
        paused = !paused;
        mpv_set_property(ctx, "pause", MPV_FORMAT_FLAG, &paused);
    }
    else if(cmd == REMOTE_COMMAND_MOVE) {
        if(!remote_status_get_loaded())
            return;
        double moveTime = remote_command_get_move_request();
        double time = remote_status_get_time() + moveTime;
        mpv_set_property(ctx, "time-pos", MPV_FORMAT_DOUBLE, &time);
    }
}
