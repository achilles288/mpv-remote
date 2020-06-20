/**
 * @file main.c
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#define PATH_MAX _MAX_PATH
#define sleep(X) Sleep(X)
#else
#include <unistd.h>
#include <linux/limits.h>
#define sleep(X) usleep((X)*1000)
#endif


static char *helpMessage =
"Usage:\n"
"    mpv-play [command] [options]\n"
"    \n"
"    command:\n"
"        -h, --help   Prints the help text\n"
"        --version    Shows the MPV Remote version\n"
"        -s, --start  Run the MPV Remote player service\n"
"        -k, --kill   Kill the running process\n"
"    \n"
"    options:\n"
"        -f           Force command\n";


static mpv_handle *ctx = NULL;
static int killRequest = 0;


static void log_error(int code, const char *msg) {
    remote_status_set_error(code, msg);
    remote_status_push();
    remote_log_write(msg);
}


static void log_mpv_error(int code) {
    if(code == 0)
        return;
    char msg[REMOTE_MESSAGE_MAX];
    snprintf(
        msg,
        REMOTE_MESSAGE_MAX,
        "MPV API error: %s\n",
        mpv_error_string(code)
    );
    log_error(code, msg);
}


static void play_exit() {
    if(ctx != NULL)
        mpv_terminate_destroy(ctx);
    log_error(0, "Stopped MPV remote player\n");
    remote_status_set_paused(0);
    remote_status_set_loaded(0);
    remote_status_set_running(0);
    remote_status_push();
}


static void exit_signal_callback(int signum) {
    play_exit();
    exit(signum);
}




int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("%s", helpMessage);
        return 1;
    }
    
    // Prints help message
    if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("%s", helpMessage);
        return 0;
    }
    
    // Prints version info
    if(strcmp(argv[1], "--version") == 0) {
        printf("mpv-remote %s\n", REMOTE_VERSION_STRING);
        return 0;
    }
    
    remote_status_pull();
    
    // Sends command to the active process
    if(strcmp(argv[1], "-k") == 0 || strcmp(argv[1], "--kill") == 0) {
        if(!remote_status_get_running()) {
            printf("No active process to kill\n");
            return 1;
        }
        remote_command_write("kill");
        int res = remote_log_wait_response(1.5);
        if(res != 0) {
            printf("Please open the task manager and kill the process\n");
            remote_status_set_default();
            remote_status_push();
        }
        return res;
    }
    
    // Starts the process
    if(strcmp(argv[1], "-s") != 0 && strcmp(argv[1], "--start") != 0) {
        printf("%s", helpMessage);
        return 1;
    }
    if(remote_status_get_running()) {
        if(argc >= 3 && strcmp(argv[2], "-f") == 0) {
            printf("Force start attempting to kill blocking processes\n");
            remote_command_write("kill");
            remote_log_wait_response(1.5);
        }
        else {
            printf("Another MPV remote player process is already running\n");
            return 1;
        }
    }
    remote_status_set_default();
    remote_status_set_running(1);
    remote_status_push();
    remote_command_read();
    printf("Running MPV remote player\n");
    
    #ifndef _WIN32
    signal(SIGHUP, exit_signal_callback);
    #endif
    signal(SIGINT, exit_signal_callback);
    signal(SIGTERM, exit_signal_callback);
    
    // Waits until a media open command is sent
    while(!killRequest) {
        sleep(1000);
        int cmd = remote_command_read();
        if(cmd == REMOTE_COMMAND_OPEN) {
            const char *url = remote_command_get_url_request();
            remote_status_set_url((char*)url);
            int type = remote_status_get_media_type();
            
            // Creates the context
            mpv_handle *ctx = mpv_create();
            if(!ctx) {
                log_error(1, "Failed creating context\n");
                continue;
            }
            int res;
            res = remote_player_enable_preset_options(ctx);
            if(res != 0) {
                log_mpv_error(res);
                mpv_terminate_destroy(ctx);
                continue;
            }
            
            // Initializes the context
            res = mpv_initialize(ctx);
            if(res != 0) {
                log_mpv_error(res);
                mpv_terminate_destroy(ctx);
                continue;
            }
            
            // Sets MPV file loading command
            const char *play_cmd[] = {"loadfile", url, NULL};
            res = mpv_command(ctx, play_cmd);
            if(res != 0) {
                log_mpv_error(res);
                mpv_terminate_destroy(ctx);
                continue;
            }
            remote_status_set_error(0, "");
            remote_status_push();
            
            /**
             * Plays the requested media.
             * Handles commands and events.
             */
            double waitTime = 0;
            while(1) {
                mpv_event* event = mpv_wait_event(ctx, 0.1);
                remote_player_event_process(ctx, event);
                if(event->event_id == MPV_EVENT_SHUTDOWN ||
                   event->event_id == MPV_EVENT_END_FILE)
                {
                    break;
                }
                
                // Aborts the process if the loading is taking long
                if(!remote_status_get_loaded()) {
                    double timeout = 5.0;
                    if(type == REMOTE_MEDIA_HTTP)
                        timeout = 30.0;
                    
                    if(waitTime > timeout) {
                        char msg[REMOTE_MESSAGE_MAX];
                        snprintf(
                            msg,
                            REMOTE_MESSAGE_MAX,
                            "Error loading media `%s`\n",
                            url
                        );
                        log_error(1, msg);
                        break;
                    }
                    waitTime += 0.1;
                }
                
                // Reads and proceeds the command given by the remote
                cmd = remote_command_read();
                remote_player_command_process(ctx, cmd);
                if(cmd == REMOTE_COMMAND_STOP)
                    break;
                else if(cmd == REMOTE_COMMAND_KILL) {
                    killRequest = 1;
                    break;
                }
                
                remote_status_push();
            }
            mpv_terminate_destroy(ctx);
            ctx = NULL;
            remote_status_set_loaded(0);
            remote_status_push();
            remote_log_write("Finished playing the media\n");
        }
        else if(cmd == REMOTE_COMMAND_KILL) {
            killRequest = 1;
        }
    }
    play_exit();
    return 0;
}
