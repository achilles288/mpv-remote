/**
 * @file environment
 * @brief Handles the environment variables within the mpv-remote
 *
 * Functions like expanding the variables of a string into the values.
 *
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 *
 * @license{This project is released under the GPL License.}
 */


#ifdef _WIN32
#define REMOTE_EXPORT __declspec(dllexport)
#else
#define REMOTE_EXPORT
#endif

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#include <Shlobj.h>
#define PATH_MAX _MAX_PATH
#else
#include <linux/limits.h>
#endif


/**
 * @brief Translates the url with variable names to the actual file path
 *
 * @param src Input string
 * @param dest Output string
 */
REMOTE_EXPORT void remote_environment_process_variables(const char *src,
                                                        char *dest)
{
    int i = 0;
    int j = 0;
    char c;
    while((c=src[i]) != '\0') {
        if(j == PATH_MAX-1)
            break;
        
        if(c == '$') {
            char value[PATH_MAX];
            #ifdef _WIN32
            if(strncmp(src+i, "${Videos}", 9) == 0) {
                SHGetSpecialFolderPathA(NULL, value, CSIDL_MYVIDEO, 0);
                i += 9;
            }
            else if(strncmp(src+i, "${Music}", 8) == 0) {
                SHGetSpecialFolderPathA(NULL, value, CSIDL_MYMUSIC, 0);
                i += 8;
            }
            #else
            if(strncmp(src+i, "${Videos}", 9) == 0) {
                strcpy(value, getenv("HOME"));
                strcat(value, "/Videos");
                i += 9;
            }
            else if(strncmp(src+i, "${Music}", 8) == 0) {
                strcpy(value, getenv("HOME"));
                strcat(value, "/Music");
                i += 8;
            }
            #endif
            else {
                dest[j++] = '$';
                i++;
                continue;
            }

            char c2;
            int k = 0;
            while((c2=value[k++]) != '\0')
                dest[j++] = c2;
        }
        else {
            dest[j++] = c;
            i++;
        }
    }
    dest[j] = '\0';
}
