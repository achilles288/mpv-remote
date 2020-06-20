#include "../player/player.h"

#include "../libremote/status.h"
#include "test-config.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include <gtest/gtest.h>


static int playerPresetOptionsTest() {
    mpv_handle *ctx = mpv_create();
    if(!ctx) {
        printf("Failed creating context\n");
        return 1;
    }
    int res;
    
    /**
     * Enables the libmpv context options chosen for the system.
     * The API call to test.
     */
    res = remote_player_enable_preset_options(ctx);
    
    if(res != 0) {
        mpv_terminate_destroy(ctx);
        printf("MPV API error: %s\n", mpv_error_string(res));
        return 1;
    }
    res = mpv_initialize(ctx);
    if(res != 0) {
        mpv_terminate_destroy(ctx);
        printf("MPV API error: %s\n", mpv_error_string(res));
        return 1;
    }
    const char *url = TEST_RESOURCE_PATH "/video.mp4";
    const char *play_cmd[] = {"loadfile", url, NULL};
    res = mpv_command(ctx, play_cmd);
    if(res != 0) {
        mpv_terminate_destroy(ctx);
        printf("MPV API error: %s\n", mpv_error_string(res));
        return 1;
    }
    
    while(1) {
        mpv_event *event = mpv_wait_event(ctx, 0.2);
        if(event->event_id == MPV_EVENT_NONE)
            break;
    }
    mpv_terminate_destroy(ctx);
    return 0;
}

TEST(playerPresetOptions, set) {
    ASSERT_EXIT(
        {
            int status = playerPresetOptionsTest();
            exit(status);
        },
        ::testing::ExitedWithCode(0),
        ".*"
    );
}




class playerEvent: public ::testing::Test {
  protected:
    mpv_handle *ctx;
    
    virtual void SetUp() {
        ctx = mpv_create();
        if(!ctx) {
            printf("Failed creating context\n");
            return;
        }
        int res = mpv_initialize(ctx);
        if(res != 0) {
            mpv_terminate_destroy(ctx);
            printf("MPV API error: %s\n", mpv_error_string(res));
            ctx = nullptr;
            return;
        }
        const char *url = TEST_RESOURCE_PATH "/video.mp4";
        const char *play_cmd[] = {"loadfile", url, NULL};
        res = mpv_command(ctx, play_cmd);
        if(res != 0) {
            mpv_terminate_destroy(ctx);
            printf("MPV API error: %s\n", mpv_error_string(res));
            ctx = nullptr;
            return;
        }
        while(1) {
            mpv_event *event = mpv_wait_event(ctx, 0.2);
            if(event->event_id == MPV_EVENT_NONE)
                break;
        }
    }
    
    virtual void TearDown() {
        if(ctx == nullptr)
            return;
        while(1) {
            mpv_event *event = mpv_wait_event(ctx, 0.1);
            if(event->event_id == MPV_EVENT_NONE)
                break;
        }
        mpv_terminate_destroy(ctx);
    }
    
    void runEvent(mpv_handle *ctx, mpv_event_id id) {
        mpv_event event;
        event.event_id = id;
        remote_player_event_process(ctx, &event);
    }
    
    void delay(double t) {
        while(1) {
            mpv_event *event = mpv_wait_event(ctx, t);
            if(event->event_id == MPV_EVENT_NONE)
                break;
        }
    }
};

TEST_F(playerEvent, fileLoaded) {
    ASSERT_NE(nullptr, ctx);
    runEvent(ctx, MPV_EVENT_FILE_LOADED);
    EXPECT_STREQ("video.mp4", remote_status_get_name());
    EXPECT_NEAR(9.9614, remote_status_get_duration(), 0.0001);
    EXPECT_TRUE(remote_status_get_loaded());
}

TEST_F(playerEvent, pause) {
    ASSERT_NE(nullptr, ctx);
    runEvent(ctx, MPV_EVENT_PAUSE);
    EXPECT_TRUE(remote_status_get_paused());
}

TEST_F(playerEvent, unpause) {
    ASSERT_NE(nullptr, ctx);
    runEvent(ctx, MPV_EVENT_UNPAUSE);
    EXPECT_FALSE(remote_status_get_paused());
}

TEST_F(playerEvent, time) {
    ASSERT_NE(nullptr, ctx);
    runEvent(ctx, MPV_EVENT_FILE_LOADED);
    delay(0.6);
    runEvent(ctx, MPV_EVENT_NONE);
    EXPECT_NEAR(0.8, remote_status_get_time(), 0.1);
    delay(1.2);
    runEvent(ctx, MPV_EVENT_NONE);
    EXPECT_NEAR(2.0, remote_status_get_time(), 0.1);
}
