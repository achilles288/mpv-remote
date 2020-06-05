#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#define sleep(X) Sleep(X)
#else
#include <unistd.h>
#include <libgen.h>
#define sleep(X) usleep((X)*1000)
#endif


static char *temp_path;
static char exec_path[128];

static void get_exec_path(char* buff, size_t len) {
#ifdef _WIN32
    GetModuleFileName(NULL, buff, (DWORD)len);
    char* q = NULL;
    char* p = buff + 1;
    while (*p != '\0') {
        if (*p == '\\')
            q = p;
        p++;
    }
    if (q != NULL)
        *q = '\0';
    #else
    if (readlink("/proc/self/exe", buff, len) != -1)
        dirname(buff);
    else
        buff[0] = '\0';
    #endif
}

static void write_command(char* cmd) {
    char cmd_file[128];
    #ifdef _WIN32
    snprintf(cmd_file, 128, "%s\\mpv-command", temp_path);
    #else
    snprintf(cmd_file, 128, "/tmp/mpv-command");
    #endif
    FILE *fp = fopen(cmd_file, "w");
    fprintf(fp, "%s", cmd);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        return 1;
    }
    
    temp_path = getenv("TEMP");
    #ifndef _WIN32
    get_exec_path(exec_path, 128);
    #endif
    
    if(strcmp(argv[1], "-p") == 0) {
        write_command("pause");
    }
    else if(strcmp(argv[1], "-m") == 0) {
        if(argc != 3) {
            printf("Need to specify time to move in seconds.\n");
            return 1;
        }
        char buff[12];
        snprintf(buff, 12, "move %s", argv[2]);
        write_command(buff);
    }
    else if(strcmp(argv[1], "-k") == 0) {
        write_command("quit");
    }
    else {
        FILE *fp;
        char play_indicator[128];
        #ifdef _WIN32
        snprintf(play_indicator, 128, "%s\\mpv-play", temp_path);
        #else
        snprintf(play_indicator, 128, "/tmp/mpv-play");
        #endif
        fp = fopen(play_indicator, "r");
        if(fp != NULL) {
            fclose(fp);
            write_command("quit");
            
            int i=0;
            while(1) {
                sleep(100);
                fp = fopen(play_indicator, "r");
                if(fp == NULL)
                    break;
                fclose(fp);
                
                if(i == 10)
                    break;
                i++;
            }
        }
        write_command("none");
        
        char sys_cmd[256];
        #ifdef _WIN32
        snprintf(sys_cmd, 256, "start mpv-play %s", argv[1]);
        #else
        snprintf(sys_cmd, 256, "%s/mpv-play %s &", exec_path, argv[1]);
        #endif
        system(sys_cmd);
    }
    
    return 0;
}
