#include "../libremote/logger.h"

#include <cstdio>
#include <string>

#include <gtest/gtest.h>

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#endif


TEST(logClear, clear) {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    FILE *fp = fopen(logFile, "w");
    ASSERT_NE(nullptr, fp);
    fprintf(fp, "Some content");
    fclose(fp);
    
    fp = fopen(logFile, "r");
    ASSERT_NE(nullptr, fp);
    fclose(fp);
    
    remote_log_clear();
    
    fp = fopen(logFile, "r");
    EXPECT_EQ(nullptr, fp);
    if(fp != nullptr)
        fclose(fp);
}




TEST(logRead, single) {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    remote_log_clear();
    FILE *fp = fopen(logFile, "w");
    ASSERT_NE(nullptr, fp);
    const char *msg;
    
    fprintf(fp, "Log\nThis is log message\n");
    fclose(fp);
    msg = remote_log_read();
    ASSERT_STREQ("Log\nThis is log message\n", msg);
}

TEST(logRead, updated) {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    remote_log_clear();
    FILE *fp = fopen(logFile, "w");
    ASSERT_NE(nullptr, fp);
    const char *msg;
    
    fprintf(fp, "The quick brown fox jumps over a lazy dog\n");
    fflush(fp);
    msg = remote_log_read();
    EXPECT_STREQ("The quick brown fox jumps over a lazy dog\n", msg);
    
    fprintf(fp, "Eat apple\nEat orange\n");
    fflush(fp);
    msg = remote_log_read();
    EXPECT_STREQ("Eat apple\nEat orange\n", msg);
    
    msg = remote_log_read();
    EXPECT_EQ(nullptr, msg);
    
    fprintf(fp, "Some content\n");
    fflush(fp);
    msg = remote_log_read();
    EXPECT_STREQ("Some content\n", msg);
    fclose(fp);
}




TEST(logWrite, plain) {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    remote_log_clear();
    testing::internal::CaptureStdout();
    remote_log_write("This is log message! ");
    std::string printed = testing::internal::GetCapturedStdout();
    EXPECT_EQ("This is log message! ", printed);
    
    FILE *fp = fopen(logFile, "r");
    ASSERT_NE(nullptr, fp);
    char content[REMOTE_MESSAGE_MAX];
    fgets(content, REMOTE_MESSAGE_MAX, fp);
    fclose(fp);
    EXPECT_STREQ("This is log message! ", content);
    remote_log_clear();
}

TEST(logWrite, arguments) {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    remote_log_clear();
    testing::internal::CaptureStdout();
    remote_log_write("Playing `%s` in %.1f seconds\n", "The Watchmen", 12.2f);
    std::string printed = testing::internal::GetCapturedStdout();
    EXPECT_EQ("Playing `The Watchmen` in 12.2 seconds\n", printed);
    
    FILE *fp = fopen(logFile, "r");
    ASSERT_NE(nullptr, fp);
    char content[REMOTE_MESSAGE_MAX];
    fgets(content, REMOTE_MESSAGE_MAX, fp);
    fclose(fp);
    EXPECT_STREQ("Playing `The Watchmen` in 12.2 seconds\n", content);
    remote_log_clear();
}

TEST(logWrite, append) {
    #ifdef _WIN32
    char logFile[PATH_MAX];
    snprintf(logFile, PATH_MAX, "%s\\mpv-log", getenv("TEMP"));
    #else
    const char *logFile = "/tmp/mpv-log";
    #endif
    
    remote_log_clear();
    testing::internal::CaptureStdout();
    remote_log_write("Alpha 1 ");
    std::string printed = testing::internal::GetCapturedStdout();
    EXPECT_EQ("Alpha 1 ", printed);
    
    FILE *fp = fopen(logFile, "r");
    ASSERT_NE(nullptr, fp);
    char content[REMOTE_MESSAGE_MAX];
    fgets(content, REMOTE_MESSAGE_MAX, fp);
    fclose(fp);
    EXPECT_STREQ("Alpha 1 ", content);
    
    testing::internal::CaptureStdout();
    remote_log_write("Beta 2 ");
    printed = testing::internal::GetCapturedStdout();
    EXPECT_EQ("Beta 2 ", printed);
    
    fp = fopen(logFile, "r");
    ASSERT_NE(nullptr, fp);
    fgets(content, REMOTE_MESSAGE_MAX, fp);
    fclose(fp);
    EXPECT_STREQ("Alpha 1 Beta 2 ", content);
    remote_log_clear();
}
