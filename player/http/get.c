/**
 * @file get.c
 * @brief Handles the GET requests for mpv-play
 * 
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 * 
 * @license{This project is released under the GPL License.}
 */


#include "../../libremote/libremote.h"
#include "auth.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#include <tchar.h>
#define PATH_MAX _MAX_PATH
#endif

#include <json.h>
#include <microhttpd.h>


static char *load_file(const char *file, const char *mode, size_t *len) {
    FILE *fp = fopen(file, mode);
    if(fp == NULL)
        return NULL;
    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char *buffer = malloc(file_size+1);
    size_t i = 0;

    if(strncmp(mode, "r", 2) == 0) {
        char c;
        while((c = getc(fp)) != EOF)
            buffer[i++] = c;
    }
    else {
        while(i < file_size)
            buffer[i++] = getc(fp);
    }
    buffer[i] = '\0';
    fclose(fp);
    
    if(len != NULL)
        *len = i;

    return buffer;
}




static void error_answer(struct RemoteConnection *con_info, int error_code) {
    static const char error_html[] =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "  <title>%s</title>"
        "</head>"
        "<body>"
        "  <h1>%s</h1>"
        "</body>"
        "</html>";
    
    // Constructs an error message
    strcpy(con_info->content_type, "text/html");
    con_info->status = error_code;
    con_info->reply = malloc(512);
    size_t len;
    if(error_code == MHD_HTTP_NOT_FOUND) {
        len = sprintf(con_info->reply, error_html, "404 Not Found",
                      "This URL isn't available.");
    }
    else if (error_code == MHD_HTTP_UNAUTHORIZED) {
        len = sprintf(con_info->reply, error_html, "Unauthorized",
                      "You need to be authenticated to proceed the request.");
    }
    else if(error_code == MHD_HTTP_UNSUPPORTED_MEDIA_TYPE) {
        len = sprintf(con_info->reply, error_html, "Unsupported Media",
                      "This media type isn't supported.");
    }
    else if(error_code == MHD_HTTP_INTERNAL_SERVER_ERROR) {
        len = sprintf(con_info->reply, error_html, "Error 500",
                      "Sorry, an internal server error occured.");
    }
    else {
        len = sprintf(con_info->reply, error_html, "Error",
                      "An error occured.");
    }
    con_info->reply_length = len;
}




struct MimeType {
    char ext[6];
    char alt[16];
    char mediatype[12];
    int binary;
};


static void answer_to_get_special(
    struct MHD_Connection *connection,
    const char *url,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

static char *browse_directory(const char *path, size_t *len);
static char *list_drives(size_t *len);
static char *get_thumbnail(const char *file, size_t *len);




void remote_http_handle_get(
    struct MHD_Connection *connection,
    const char *url,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
)
{
    struct RemoteConnection *con_info = *con_cls;
    
    // Checks if the url is a special GET request
    answer_to_get_special(connection, url, upload_data,
                          upload_data_size, con_cls);
    if(con_info->reply || con_info->status != 0)
        return;
    
    char file_path[128];
    snprintf(file_path, 128, PREFIX "%s", url);
    char *ext = strrchr(url, '.');
    if(ext == NULL) {
        error_answer(con_info, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE);
        return;
    }
    ext += 1;
    
    // Checks the extensions to determine the mimetype
    const struct MimeType mimetypes[] = {
        {"html", "html", "text", 0},
        {"css", "css", "text", 0},
        {"js", "javascript", "text", 0},
        {"png", "png", "image", 1},
        {"ico", "icon", "image", 1},
        {"svg", "svg", "image", 0},
        {"ttf", "ttf", "font", 1},
        {"woff", "woff", "font", 1},
        {"woff2", "woff2", "font", 1},
    };
    int binary = 0;
    int isSupported = 0;

    for(int i=0; i<9; i++) {
        if(strncmp(ext, mimetypes[i].ext, 5) == 0) {
            char *alt = mimetypes[i].alt;
            char *mediatype = mimetypes[i].mediatype;
            snprintf(con_info->content_type, 24, "%s/%s", mediatype, alt);
            binary = mimetypes[i].binary;
            isSupported = 1;
            break;
        }
    }

    if(!isSupported) {
        error_answer(con_info, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE);
        return;
    }
    
    // Reads the file and loads the content to a buffer
    size_t file_size = 0;
    con_info->reply = load_file(file_path, binary?"rb":"r", &file_size);
    if(con_info->reply == NULL) {
        error_answer(con_info, MHD_HTTP_NOT_FOUND);
        return;
    }
    con_info->reply_length = file_size;
    con_info->status = MHD_HTTP_OK;
}




static void answer_to_get_special(
    struct MHD_Connection *connection,
    const char *url,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
)
{
    if(strcmp(url, "/") == 0)
        url = "/index.html";
    char file_path[128];
    snprintf(file_path, 128, PREFIX "%s", url);
    
    // Prepares the page footer
    static char footer_content[4096];
    static size_t footer_content_size = 0;
    static int footer_read = 0;
    if(!footer_read) {
        size_t file_size;
        char *buffer = load_file(PREFIX "/footer.html", "r", &file_size);
        if(buffer != NULL) {
            int ret = snprintf(footer_content, 4096, buffer,
                               REMOTE_VERSION_STRING);
            free(buffer);
            if(ret == -1) {
                footer_content[0] = '\0';
                footer_content_size = 0;
            }
            else {
                footer_content_size = ret;
            }
        }
        else
            footer_content[0] = '\0';
        footer_read = 1;
    }
    
    struct RemoteConnection *con_info = *con_cls;
    strcpy(con_info->content_type, "text/html");
    
    // Serves the special URLs
    if(strcmp(url, "/index.html") == 0 || strcmp(url, "/settings.html") == 0)
    {
        size_t file_size;
        char *buffer = load_file(file_path, "r", &file_size);
        if(buffer == NULL) {
            error_answer(con_info, MHD_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
        con_info->reply = malloc(file_size+footer_content_size);
        size_t len = snprintf(con_info->reply, file_size+footer_content_size,
                              buffer, footer_content);
        con_info->reply_length = len;
        con_info->status = MHD_HTTP_OK;
        free(buffer);
    }
    else if(strcmp(url, "/browse") == 0) {
        if(!remote_http_is_authenticated(con_info)) {
            error_answer(con_info, MHD_HTTP_UNAUTHORIZED);
            return;
        }
        
        const char *p = MHD_lookup_connection_value(
            connection,
            MHD_GET_ARGUMENT_KIND,
            "path"
        );

        size_t json_length = 0;

        if(p == NULL) {
            con_info->reply = list_drives(&json_length);
        }
        else {
            char path[PATH_MAX];
            remote_environment_process_variables(p, path);
            con_info->reply = browse_directory(path, &json_length);
        }

        con_info->reply_length = json_length;
        con_info->status = MHD_HTTP_OK;
        strcpy(con_info->content_type, "application/json");
    }
    else if(strcmp(url, "/thumbnail") == 0) {
        if(!remote_http_is_authenticated(con_info)) {
            error_answer(con_info, MHD_HTTP_UNAUTHORIZED);
            return;
        }

        const char* p = MHD_lookup_connection_value(
            connection,
            MHD_GET_ARGUMENT_KIND,
            "file"
        );

        size_t file_size = 0;

        if(p == NULL) {
            error_answer(con_info, MHD_HTTP_BAD_REQUEST);
            return;
        }
        else {
            char file_path[PATH_MAX];
            remote_environment_process_variables(p, file_path);
            con_info->reply = get_thumbnail(file_path, &file_size);
        }

        if(con_info->reply == NULL) {
            error_answer(con_info, MHD_HTTP_NOT_FOUND);
            return;
        }

        con_info->reply_length = file_size;
        con_info->status = MHD_HTTP_OK;
        strcpy(con_info->content_type, "image/png");
    }
    else if(strcmp(url, "/is-authenticated") == 0) {
        strcpy(con_info->content_type, "");
        int auth = remote_http_is_authenticated(con_info);
        con_info->status = auth ? MHD_HTTP_OK : MHD_HTTP_UNAUTHORIZED;
    }
    else if(strcmp(url, "/status") == 0) {
        strcpy(con_info->content_type, "application/json");
        #ifdef _WIN32
        char jsonFile[PATH_MAX];
        snprintf(jsonFile, PATH_MAX, "%s\\mpv-status.json", getenv("TEMP"));
        #else
        const char *jsonFile = "/tmp/mpv-status.json";
        #endif
        
        if(remote_http_is_authenticated(con_info)) {
            size_t file_size;
            char *json = load_file(jsonFile, "r", &file_size);
            if(json == NULL) {
                error_answer(con_info, MHD_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }
            con_info->reply = json;
            con_info->reply_length = file_size;
            con_info->status = MHD_HTTP_OK;
        }
        else
            error_answer(con_info, MHD_HTTP_UNAUTHORIZED);
    }
}




static char* browse_directory(const char* path, size_t* len) {
    #ifdef _WIN32
    WIN32_FIND_DATA ffd;
    TCHAR szDir[PATH_MAX];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;

    _stprintf(szDir, "%s\\*", path);

    struct json_object* jobj = json_object_new_object();
    struct json_object* jarray = json_object_new_array();
    json_object_object_add(jobj, "files", jarray);
    hFind = FindFirstFile(szDir, &ffd);

    if(hFind == INVALID_HANDLE_VALUE) {
        const char* content = json_object_to_json_string_ext(
            jobj,
            JSON_C_TO_STRING_PRETTY
        );
        size_t reply_length = strlen(content);
        *len = reply_length;
        char* reply = malloc(reply_length+1);
        reply[reply_length] = '\0';
        memcpy(reply, content, reply_length);
        json_object_put(jobj);
        return reply;
    }

    static const char *video_ext[] = {
        "mp4", "mov", "wmv", "flv", "avi", "avchd", "webm", "mkv"
    };
    static const char *audio_ext[] = {
        "m4a", "flac", "mp3", "wav", "mwa", "aac"
    };

    do {
        if(strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
            continue;

        const char *type = NULL;
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            type = "directory";
        }
        else {
            char *ext = strrchr(ffd.cFileName, '.');
            while(ext != NULL) {
                ext += 1;
                for(int i=0; i<8; i++) {
                    if(strncmp(ext, video_ext[i], 5) == 0) {
                        type = "video";
                        break;
                    }
                }
                if(type != NULL)
                    break;
                for(int i=0; i<6; i++) {
                    if(strncmp(ext, audio_ext[i], 5) == 0) {
                        type = "audio";
                        break;
                    }
                }
                break;
            }
        }

        if(type != NULL) {
            struct json_object *jfile = json_object_new_object();
            json_object_object_add(jfile, "name",
                                   json_object_new_string(ffd.cFileName));
            json_object_object_add(jfile, "type",
                                   json_object_new_string(type));
            json_object_array_add(jarray, jfile);
        }
    } while(FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);
    const char *content = json_object_to_json_string_ext(
        jobj,
        JSON_C_TO_STRING_PRETTY
    );
    size_t reply_length = strlen(content);
    *len = reply_length;
    char* reply = malloc(reply_length+1);
    reply[reply_length] = '\0';
    memcpy(reply, content, reply_length);
    json_object_put(jobj);
    return reply;
    #else
    return NULL;
    #endif
}




static char *list_drives(size_t *len) {
    #ifdef _WIN32
    struct json_object* jobj = json_object_new_object();
    struct json_object* jarray = json_object_new_array();
    json_object_object_add(jobj, "files", jarray);

    for(TCHAR drive='D'; drive<='H'; drive++) {
        WIN32_FIND_DATA ffd;
        TCHAR szDir[PATH_MAX];
        HANDLE hFind = INVALID_HANDLE_VALUE;
        DWORD dwError = 0;
        _stprintf(szDir, "%c:\\*", drive);
        hFind = FindFirstFile(szDir, &ffd);
        if(hFind != INVALID_HANDLE_VALUE) {
            struct json_object* jfile = json_object_new_object();
            char label[3] = {drive, ':', '\0'};
            json_object_object_add(jfile, "name",
                                   json_object_new_string(label));
            json_object_object_add(jfile, "type",
                                   json_object_new_string("directory"));
            json_object_array_add(jarray, jfile);
        }
        FindClose(hFind);
    }

    const char *content = json_object_to_json_string_ext(
        jobj,
        JSON_C_TO_STRING_PRETTY
    );
    size_t reply_length = strlen(content);
    *len = reply_length;
    char *reply = malloc(reply_length+1);
    reply[reply_length] = '\0';
    memcpy(reply, content, reply_length);
    json_object_put(jobj);
    return reply;
    #else
    return NULL;
    #endif
}




static char *get_thumbnail(const char *file, size_t *len) {
    char cmd[512];
    snprintf(cmd, 512, "ffprobe -i \"%s\" -show_entries format=duration -v "
                       "quiet -of csv=\"p = 0\"", file);

    char *read = {0};
    read = calloc(16, 1);
    cmd_rsp(cmd, &read, 16);
    if(read[0] == '\0') {
        free(read);
        return NULL;
    }
    double duration = atof(read);
    free(read);

    #ifdef _WIN32
    char thumbnail[PATH_MAX];
    snprintf(thumbnail, PATH_MAX, "%s\\mpv-thumbnail.png", getenv("TEMP"));
    #else
    const char *thumbnail = "/tmp/mpv-thumbnail.png";
    #endif

    snprintf(
        cmd,
        512,
        "ffmpeg -ss %lf -i \"%s\" -vf select=\"eq(pict_type\\, I), scale = "
        "320:180\" -vframes 1 \"%s\"",
        duration / 2,
        file,
        thumbnail
    );

    remove(thumbnail);
    read = calloc(8192, 1);
    cmd_rsp(cmd, &read, 8192);
    free(read);
    char *content = load_file(thumbnail, "rb", len);
    remove(thumbnail);
    return content;
}