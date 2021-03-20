/**
 * @file post.c
 * @brief Handles the POST requests for mpv-play
 * 
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 * 
 * @license{This project is released under the GPL License.}
 */


#include "../../libremote/libremote.h"
#include "auth.h"

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#endif


int remote_http_handle_post(
    struct MHD_Connection *connection,
    const char *url,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
)
{
    struct RemoteConnection *con_info = *con_cls;
    int match = 0;
    
    static const char* listedURL1[] = {
        "/command", "/upload", "/authenticate", "/change-password"
    };

    for(int i=0; i<4; i++) {
        if(strcmp(url, listedURL1[i]) == 0) {
            match = 1;
            break;
        }
    }
    if(!match) {
        con_info->status = MHD_HTTP_NOT_FOUND;
        return MHD_NO;
    }

    // The requested URL requires no authentication
    match = (strcmp(url, "/authenticate") == 0);

    // If the requested URL requires authentication
    if(!match) {
        if(!remote_http_is_authenticated(con_info)) {
            con_info->status = MHD_HTTP_UNAUTHORIZED;
            return MHD_NO;
        }
    }
    
    // Processes the request
    if(*upload_data_size != 0) {
        MHD_post_process(con_info->postprocessor, upload_data,	
	                     *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }
    else
        return MHD_NO;
}


int remote_http_iterate_post(
    void *coninfo_cls,
    enum MHD_ValueKind kind,
    const char *key,
    const char *filename,
    const char *content_type,
    const char *transfer_encoding,
    const char *data,
    uint64_t off,
    size_t size
)
{
    struct RemoteConnection* con_info = coninfo_cls;

    if(strcmp(con_info->url, "/authenticate") == 0) {
        if(strcmp(key, "password") == 0) {
            int auth = remote_http_authenticate(con_info, data);
            con_info->status = auth ? MHD_HTTP_OK : MHD_HTTP_UNAUTHORIZED;
            return MHD_YES;
        }
    }
    else if(strcmp(con_info->url, "/change-password") == 0) {
        int rightKey = 0;
        if(strcmp(key, "old-password") == 0) {
            strcpy(con_info->param1, data);
            rightKey = 1;
        }
        else if(strcmp(key, "password") == 0) {
            strcpy(con_info->param2, data);
            rightKey = 1;
        }

        if(strlen(con_info->param1) && strlen(con_info->param2)) {
            int auth = remote_http_change_password(
                con_info->param1,
                con_info->param2
            );
            con_info->status = auth ? MHD_HTTP_OK : MHD_HTTP_UNAUTHORIZED;
            return MHD_YES;
        }
        else if(rightKey)
            return MHD_YES;

    }
    else if(strcmp(con_info->url, "/command") == 0) {
        if(strcmp(key, "command") == 0) {
            remote_command_write(data);
            con_info->status = MHD_HTTP_OK;
            return MHD_YES;
        }
    }
    else if(strcmp(con_info->url, "/upload") == 0) {
        if(strcmp(key, "blob") == 0) {
            if(con_info->fp == NULL) {
                if(filename == NULL) {
                    con_info->status = MHD_HTTP_BAD_REQUEST;
                    return MHD_YES;
                }
                #ifdef _WIN32
                CreateDirectory("uploads", NULL);
                #else
                mkdir("uploads", 0700);
                #endif

                // Extract name, extension and file type from full name
                char name[PATH_MAX], ext[6], type[12];
                strcpy(ext, "");
                strcpy(type, "");
                char* ptr;
                ptr = strrchr(filename, '.');
                if(ptr != NULL) {
                    strncpy(ext, ptr, 6);
                    memcpy(name, filename, ptr-filename);
                    name[ptr-filename] = '\0';
                }
                else
                    strncpy(name, filename, PATH_MAX);
                ptr = strrchr(content_type, '/');
                if(ptr != NULL) {
                    memcpy(type, content_type, ptr-content_type);
                    type[ptr-content_type] = '\0';
                }

                // Checks if the media type is a video
                if(strcmp(type, "video") != 0) {
                    con_info->status = MHD_HTTP_UNSUPPORTED_MEDIA_TYPE;
                    return MHD_YES;
                }

                char newfile[PATH_MAX];
                char *json = malloc(PATH_MAX);
                snprintf(newfile, PATH_MAX-1, "uploads/%s", filename);
                snprintf(json, PATH_MAX-1, "{\"url\":\"%s\"}", newfile);
                con_info->reply = json;
                con_info->reply_length = strlen(json);
                con_info->status = MHD_HTTP_OK;
                strcpy(con_info->content_type, "application/json");
                FILE *fp = fopen(newfile, "rb");

                if(fp != NULL) {
                    fclose(fp);
                    return MHD_YES;
                }
                else
                    con_info->fp = fopen(newfile, "wb");
            }

            // Writes to the new file
            if(size > 0) {
                if(!fwrite(data, size, 1, con_info->fp))
                    return MHD_NO;
            }
            return MHD_YES;
        }
    }
    
    if(con_info->status == 0)
        con_info->status = MHD_HTTP_BAD_REQUEST;
    return MHD_YES;
}