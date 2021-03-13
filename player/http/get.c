/**
 * @file get.c
 * @brief Handles the GET requests for mpv-play
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#include "auth.h"
#include "../../libremote/libremote.h"

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#endif

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
        len = snprintf(con_info->reply, 512, error_html, "404 Not Found",
                       "This URL isn't available.");
    }
    else if(error_code == MHD_HTTP_UNSUPPORTED_MEDIA_TYPE) {
        len = snprintf(con_info->reply, 512, error_html, "Unsupported Media",
                       "This media type isn't supported.");
    }
    else if(error_code == MHD_HTTP_INTERNAL_SERVER_ERROR) {
        len = snprintf(con_info->reply, 512, error_html, "Error 500",
                       "Sorry, an internal server error occured.");
    }
    else {
        len = snprintf(con_info->reply, 512, error_html, "Error",
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
    char *ext = strrchr(url, '.') + 1;
    if(ext == NULL) {
        error_answer(con_info, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE);
        return;
    }
    
    // Checks the extensions to determine the mimetype
    struct MimeType mimetypes[] = {
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
        if(strcmp(ext, mimetypes[i].ext) == 0) {
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
    else if(strcmp(url, "/is-authenticated") == 0) {
        strcpy(con_info->content_type, "");
        int auth = remote_http_isAuthenticated(con_info);
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
        
        if(remote_http_isAuthenticated(con_info)) {
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