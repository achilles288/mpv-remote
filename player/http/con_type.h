/**
 * @file con_type.h
 * @brief Data structures and constants to define the HTTP connection
 * 
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 * 
 * @license{This project is released under the GPL License.}
 */


#ifndef __MPV_REMOTE_HTTP_CONTYPE_H__
#define __MPV_REMOTE_HTTP_CONTYPE_H__ ///< Header guard

#include <stdint.h>
#include <stdio.h>

#include <microhttpd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PORT 8888 ///< The port number which the web page is hosted
#define PREFIX "http/public" ///< The directory where HTTP contents are located
#define POST_BUFFER_SIZE 8192 ///< The size of one packet for POST requests
#define PARAM_SIZE 256 ///< Maximum length of each param

#define GET_METHOD 1 ///< The GET method
#define POST_METHOD 2 ///< The POST method
#define UNKNOWN_METHOD -1 ///< Unknown method


/**
 * @brief HTTP connection
 */
struct RemoteConnection {
    uint32_t address; ///< IP address
    int method; ///< GET or POST
    char url[128]; ///< The address of a web page
    char content_type[24]; ///< The type of media the sever returns
    int status; ///< Connection status
    char *reply; ///< The body of the content
    size_t reply_length; ///< The number of bytes of the content
    FILE *fp; ///< File pointer which only involves in file uploading
    char param1[PARAM_SIZE]; // Param 1
    char param2[PARAM_SIZE]; // Param 2
    char param3[PARAM_SIZE]; // Param 3
    char param4[PARAM_SIZE]; // Param 4
    struct MHD_PostProcessor* postprocessor; ///< To handle the POST requests
};


#ifdef __cplusplus
}
#endif

#endif
