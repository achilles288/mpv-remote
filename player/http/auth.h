/**
 * @file auth.h
 * @brief Authenticates the users for HTTP services
 * 
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 * 
 * @license{This project is released under the GPL License.}
 */


#ifndef __MPV_REMOTE_HTTP_AUTH_H__
#define __MPV_REMOTE_HTTP_AUTH_H__ ///< Header guard

#include "con_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Authenticates the access to special services
 * 
 * @param con_info The user
 * @param pswd The password string
 * 
 * @return 1 if authenticated and 0 otherwise
 */
int remote_http_authenticate(struct RemoteConnection* con_info,
	                         const char* pswd);

/**
 * @brief To check if the user if authenticated
 * 
 * @param con_info The user
 * 
 * @return 1 if authenticated and 0 otherwise
 */
int remote_http_is_authenticated(struct RemoteConnection* con_info);

/**
 * @brief Changes the password
 *
 * @param old_pswd The old password
 * @param new_pswd The new password
 *
 * @return 1 if the old password is correct and 0 otherwise
 */
int remote_http_change_password(const char *old_pswd, const char *new_pswd);

#ifdef __cplusplus
}
#endif

#endif
