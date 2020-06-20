/**
 * @file status.h
 * @brief Tracks and updates the remote media player status
 * 
 * Functions track and update the status attributes. To sync multiple
 * key-value pairs at once, JSON file system is used.
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#ifndef __MPV_REMOTE_STATUS_H__
#define __MPV_REMOTE_STATUS_H__ ///< Header guard

#ifndef REMOTE_EXPORT
#ifdef _WIN32
#define REMOTE_EXPORT __declspec(dllimport)
#else
#define REMOTE_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define REMOTE_MEDIA_LOCAL 0 ///< Local file on the host device
#define REMOTE_MEDIA_HTTP  1 ///< File on web

#ifndef REMOTE_MESSAGE_MAX
#define REMOTE_MESSAGE_MAX 1024 ///< Maximum size of a log message
#endif


/**
 * @brief Syncs the status attributes with the JSON file
 * 
 * Pulls the data saved in the JSON file into the respective members.
 * The process is done by the use of a JSON parser. After calling this
 * function, the data can be accessed by calling the get functions like
 * remote_status_get_url().
 */
REMOTE_EXPORT void remote_status_pull();

/**
 * @brief Gets the name of the media being played
 * 
 * @return Name string
 */
REMOTE_EXPORT const char* remote_status_get_name();

/**
 * @brief Gets the media URL being played
 * 
 * @return URL string
 */
REMOTE_EXPORT const char* remote_status_get_url();

/**
 * @brief Gets the type of media being played
 * 
 * @return REMOTE_MEDIA_LOCAL or REMOTE_MEDIA_HTTP
 */
REMOTE_EXPORT int remote_status_get_media_type();

/**
 * @brief Gets the playback time of the media
 * 
 * @return Time in seconds
 */
REMOTE_EXPORT double remote_status_get_time();

/**
 * @brief Gets the playback duration of the media
 * 
 * @return Time in seconds
 */
REMOTE_EXPORT double remote_status_get_duration();

/**
 * @brief Checks whether the media is paused
 * 
 * @return 1 if the media is paused and 0 if it is playing
 */
REMOTE_EXPORT int remote_status_get_paused();

/**
 * @brief Checks whether the media is loaded
 * 
 * @return 1 if the media is loaded and 0 if it is playing
 */
REMOTE_EXPORT int remote_status_get_loaded();

/**
 * @brief Checks whether the display program is running
 * 
 * @return 1 if the display program is active
 */
REMOTE_EXPORT int remote_status_get_running();

/**
 * @brief Gets the current error code and message
 * 
 * @param msg Pointer to string to which the error message is copied
 * 
 * @return Error code
 */
REMOTE_EXPORT int remote_status_get_error(char* msg);

/**
 * @brief Prints all the attributes of the remote media player
 */
REMOTE_EXPORT void remote_status_print();




/**
 * @brief Updates the status attributes into the JSON file
 * 
 * Pushes the updated attributes creating a new JSON file.
 */
REMOTE_EXPORT void remote_status_push();

/**
 * @brief Reset the status attributes to the default
 */
REMOTE_EXPORT void remote_status_set_default();

/**
 * @brief Updates the media name tag
 * 
 * @param s Media name tag
 */
REMOTE_EXPORT void remote_status_set_name(const char *s);

/**
 * @brief Updates the media URL
 * 
 * The same function also updates the media type by analyzing the URL
 * structure
 * 
 * @param s Media URL
 */
REMOTE_EXPORT void remote_status_set_url(const char *s);

/**
 * @brief Updates the playback time of the media
 * 
 * @param t Time in seconds
 */
REMOTE_EXPORT void remote_status_set_time(double t);

/**
 * @brief Updates the playback duration of the media
 * 
 * @param t Time in seconds
 */
REMOTE_EXPORT void remote_status_set_duration(double t);

/**
 * @brief Updates the pause/play status
 * 
 * @param b 1 for pause and 0 for play
 */
REMOTE_EXPORT void remote_status_set_paused(int b);

/**
 * @brief Updates the loaded status
 * 
 * @param b 1 if the media is loaded
 */
REMOTE_EXPORT void remote_status_set_loaded(int b);

/**
 * @brief Updates the running status
 * 
 * This attribute is updated to notify the remote whether the display
 * program is active or not.
 * 
 * @param b 1 if the display program is running
 */
REMOTE_EXPORT void remote_status_set_running(int b);

/**
 * @brief Updates the error code and message
 * 
 * @param code Error code
 * @param msg Error message
 */
REMOTE_EXPORT void remote_status_set_error(int code, const char *msg);

#ifdef __cplusplus
}
#endif

#endif
