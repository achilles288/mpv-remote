/**
 * @file http.c
 * @brief Hosts the mpv-remote services via HTTP protocol
 * 
 * Deploys a web socket interactable by HTTP protocol in LAN. In other words,
 * it allows all libmpv-remote functions such as media loading, pausing and
 * seeking to be usable on a web browser running on an extra client device.
 * 
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 * 
 * @license{This project is released under the GPL License.}
 */


#include "http.h"

#include "config.h"
#include "con_type.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <netdb.h>
#endif

#define POST_BUFFER_SIZE 8192 ///< The size of one packet for POST requests


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
        con_info->status = 0;
        con_info->reply_length = 0;
        con_info->reply = NULL;
        con_info->fp = NULL;
        strncpy(con_info->url, url, 128);
        strcpy(con_info->content_type, "");
        strcpy(con_info->param1, "");
        strcpy(con_info->param2, "");
        strcpy(con_info->param3, "");
        strcpy(con_info->param4, "");
        
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

static void get_ip_address(char *addr) {
    #ifdef _WIN32
    ULONG Err;
    PIP_ADAPTER_INFO pAdapt;
    DWORD AdapterInfoSize;
    PIP_ADDR_STRING pAddrStr;

    if((Err = GetAdaptersInfo(NULL, &AdapterInfoSize)) != 0) {
        if(Err != ERROR_BUFFER_OVERFLOW)
            return;
    }

    pAdapt = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, AdapterInfoSize);
    if(pAdapt == NULL) {
        return;
    }

    if((Err = GetAdaptersInfo(pAdapt, &AdapterInfoSize)) != 0) {
        return;
    }

    if(pAdapt) {
        pAddrStr = &(pAdapt->IpAddressList);
        if(pAddrStr) {
            strncpy(addr, pAddrStr->IpAddress.String, 15);
        }
        pAdapt = pAdapt->Next;
    }
    
    #else
    struct ifaddrs *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if(getifaddrs(&ifa) == -1)
        return;
    
    while(ifa != NULL) {
        if(ifa->ifa_addr == NULL)
            continue;
        
        s = getnameinfo(
            ifa->ifa_addr,
            sizeof(struct sockaddr_in),
            host,
            NI_MAXHOST,
            NULL,
            0,
            NI_NUMERICHOST
        );

        if(ifa->ifa_addr->sa_family==AF_INET) {
            if(s != 0) {
                return;
            }
            strncpy(addr, host, 15);
        }
        ifa = ifa->ifa_next;
    }
    freeifaddrs(ifa);
    #endif
}

/**
 * @brief Starts running a web server on a parallel thread
 * 
 * @return Error code
 */
int remote_http_start_daemon() {
    remote_http_stop_daemon();
    
    http_daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD, HTTP_PORT, NULL, NULL,
        &answer_to_connection, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed,
        NULL, MHD_OPTION_END
    );

    char ip_addr[16] = "0.0.0.0";
    get_ip_address(ip_addr);
    printf("HTTP services can be used at http://%s:%d\n", ip_addr, HTTP_PORT);
    
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
