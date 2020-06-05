#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <libgen.h>
#endif

#include <mpv/client.h>


#define COMMAND_NONE  0
#define COMMAND_PAUSE 1
#define COMMAND_MOVE  2
#define COMMAND_QUIT  3


static char *temp_path;
static int move_time = 0;

static int read_command() {
    char cmd_file[128];
    #ifdef _WIN32
    snprintf(cmd_file, 128, "%s\\mpv-command", temp_path);
    #else
    snprintf(cmd_file, 128, "/tmp/mpv-command");
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
    
    temp_path = getenv("TEMP");
    
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
    int is_from_web = 0;
    double load_timeout = 10.0;
    if(strlen(argv[1]) > 20) {
        char buff[9];
        memcpy(buff, argv[1], 8);
        buff[8] = '\0';
        if(strcmp(buff, "https://") == 0) {
            is_from_web = 1;
            load_timeout = 30.0;
        }
    }
    
    if(!is_from_web) {
        FILE* fp = fopen(argv[1], "r");
        if(fp == NULL) {
            printf("Media file %s does not exist.", argv[1]);
            return 1;
        }
        fclose(fp);
    }
    
    const char *play_cmd[] = {"loadfile", argv[1], NULL};
    check_error(mpv_command(ctx, play_cmd));
    
    // Let it play, and wait until the user quits.
    char play_indicator[128];
    #ifdef _WIN32
    snprintf(play_indicator, 128, "%s\\mpv-play", temp_path);
    #else
    snprintf(play_indicator, 128, "/tmp/mpv-play");
    #endif
    FILE *fp = fopen(play_indicator, "w");
    fprintf(fp, "play");
    fclose(fp);
    double t = 0;
    int loaded = 0;
    int paused = 0;
    while(1) {
        mpv_event* event = mpv_wait_event(ctx, 0.1);
        t += 0.1;
        if(event->event_id == MPV_EVENT_FILE_LOADED) {
            printf("Playing %s\n", argv[1]);
            loaded = 1;
        }
        if(event->event_id == MPV_EVENT_PAUSE) {
            if(!paused)
                printf("Paused\n");
            paused = 1;
        }
        if(event->event_id == MPV_EVENT_UNPAUSE) {
            if(paused)
                printf("Resumed\n");
            paused = 0;
        }
        if(event->event_id == MPV_EVENT_SHUTDOWN)
            break;

        if(loaded == 0 && t > load_timeout) {
            printf("Error loading media %s\n", argv[1]);
            break;
        }
        
        int cmd = read_command();
        if(cmd == COMMAND_PAUSE) {
            const char *cmd[] = {"keypress", "p", NULL};
            check_error(mpv_command(ctx, cmd));
        }
        else if(cmd == COMMAND_MOVE) {
            printf("Moved media by %d seconds\n", move_time);
            char buff[6];
            snprintf(buff, 6, "%d", move_time);
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
    printf("Exit\n");
    return 0;
}
