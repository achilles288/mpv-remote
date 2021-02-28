/**
 * @file http.c
 * @brief Hosts the mpv-remote services via HTTP protocol
 * 
 * Deploys a web socket interactable by HTTP protocol in LAN. In other words,
 * it allows all libmpv-remote functions such as media loading, pausing and
 * seeking to be usable on a web browser running on an extra client device.
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#include "http.h"

#include "../libremote/libremote.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Winsock2.h>
#define PATH_MAX _MAX_PATH
#else
#include <netinet/in.h>
#include <gcrypt.h>
#endif

#include <microhttpd.h>

#define PORT 8888
#define PREFIX "http/public"

#define GET_METHOD 1
#define POST_METHOD 2
#define UNKNOWN_METHOD -1


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




struct RemoteConnection {
    uint32_t address;
    int method;
    char url[128];
    char content_type[24];
    int status;
    char *reply;
    size_t reply_length;
    struct MHD_PostProcessor *postprocessor;
};


struct MimeType {
    char ext[6];
    char alt[16];
    char mediatype[12];
    int binary;
};


static uint32_t admin_addr = 0;


#ifdef _WIN32
static int authenticate_wincrypt(const char* pswd) {
    const DWORD dlen = 20;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    CryptAcquireContext(&hProv, NULL, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);

    size_t file_length = 0;
    unsigned char* registered = load_file("http/password", "rb", &file_length);

    // If the hash does not exist or the hash is invalid
    if(registered == NULL || file_length != dlen) {
        if(registered != NULL)
            free(registered);
        // Rewrites a hash from a default password
        FILE* fp = fopen("http/password", "wb");
        DWORD len = dlen;
        registered = malloc(dlen);
        CryptHashData(hHash, "password", 8, 0);
        CryptGetHashParam(hHash, HP_HASHVAL, registered, &len, 0);
        for(DWORD i=0; i<dlen; i++)
            fputc(registered[i], fp);
        fclose(fp);
    }

    // Hashes the input password and compares with the registered one
    DWORD len = dlen;
    unsigned char* x = malloc(dlen);
    CryptHashData(hHash, pswd, strlen(pswd), 0);
    CryptGetHashParam(hHash, HP_HASHVAL, x, &len, 0);
    int auth = 1;
    for(DWORD i=0; i<dlen; i++) {
        if(x[i] != registered[i]) {
            auth = 0;
            break;
        }
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return auth;
}
#else
static int authenticate_gcrypt(const char* pswd) {
    unsigned int dlen = gcry_md_get_algo_dlen(GCRY_MD_SHA256);
    gcry_md_hd_t h;
    gcry_md_open(&h, GCRY_MD_SHA256, GCRY_MD_FLAG_SECURE);

    size_t len = 0;
    unsigned char* registered = load_file("http/password", "rb", &len);

    // If the hash does not exist or the hash is invalid
    if(registered == NULL || len != dlen) {
        if(registered != NULL)
            free(registered);
        // Rewrites a hash from a default password
        FILE* fp = fopen("http/password", "wb");
        gcry_md_write(h, "password", 8);
        unsigned char* x = gcry_md_read(h, GCRY_MD_SHA256);
        registered = malloc(dlen);
        memcpy(registered, x, dlen);
        for(size_t i=0; i<dlen; i++)
            fputc(registered[i], fp);
        fclose(fp);
        gcry_md_reset(h);
    }

    // Hashes the input password and compares with the registered one
    gcry_md_write(h, pswd, strlen(pswd));
    unsigned char* x = gcry_md_read(h, GCRY_MD_SHA256);
    int auth = 1;
    for(size_t i=0; i<dlen; i++) {
        if(x[i] != registered[i]) {
            auth = 0;
            break;
        }
    }

    gcry_md_close(h);
    return auth;
}
#endif

static int authenticate(const char *pswd) {
    #ifdef _WIN32
    return authenticate_wincrypt(pswd);
    #else
    return authenticate_gcrypt(pswd);
    #endif
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
    strncpy(con_info->content_type, "text/html", 24);
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




/* 
 * 
 * GET request handling
 * 
 */

static void answer_to_get_special(
    struct MHD_Connection *connection,
    const char *url,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);


static void answer_to_get(
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
    strncpy(con_info->content_type, "text/html", 24);
    
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
        strncpy(con_info->content_type, "", 24);
        int auth = (con_info->address == admin_addr);
        con_info->status = auth ? MHD_HTTP_OK : MHD_HTTP_UNAUTHORIZED;
    }
    else if(strcmp(url, "/status") == 0) {
        strncpy(con_info->content_type, "application/json", 24);
        #ifdef _WIN32
        char jsonFile[PATH_MAX];
        snprintf(jsonFile, PATH_MAX, "%s\\mpv-status.json", getenv("TEMP"));
        #else
        const char *jsonFile = "/tmp/mpv-status.json";
        #endif
        
        if(con_info->address == admin_addr) {
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
            con_info->status = MHD_HTTP_UNAUTHORIZED;
    }
}




/* 
 * 
 * POST request handling
 * 
 */
static int answer_to_post(
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
        error_answer(con_info, MHD_HTTP_NOT_FOUND);
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


static int iterate_post(
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
    // Limit data size for non-blob
    if(strcmp(key, "blob") != 0 && size > 128)
        return MHD_YES;
    
    struct RemoteConnection *con_info = coninfo_cls;
    
    // POST requests that do not require authentication
    if(strcmp(con_info->url, "/authenticate") == 0) {
        if(strcmp(key, "password") == 0) {
            int auth = authenticate(data);
            if(auth)
                admin_addr = con_info->address;
            con_info->status = auth ? MHD_HTTP_OK : MHD_HTTP_UNAUTHORIZED;
            return MHD_NO;
        }
        return MHD_YES;
    }
    
    int auth = (con_info->address == admin_addr);
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
            
        }
    }
    
    return MHD_YES;
}








static int answer_to_connection(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
)
{
    (void) cls;
    (void) version;
    
    int met = UNKNOWN_METHOD;
    if(strcmp(method, "GET") == 0)
        met = GET_METHOD;
    else if(strcmp(method, "POST") == 0)
        met = POST_METHOD;
    else
        return MHD_NO;
    
    // Initialize the connection
    if(*con_cls == NULL) {
        const union MHD_ConnectionInfo *info = MHD_get_connection_info(
            connection,
            MHD_CONNECTION_INFO_CLIENT_ADDRESS
        );
        struct sockaddr_in *sa = (struct sockaddr_in*) info->client_addr;
        
        struct RemoteConnection *con_info;
        con_info = malloc(sizeof(struct RemoteConnection));
        con_info->address = sa->sin_addr.s_addr;
        con_info->method = met;
        strncpy(con_info->url, url, 128);
        strncpy(con_info->content_type, "", 24);
        con_info->status = 0;
        con_info->reply = NULL;
        con_info->reply_length = 0;
        
        if(met == POST_METHOD) {
            con_info->postprocessor
                = MHD_create_post_processor(connection, 512, 
                                            iterate_post, (void*) con_info);
        }
        
        *con_cls = (void*) con_info; 
        return MHD_YES;
    }
    
    // Handle the request
    if(met == GET_METHOD)
        answer_to_get(connection, url, upload_data, upload_data_size, con_cls);
    else if(met == POST_METHOD) {
        int ret = answer_to_post(connection, url, upload_data,
                                 upload_data_size, con_cls);
        if(ret == MHD_YES)
            return MHD_YES;
    }
    
    struct RemoteConnection *con_info = *con_cls;
    
    // Creates a response from the reply
    if(con_info->reply || con_info->status != 0) {
        struct MHD_Response *response;
        response = MHD_create_response_from_buffer(con_info->reply_length,
                                                   con_info->reply,
                                                   MHD_RESPMEM_PERSISTENT);
        if(strlen(con_info->content_type)) {
            MHD_add_response_header(response, "Content-Type",
                                    con_info->content_type);
        }
        int ret = MHD_queue_response(connection, con_info->status, response);
        MHD_destroy_response(response);
        return ret;
    }
    else
        return MHD_NO;
}


void request_completed(void *cls, struct MHD_Connection *connection, 
     		       void **con_cls, enum MHD_RequestTerminationCode toe)
{
    struct RemoteConnection *con_info = *con_cls;
    
    if(con_info == NULL)
        return;
    
    if(con_info->method == POST_METHOD)
        MHD_destroy_post_processor(con_info->postprocessor);        
    
    if(con_info->reply)
        free(con_info->reply);
    
    free(con_info);
    *con_cls = NULL;   
}








static struct MHD_Daemon *http_daemon = NULL;

/**
 * @brief Starts running a web server on a parallel thread
 * 
 * @return Error code
 */
int remote_http_start_daemon() {
    remote_http_stop_daemon();
    
    http_daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        PORT,
        NULL, NULL,
        &answer_to_connection, NULL,
        MHD_OPTION_END
    );
    
    if(http_daemon == NULL)
        return 1;
    
    return 0;
}

/**
 * @brief Terminates the thread running the web server
 */
void remote_http_stop_daemon() {
    if(http_daemon != NULL)
        MHD_stop_daemon(http_daemon);
}
