#include "../libremote/command.h"

#include <cstdio>

#include <gtest/gtest.h>

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#endif


TEST(commandTokenize, normal) {
    char *tokens[5];
    char cmd[] = "I am legend";
    int count = _remote_command_tokenize(cmd, tokens, 5);
    EXPECT_STREQ("I", tokens[0]);
    EXPECT_STREQ("am", tokens[1]);
    EXPECT_STREQ("legend", tokens[2]);
    EXPECT_EQ(nullptr, tokens[3]);
    EXPECT_EQ(3, count);
}

TEST(commandTokenize, quoted) {
    char *tokens[5];
    int count;
    
    char cmd1[] = "eat \"coconut\"";
    count = _remote_command_tokenize(cmd1, tokens, 5);
    EXPECT_STREQ("eat", tokens[0]);
    EXPECT_STREQ("coconut", tokens[1]);
    EXPECT_EQ(nullptr, tokens[2]);
    EXPECT_EQ(2, count);
    
    char cmd2[] = "mpv \"Linkin Park - Numb.mp4\" --fs --hwdec=auto";
    count = _remote_command_tokenize(cmd2, tokens, 5);
    EXPECT_STREQ("mpv", tokens[0]);
    EXPECT_STREQ("Linkin Park - Numb.mp4", tokens[1]);
    EXPECT_STREQ("--fs", tokens[2]);
    EXPECT_STREQ("--hwdec=auto", tokens[3]);
    EXPECT_EQ(nullptr, tokens[4]);
    EXPECT_EQ(4, count);
}




TEST(commandRead, empty) {
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    remove(cmdFile);
    
    ASSERT_EQ(REMOTE_COMMAND_NONE, remote_command_read());
}

TEST(commandRead, notListed) {
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    FILE *fp = fopen(cmdFile, "w");
    ASSERT_NE(nullptr, fp);
    fprintf(fp, "remote-test 1033");
    fclose(fp);
    
    ASSERT_EQ(REMOTE_COMMAND_NONE, remote_command_read());
}

TEST(commandRead, open) {
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    FILE *fp = fopen(cmdFile, "w");
    ASSERT_NE(nullptr, fp);
    fprintf(fp, "open \"The Matrix Revolutions (2003).mp4\"");
    fclose(fp);
    
    EXPECT_EQ(REMOTE_COMMAND_OPEN, remote_command_read());
    
    EXPECT_STREQ("The Matrix Revolutions (2003).mp4",
                 remote_command_get_url_request());
}

TEST(commandRead, pause) {
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    FILE *fp = fopen(cmdFile, "w");
    ASSERT_NE(nullptr, fp);
    fprintf(fp, "pause");
    fclose(fp);
    
    ASSERT_EQ(REMOTE_COMMAND_PAUSE, remote_command_read());
}

TEST(commandRead, move) {
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    FILE *fp = fopen(cmdFile, "w");
    ASSERT_NE(nullptr, fp);
    fprintf(fp, "move 30.6");
    fclose(fp);
    
    EXPECT_EQ(REMOTE_COMMAND_MOVE, remote_command_read());
    
    EXPECT_NEAR(30.6, remote_command_get_move_request(), 0.0001);
}

TEST(commandRead, stop) {
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    FILE *fp = fopen(cmdFile, "w");
    ASSERT_NE(nullptr, fp);
    fprintf(fp, "pause");
    fclose(fp);
    
    ASSERT_EQ(REMOTE_COMMAND_PAUSE, remote_command_read());
}




TEST(commandWrite, plain) {
    remote_command_write("open \"Watchmen (2009).mp4\"");
    
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    char content[REMOTE_MESSAGE_MAX];
    FILE *fp = fopen(cmdFile, "r");
    ASSERT_NE(nullptr, fp);
    fgets(content, REMOTE_MESSAGE_MAX, fp);
    fclose(fp);
    remove(cmdFile);
    ASSERT_STREQ("open \"Watchmen (2009).mp4\"", content);
}

TEST(commandWrite, arguments) {
    remote_command_write("cmd1 %s %d", "String", 301);
    
    #ifdef _WIN32
    char cmdFile[PATH_MAX];
    snprintf(cmdFile, PATH_MAX, "%s\\mpv-command", getenv("TEMP"));
    #else
    const char *cmdFile = "/tmp/mpv-command";
    #endif
    
    char content[REMOTE_MESSAGE_MAX];
    FILE *fp = fopen(cmdFile, "r");
    ASSERT_NE(nullptr, fp);
    fgets(content, REMOTE_MESSAGE_MAX, fp);
    fclose(fp);
    remove(cmdFile);
    ASSERT_STREQ("cmd1 String 301", content);
}
