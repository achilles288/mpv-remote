#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#else
#include <unistd.h>
#include <libgen.h>
#endif

#include <mpv/client.h>


#define COMMAND_NONE  0
#define COMMAND_PAUSE 1
#define COMMAND_MOVE  2
#define COMMAND_QUIT  3


static char exec_path[128];

static void get_exec_path(char* buff, size_t len) {
    #ifdef _WIN32
    
    #else
    if (readlink("/proc/self/exe", buff, len) != -1)
        dirname(buff);
    else
        buff[0] = '\0';
    #endif
}

static int move_time = 0;

static int read_command() {
    char cmd_file[128+8];
    #ifdef _WIN32
    sprintf(cmd_file, "%s/command", exec_path);
    #else
    sprintf(cmd_file, "/tmp/mpv-command");
    #endif
    FILE *fp = fopen(cmd_file, "r");
    if(fp == NULL)
        return COMMAND_NONE;
    
    char cmd[64];
    int i;
    i = 0;
    char ch = getc(fp);
    while(ch != EOF) {
        cmd[i] = ch;
        ch = getc(fp);
        i++;
    }
    cmd[i] = '\0';
    fclose(fp);
    remove(cmd_file);
    
    char *tokens[5];
    char *tok = strtok(cmd, " ");
    tokens[0] = tok;
    i = 0;
    while(tok != NULL) {
        tok = strtok(NULL, " ");
        tokens[++i] = tok;
    }
    
    if(strcmp(tokens[0], "pause") == 0) {
        return COMMAND_PAUSE;
    }
    else if(strcmp(tokens[0], "move") == 0) {
        move_time = atoi(tokens[1]);
        return COMMAND_MOVE;
    }
    else if(strcmp(tokens[0], "quit") == 0) {
        return COMMAND_QUIT;
    }
    else {
        return COMMAND_NONE;
    }
}

static void check_error(int status) {
    if(status < 0) {
        printf("MPV API error: %s\n", mpv_error_string(status));
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("Pass a single media file as argument\n");
        return 1;
    }
    
    get_exec_path(exec_path, 128);
    
    mpv_handle *ctx = mpv_create();
    if(!ctx) {
        printf("Failed creating context\n");
        return 1;
    }
    
    check_error(mpv_set_option_string(ctx, "fs", "yes"));
    check_error(mpv_set_option_string(ctx, "hwdec", "auto"));
    check_error(mpv_set_option_string(ctx, "input-default-bindings", "yes"));
    check_error(mpv_set_option_string(ctx, "input-vo-keyboard", "yes"));
    int val = 1;
    check_error(mpv_set_option(ctx, "osc", MPV_FORMAT_FLAG, &val));
    check_error(mpv_initialize(ctx));
    
    // Play this file.
    const char *play_cmd[] = {"loadfile", argv[1], NULL};
    check_error(mpv_command(ctx, play_cmd));
    
    // Let it play, and wait until the user quits.
    char play_indicator[128+5];
    #ifdef _WIN32
    sprintf(play_indicator, "%s/play", exec_path);
    #else
    sprintf(play_indicator, "/tmp/mpv-play");
    #endif
    FILE *fp = fopen(play_indicator, "w");
    fprintf(fp, "play");
    fclose(fp);
    while(1) {
        mpv_event *event = mpv_wait_event(ctx, 0.1);
        if(event->event_id == MPV_EVENT_SHUTDOWN)
            break;
        
        int cmd = read_command();
        if(cmd == COMMAND_PAUSE) {
            const char *cmd[] = {"keypress", "p", NULL};
            check_error(mpv_command(ctx, cmd));
        }
        else if(cmd == COMMAND_MOVE) {
            char buff[6];
            sprintf(buff, "%d", move_time);
            const char *cmd[] = {"seek", buff, NULL};
            check_error(mpv_command(ctx, cmd));
        }
        else if(cmd == COMMAND_QUIT) {
            const char *cmd[] = {"quit", NULL};
            check_error(mpv_command(ctx, cmd));
        }
    }
    
    remove(play_indicator);
    mpv_terminate_destroy(ctx);
    return 0;
}
