/**
 * @file post.c
 * @brief Handles the POST requests for mpv-play
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#include "auth.h"
#include "../../libremote/libremote.h"

#include <string.h>
#include <stdlib.h>


int remote_http_handle_post(
    struct MHD_Connection *connection,
    const char *url,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
)
{
    struct RemoteConnection *con_info = *con_cls;
    
    if(strcmp(url, "/command") != 0 && strcmp(url, "/upload") != 0 &&
       strcmp(url, "/authenticate") != 0)
    {
        con_info->status = MHD_HTTP_NOT_FOUND;
        return MHD_NO;
    }
    
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

    // Limit data size for non-blob
    if(strcmp(key, "blob") != 0 && size > 128) {
        con_info->status = MHD_HTTP_BAD_REQUEST;
        return MHD_YES;
    }

    // POST requests that do not require authentication
    if(strcmp(con_info->url, "/authenticate") == 0) {
        if(strcmp(key, "password") == 0) {
            int auth = remote_http_authenticate(con_info, data);
            con_info->status = auth ? MHD_HTTP_OK : MHD_HTTP_UNAUTHORIZED;
            return MHD_NO;
        }
        return MHD_YES;
    }
    
    int auth = remote_http_isAuthenticated(con_info);
    con_info->status = auth ? MHD_HTTP_OK : MHD_HTTP_UNAUTHORIZED;
    if(!auth)
        return MHD_YES;
    
    // POST requests that require authentication
    if(strcmp(con_info->url, "/command") == 0) {
        if(strcmp(key, "command") == 0) {
            remote_command_write(data);
            return MHD_NO;
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
                CreateDirectory("upload", NULL);
                #else
                mkdir("upload", 0700);
                #endif

                // Extract name, extension and file type from full name
                char name[64], ext[6], type[12];
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
                    strncpy(name, filename, 64);
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

                char newfile[96];
                char *json = malloc(96);
                snprintf(newfile, 96, "upload/%s", filename);
                snprintf(json, 96, "{\"url\":\"%s\"}", newfile);
                con_info->reply = json;
                con_info->reply_length = strnlen(json, 96);
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
    
    return MHD_YES;
}