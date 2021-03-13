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
#include "con_type.h"


extern void remote_http_handle_get(
    struct MHD_Connection* connection,
    const char* url,
    const char* upload_data,
    size_t* upload_data_size,
    void** con_cls
);


extern int remote_http_handle_post(
    struct MHD_Connection* connection,
    const char* url,
    const char* upload_data,
    size_t* upload_data_size,
    void** con_cls
);


extern int remote_http_iterate_post(
    void* coninfo_cls,
    enum MHD_ValueKind kind,
    const char* key,
    const char* filename,
    const char* content_type,
    const char* transfer_encoding,
    const char* data,
    uint64_t off,
    size_t size
);


static int answer_to_connection(
    void* cls,
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
        con_info->fp = NULL;
        
        if(met == POST_METHOD) {
            con_info->postprocessor = MHD_create_post_processor(
                connection,
                POST_BUFFER_SIZE, 
                remote_http_iterate_post,
                (void*) con_info
            );
        }
        
        *con_cls = (void*) con_info; 
        return MHD_YES;
    }
    
    // Handle the request
    if(met == GET_METHOD) {
        remote_http_handle_get(connection, url, upload_data, upload_data_size,
                               con_cls);
    }
    else if(met == POST_METHOD) {
        int ret = remote_http_handle_post(connection, url, upload_data,
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
    
    if(con_info->reply != NULL)
        free(con_info->reply);

    if(con_info->fp != NULL)
        fclose(con_info->fp);
    
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
        MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
        &answer_to_connection, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed,
        NULL, MHD_OPTION_END
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
