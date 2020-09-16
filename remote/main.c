/**
 * @file mpv-remote.c
 * @brief MPV Remote command sender program
 * 
 * Passes commands from the user to the display program by means of file. 
 * Command is sent by writing string in a text file which is then read by
 * the active display program. Monitors the status of the media player from
 * the JSON file written by the second program.
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#include "../libremote/libremote.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#define PATH_MAX _MAX_PATH
#else
#include <linux/limits.h>
#endif

static char *helpMessageBrief =
"Usage:\n"
"    mpv-remote [url]\n"
"    mpv-remote [command] [args]\n";

static char *helpMessage =
"Usage:\n"
"    mpv-remote [url]\n"
"    mpv-remote [command] [args]\n"
"    \n"
"    url:\n"
"        Supports files from local and web. For local files, use relative or\n"
"        absolute file path. For files over http, the app uses a software\n"
"        called youtube-dl.\n"
"    \n"
"    command:\n"
"        -h, --help         Prints the help text\n"
"        --version          Shows the MPV Remote version\n"
"        --status           Prints the media player status\n"
"        -p, --pause        Pauses or resumes the current media\n"
"        -m, --move [time]  Rewinds or skips the current media in seconds\n"
"        -s, --stop         Stops the current media\n";


int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("%s", helpMessageBrief);
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
    
    // Prints the player status
    if(strcmp(argv[1], "--status") == 0) {
        remote_status_pull();
        remote_status_print();
        return 0;
    }
    
    remote_status_pull();
    if(remote_status_get_running() == 0) {
        printf("The media player is not running\n"
               "Please run `mpv-play --start` on the host PC\n");
        return 1;
    }
    
    remote_log_seek_end();
    double timeout = 1.0;
    
    /**
     * Processes the commands.
     */
    if(strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "--pause") == 0) {
        if(!remote_status_get_loaded()) {
            printf("A media is not being played\n");
            return 1;
        }
        remote_command_write("pause");
    }
    else if(strcmp(argv[1], "-m") == 0 || strcmp(argv[1], "--move") == 0) {
        if(argc != 3) {
            printf("Please specify time to move in seconds\n");
            return 1;
        }
        if(!remote_status_get_loaded()) {
            printf("A media is not being played\n");
            return 1;
        }
        
        // Prints the amount of time moved
        double time;
        sscanf(argv[2], "%lf", &time);
        if(time < 0)
            printf("Rewinded the media by %d seconds\n", (int) (-time+0.5));
        else
            printf("Skipped the media by %d seconds\n", (int) (time+0.5));
        
        remote_command_write("move %lf", time);
        return 0;
    }
    else if(strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--stop") == 0) {
        if(!remote_status_get_loaded()) {
            printf("A media is not being played\n");
            return 1;
        }
        remote_command_write("stop");
    }
    else {
        remote_status_set_url(argv[1]);
        const char *url = argv[1];
        int type = remote_status_get_media_type();
        
        // Check file availability
        if(type == REMOTE_MEDIA_LOCAL) {
            FILE *fp = fopen(url, "r");
            if(fp == NULL) {
                printf("Media `%s` does not exist\n", url);
                return 1;
            }
            fclose(fp);
        }
        
        // Abort if a media is already being played
        if(remote_status_get_loaded()) {
            printf("A media is already being played\n");
            return 1;
        }
        
        timeout = 6.0;
        if(type == REMOTE_MEDIA_HTTP)
            timeout = 31.0;
        
        if(type == REMOTE_MEDIA_LOCAL) {
            char absPath[PATH_MAX];
            #ifdef _WIN32
            _fullpath(absPath, url, PATH_MAX);
            #else
            realpath(url, absPath);
            #endif
            remote_command_write("open \"%s\"", absPath);
        }
        else {
            remote_command_write("open \"%s\"", url);
        }
    }
    
    // Waits for any response from the display program
    return remote_log_wait_response(timeout);
}
