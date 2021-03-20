/**
 * @file http.h
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


#ifndef __MPV_REMOTE_HTTP_H__
#define __MPV_REMOTE_HTTP_H__ ///< Header guard

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Starts running a web server on a parallel thread
 * 
 * @return Error code
 */
int remote_http_start_daemon();

/**
 * @brief Terminates the thread running the web server
 */
void remote_http_stop_daemon();

#ifdef __cplusplus
}
#endif

#endif
