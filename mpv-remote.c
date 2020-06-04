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

static void write_command(char* cmd) {
    char cmd_file[128+8];
    #ifdef _WIN32
    sprintf(cmd_file, "%s/command", exec_path);
    #else
    sprintf(cmd_file, "/tmp/mpv-command");
    #endif
    FILE *fp = fopen(cmd_file, "w");
    fprintf(fp, "%s", cmd);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        return 1;
    }
    
    get_exec_path(exec_path, 128);
    
    if(strcmp(argv[1], "-p") == 0) {
        write_command("pause");
    }
    else if(strcmp(argv[1], "-m") == 0) {
        if(argc != 3) {
            printf("Need to specify time to move in seconds.\n");
            return 1;
        }
        char buff[12];
        sprintf(buff, "move %s", argv[2]);
        write_command(buff);
    }
    else if(strcmp(argv[1], "-k") == 0) {
        write_command("quit");
    }
    else {
        FILE *fp;
        char play_indicator[128+5];
        #ifdef _WIN32
        sprintf(play_indicator, "%s/play", exec_path);
        #else
        sprintf(play_indicator, "/tmp/mpv-play");
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
            
            write_command("none");
        }
        
        char sys_cmd[256];
        #ifdef _WIN32
        sprintf(sys_cmd, "start %s/mpv-play %s", exec_path, argv[1]);
        #else
        sprintf(sys_cmd, "%s/mpv-play %s &", exec_path, argv[1]);
        #endif
        system(sys_cmd);
    }
    
    return 0;
}
