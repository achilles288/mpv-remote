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


#ifdef _WIN32
#define REMOTE_EXPORT __declspec(dllexport)
#else
#define REMOTE_EXPORT
#endif

#include "status.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#else
#include <linux/limits.h>
#endif
#define MESSAGE_MAX REMOTE_MESSAGE_MAX
#define JSON_FILE_MAX 8192

#include <json.h>


static char name[PATH_MAX];
static char url[PATH_MAX];
static int mediaType = REMOTE_MEDIA_LOCAL;
static double time = 0.0;
static double duration = 0.0;
static int paused = 0;
static int loaded = 1;
static int running = 0;
static int errorCode = 0;
static char errorMessage[MESSAGE_MAX];


/**
 * @brief Syncs the status attributes with the JSON file
 * 
 * Pulls the data saved in the JSON file into the respective members.
 * The process is done by the use of a JSON parser. After calling this
 * function, the data can be accessed by calling the get functions like
 * remote_status_get_url().
 */
REMOTE_EXPORT void remote_status_pull() {
    #ifdef _WIN32
    char jsonFile[PATH_MAX];
    snprintf(jsonFile, PATH_MAX, "%s\\mpv-status.json", getenv("TEMP"));
    #else
    const char *jsonFile = "/tmp/mpv-status.json";
    #endif
    FILE *fp = fopen(jsonFile, "r");
    char content[JSON_FILE_MAX];
    if(fp == NULL) {
        remote_status_set_default();
        return;
    }
    int i;
    i = 0;
    char ch = getc(fp);
    while(ch != EOF) {
        content[i] = ch;
        ch = getc(fp);
        i++;
    }
    content[i] = '\0';
    fclose(fp);
    
    struct json_object *jobj, *jdata, *jerr;
    json_bool res;
    jobj = json_tokener_parse(content);
    
    // Gets name
    res = json_object_object_get_ex(jobj, "name", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    const char* jname_str = json_object_get_string(jdata);
    strcpy(name, jname_str);
    
    // Gets URL
    res = json_object_object_get_ex(jobj, "url", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    const char* jurl_str = json_object_get_string(jdata);
    char url[PATH_MAX];
    strcpy(url, jurl_str);
    remote_status_set_url(url);
    
    // Gets time
    res = json_object_object_get_ex(jobj, "time", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    time = json_object_get_double(jdata);
    res = json_object_object_get_ex(jobj, "duration", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    duration = json_object_get_double(jdata);
    
    // Gets pause/play status
    res = json_object_object_get_ex(jobj, "paused", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    paused = json_object_get_boolean(jdata);
    
    // Gets loaded status
    res = json_object_object_get_ex(jobj, "loaded", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    loaded = json_object_get_boolean(jdata);
    
    // Gets running status
    res = json_object_object_get_ex(jobj, "running", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    running = json_object_get_boolean(jdata);
    
    // Gets error code and message
    res = json_object_object_get_ex(jobj, "error", &jerr);
    if(!res) {
        remove(jsonFile);
        return;
    }
    res = json_object_object_get_ex(jerr, "code", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    errorCode = json_object_get_int(jdata);
    res = json_object_object_get_ex(jerr, "message", &jdata);
    if(!res) {
        remove(jsonFile);
        return;
    }
    const char *msg = json_object_get_string(jdata);
    strcpy(errorMessage, msg);
    json_object_put(jobj);
}

/**
 * @brief Gets the name of the media being played
 * 
 * @return Name string
 */
REMOTE_EXPORT const char* remote_status_get_name() { return name; }

/**
 * @brief Gets the media URL being played
 * 
 * @return URL string
 */
REMOTE_EXPORT const char* remote_status_get_url() { return url; }

/**
 * @brief Gets the type of media being played
 * 
 * @return REMOTE_MEDIA_LOCAL or REMOTE_MEDIA_HTTP
 */
REMOTE_EXPORT int remote_status_get_media_type() { return mediaType; }

/**
 * @brief Gets the playback time of the media
 * 
 * @return Time in seconds
 */
REMOTE_EXPORT double remote_status_get_time() { return time; }

/**
 * @brief Gets the playback duration of the media
 * 
 * @return Time in seconds
 */
REMOTE_EXPORT double remote_status_get_duration() { return duration; }

/**
 * @brief Checks whether the media is paused
 * 
 * @return 1 if the media is paused and 0 if it is playing
 */
REMOTE_EXPORT int remote_status_get_paused() { return paused; }

/**
 * @brief Checks whether the media is loaded
 * 
 * @return 1 if the media is loaded and 0 if it is playing
 */
REMOTE_EXPORT int remote_status_get_loaded() { return loaded; }

/**
 * @brief Checks whether the display program is running
 * 
 * @return 1 if the display program is active
 */
REMOTE_EXPORT int remote_status_get_running() { return running; }

/**
 * @brief Gets the current error code and message
 * 
 * @param msg Pointer to string to which the error message is copied
 * 
 * @return Error code
 */
REMOTE_EXPORT int remote_status_get_error(char* msg) {
    strcpy(msg, errorMessage);
    return errorCode;
}

/**
 * @brief Prints all the attributes of the remote media player
 */
REMOTE_EXPORT void remote_status_print() {
    printf(
        "MPV Remote Player status:\n"
        "    name: %s\n"
        "    url: %s\n"
        "    time: %d:%d:%d\n"
        "    duration: %d:%d:%d\n"
        "    paused: %d\n"
        "    loaded: %d\n"
        "    running: %d\n",
        name,
        url,
        (int)time / 3600,
        ((int)time / 60) % 60,
        (int)time % 60,
        (int)duration / 3600,
        ((int)duration / 60) % 60,
        (int)duration % 60,
        paused,
        loaded,
        running
    );
    if(errorCode != 0) {
        char buff[MESSAGE_MAX];
        strcpy(buff, errorMessage);
        size_t len = strlen(buff);
        if(buff[len-1] == '\n')
            buff[len-1] = '\0';
        printf(
            "    error:\n"
            "        code: %d\n"
            "        message: %s\n",
            errorCode,
            buff
        );
    }
}




/**
 * @brief Updates the status attributes into the JSON file
 * 
 * Pushes the updated attributes creating a new JSON file.
 */
REMOTE_EXPORT void remote_status_push() {
    // Creates JSON object
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "name", json_object_new_string(name));
    json_object_object_add(jobj, "url", json_object_new_string(url));
    json_object_object_add(jobj, "time", json_object_new_double(time));
    json_object_object_add(jobj, "duration", json_object_new_double(duration));
    json_object_object_add(jobj, "paused", json_object_new_boolean(paused));
    json_object_object_add(jobj, "loaded", json_object_new_boolean(loaded));
    json_object_object_add(jobj, "running", json_object_new_boolean(running));
    
    struct json_object *jerr = json_object_new_object();
    json_object_object_add(jerr, "code", json_object_new_int(errorCode));
    json_object_object_add(jerr, "message",
                           json_object_new_string(errorMessage));
    json_object_object_add(jobj, "error", jerr);
    
    // Writes JSON file
    const char *content = json_object_to_json_string_ext(
        jobj,
        JSON_C_TO_STRING_PRETTY
    );
    #ifdef _WIN32
    char jsonFile[PATH_MAX];
    snprintf(jsonFile, PATH_MAX, "%s\\mpv-status.json", getenv("TEMP"));
    #else
    const char *jsonFile = "/tmp/mpv-status.json";
    #endif
    FILE *fp = fopen(jsonFile, "w");
    fprintf(fp, "%s", content);
    fclose(fp);
    
    json_object_put(jobj);
}

/**
 * @brief Reset the status attributes to the default
 */
REMOTE_EXPORT void remote_status_set_default() {
    name[0] = '\0';
    url[0] = '\0';
    time = 0.0;
    duration = 0.0;
    paused = 0;
    loaded = 0;
    running = 0;
    errorCode = 0;
    errorMessage[0] = '\0';
}

/**
 * @brief Updates the media name tag
 * 
 * @param s Media name tag
 */
REMOTE_EXPORT void remote_status_set_name(const char *s) { strcpy(name, s); }

/**
 * @brief Updates the media URL
 * 
 * The same function also updates the media type by analyzing the URL
 * structure
 * 
 * @param s Media URL
 */
REMOTE_EXPORT void remote_status_set_url(const char *s) {
    // Copies the string
    strcpy(url, s);
    
    // Checks the media type
    mediaType = REMOTE_MEDIA_LOCAL;
    if(strlen(url) > 20) {
        char buff[9];
        memcpy(buff, url, 8);
        buff[8] = '\0';
        if(strcmp(buff, "https://") == 0)
            mediaType = REMOTE_MEDIA_HTTP;
    }
}

/**
 * @brief Updates the playback time of the media
 * 
 * @param t Time in seconds
 */
REMOTE_EXPORT void remote_status_set_time(double t) { time = t;}

/**
 * @brief Updates the playback duration of the media
 * 
 * @param t Time in seconds
 */
REMOTE_EXPORT void remote_status_set_duration(double t) { duration = t;}

/**
 * @brief Updates the pause/play status
 * 
 * @param b 1 for pause and 0 for play
 */
REMOTE_EXPORT void remote_status_set_paused(int b) { paused = b; }

/**
 * @brief Updates the loaded status
 * 
 * @param b 1 if the media is loaded
 */
REMOTE_EXPORT void remote_status_set_loaded(int b) { loaded = b; }

/**
 * @brief Updates the running status
 * 
 * This attribute is updated to notify the remote whether the display
 * program is active or not.
 * 
 * @param b 1 if the display program is running
 */
REMOTE_EXPORT void remote_status_set_running(int b) { running = b; }

/**
 * @brief Updates the error code and message
 * 
 * @param code Error code
 * @param msg Error message
 */
REMOTE_EXPORT void remote_status_set_error(int code, const char *msg) {
    errorCode = code;
    strcpy(errorMessage, msg);
}
