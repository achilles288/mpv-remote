#include "../libremote/status.h"

#include "test-config.h"

#include <cstdio>
#include <string>

#include <gtest/gtest.h>

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#endif


static char fileContent[8192];

static const char *readFile(const char *file) {
    FILE *fp = fopen(file, "r");
    if(fp == nullptr)
        return nullptr;
    int i = 0;
    char ch = getc(fp);
    while(ch != EOF) {
        fileContent[i] = ch;
        ch = getc(fp);
        i++;
    }
    fileContent[i] = '\0';
    fclose(fp);
    return fileContent;
}

static void copyFile(const char *dest, const char *src) {
    FILE *fsrc = fopen(src, "r");
    FILE *fdes = fopen(dest, "w");
    if(fsrc == nullptr || fdes == nullptr)
        return;
    char ch = getc(fsrc);
    while(ch != EOF) {
        fputc(ch, fdes);
        ch = getc(fsrc);
    }
    fclose(fsrc);
    fclose(fdes);
}




TEST(statusAssertion, assert) {
    remote_status_set_name("JoJo's Bizarre Adventures");
    EXPECT_STREQ("JoJo's Bizarre Adventures", remote_status_get_name());
    
    remote_status_set_url("~/Videos/jojo.mp4");
    EXPECT_STREQ("~/Videos/jojo.mp4", remote_status_get_url());
    
    remote_status_set_time(3202.12);
    EXPECT_EQ(3202.12, remote_status_get_time());
    
    remote_status_set_duration(7820.03);
    EXPECT_EQ(7820.03, remote_status_get_duration());
    
    remote_status_set_paused(false);
    EXPECT_FALSE(remote_status_get_paused());
    remote_status_set_paused(true);
    EXPECT_TRUE(remote_status_get_paused());
    
    remote_status_set_loaded(false);
    EXPECT_FALSE(remote_status_get_loaded());
    remote_status_set_loaded(true);
    EXPECT_TRUE(remote_status_get_loaded());
    
    remote_status_set_running(false);
    EXPECT_FALSE(remote_status_get_running());
    remote_status_set_running(true);
    EXPECT_TRUE(remote_status_get_running());
    
    remote_status_set_error(1, "Some error message");
    char msg[64];
    int code = remote_status_get_error(msg);
    EXPECT_EQ(1, code);
    EXPECT_STREQ("Some error message", msg);
}

TEST(statusAssertion, setDefault) {
    remote_status_set_default();
    EXPECT_STREQ("", remote_status_get_name());
    EXPECT_STREQ("", remote_status_get_url());
    EXPECT_EQ(0.0, remote_status_get_time());
    EXPECT_EQ(0.0, remote_status_get_duration());
    EXPECT_FALSE(remote_status_get_paused());
    EXPECT_FALSE(remote_status_get_loaded());
    EXPECT_FALSE(remote_status_get_running());
    char msg[64];
    int code = remote_status_get_error(msg);
    EXPECT_EQ(0, code);
    EXPECT_STREQ("", msg);
}




TEST(statusPrint, print) {
    remote_status_set_default();
    remote_status_set_name("The Watchmen");
    remote_status_set_url("~/Downloads/The Watchmen.mp4");
    remote_status_set_time(4048.3);
    remote_status_set_duration(10040.4);
    remote_status_set_paused(false);
    remote_status_set_loaded(true);
    remote_status_set_running(true);
    remote_status_set_error(0, "Media successfully loaded");
    
    testing::internal::CaptureStdout();
    remote_status_print();
    std::string printed = testing::internal::GetCapturedStdout();
    
    std::string expected =
        "MPV Remote Player status:\n"
        "    name: The Watchmen\n"
        "    url: ~/Downloads/The Watchmen.mp4\n"
        "    time: 1:7:28\n"
        "    duration: 2:47:20\n"
        "    paused: 0\n"
        "    loaded: 1\n"
        "    running: 1\n";
    
    EXPECT_EQ(expected, printed);
    
    remote_status_set_default();
    remote_status_set_name("");
    remote_status_set_url("~/Videos/Batmetal.webm");
    remote_status_set_time(0);
    remote_status_set_duration(0);
    remote_status_set_paused(true);
    remote_status_set_loaded(false);
    remote_status_set_running(false);
    remote_status_set_error(1, "Failed to load media");
    
    testing::internal::CaptureStdout();
    remote_status_print();
    printed = testing::internal::GetCapturedStdout().c_str();
    
    expected =
        "MPV Remote Player status:\n"
        "    name: \n"
        "    url: ~/Videos/Batmetal.webm\n"
        "    time: 0:0:0\n"
        "    duration: 0:0:0\n"
        "    paused: 1\n"
        "    loaded: 0\n"
        "    running: 0\n"
        "    error:\n"
        "        code: 1\n"
        "        message: Failed to load media\n";
    
    EXPECT_EQ(expected, printed);
}




TEST(statusPull, valid) {
    #ifdef _WIN32
    char jsonFile[PATH_MAX];
    snprintf(jsonFile, PATH_MAX, "%s\\mpv-status.json", getenv("TEMP"));
    #else
    const char *jsonFile = "/tmp/mpv-status.json";
    #endif
    
    copyFile(jsonFile, TEST_RESOURCE_PATH "/valid_input.json");
    remote_status_set_default();
    
    ASSERT_EXIT(
        {
            remote_status_pull();
            exit(0);
        },
        ::testing::ExitedWithCode(0),
        ".*"
    );
    remote_status_pull();
    
    EXPECT_STREQ("JoJo's Bizarre Adventures", remote_status_get_name());
    EXPECT_STREQ("~/Videos/jojo.mp4", remote_status_get_url());
    EXPECT_NEAR(124.56, remote_status_get_time(), 0.0001);
    EXPECT_NEAR(623.06, remote_status_get_duration(), 0.0001);
    EXPECT_FALSE(remote_status_get_paused());
    EXPECT_TRUE(remote_status_get_loaded());
    EXPECT_TRUE(remote_status_get_running());
    char msg[64];
    int code = remote_status_get_error(msg);
    EXPECT_EQ(0, code);
    EXPECT_STREQ("Media successfully loaded", msg);
}

TEST(statusPull, missing) {
    #ifdef _WIN32
    char jsonFile[PATH_MAX];
    snprintf(jsonFile, PATH_MAX, "%s\\mpv-status.json", getenv("TEMP"));
    #else
    const char *jsonFile = "/tmp/mpv-status.json";
    #endif
    
    copyFile(jsonFile, TEST_RESOURCE_PATH "/missing_input_1.json");
    remote_status_set_default();
    
    ASSERT_EXIT(
        {
            remote_status_pull();
            exit(0);
        },
        ::testing::ExitedWithCode(0),
        ".*"
    );
    
    copyFile(jsonFile, TEST_RESOURCE_PATH "/missing_input_2.json");
    
    ASSERT_EXIT(
        {
            remote_status_pull();
            exit(0);
        },
        ::testing::ExitedWithCode(0),
        ".*"
    );
}

TEST(statusPull, invalid) {
    #ifdef _WIN32
    char jsonFile[PATH_MAX];
    snprintf(jsonFile, PATH_MAX, "%s\\mpv-status.json", getenv("TEMP"));
    #else
    const char *jsonFile = "/tmp/mpv-status.json";
    #endif
    
    copyFile(jsonFile, TEST_RESOURCE_PATH "/invalid_input.json");
    
    ASSERT_EXIT(
        {
            remote_status_pull();
            exit(0);
        },
        ::testing::ExitedWithCode(0),
        ".*"
    );
}




TEST(statusPush, push) {
    remote_status_set_default();
    remote_status_set_name("Twice - More & More");
    remote_status_set_url("~/Music/Twice.webm");
    remote_status_set_time(61.3);
    remote_status_set_duration(243.1);
    remote_status_set_paused(true);
    remote_status_set_loaded(true);
    remote_status_set_running(true);
    remote_status_set_error(0, "Media successfully loaded");
    remote_status_push();
    
    ASSERT_EXIT(
        {
            remote_status_pull();
            exit(0);
        },
        ::testing::ExitedWithCode(0),
        ".*"
    );
    remote_status_set_default();
    remote_status_pull();
    EXPECT_STREQ("Twice - More & More", remote_status_get_name());
    EXPECT_STREQ("~/Music/Twice.webm", remote_status_get_url());
    EXPECT_NEAR(61.3, remote_status_get_time(), 0.0001);
    EXPECT_NEAR(243.1, remote_status_get_duration(), 0.0001);
    EXPECT_TRUE(remote_status_get_paused());
    EXPECT_TRUE(remote_status_get_loaded());
    EXPECT_TRUE(remote_status_get_running());
    char msg[64];
    int code = remote_status_get_error(msg);
    EXPECT_EQ(0, code);
    EXPECT_STREQ("Media successfully loaded", msg);
    
    #ifdef _WIN32
    char jsonFile[PATH_MAX];
    snprintf(jsonFile, PATH_MAX, "%s\\mpv-status.json", getenv("TEMP"));
    #else
    const char *jsonFile = "/tmp/mpv-status.json";
    #endif
    remove(jsonFile);
}
