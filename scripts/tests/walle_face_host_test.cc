// Host-side tests for the WALL-E face (main/boards/xiao-esp32s3-walle),
// rendered with a real LVGL into a RAM framebuffer. Built and run by
// test_walle_face.py.
#include <lvgl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "esp_random.h"

#define private public
#include "walle_face.h"
#undef private

namespace {

constexpr int kSize = 240;

uint32_t g_tick = 0;
uint32_t TickCallback() { return g_tick; }

std::vector<uint16_t> g_framebuffer(kSize* kSize);
int g_flushes = 0;

void FlushCallback(lv_display_t* display, const lv_area_t* area, uint8_t* pixels) {
    int width = lv_area_get_width(area);
    for (int y = area->y1; y <= area->y2; y++) {
        memcpy(&g_framebuffer[y * kSize + area->x1], pixels + (y - area->y1) * width * 2,
               width * 2);
    }
    g_flushes++;
    lv_display_flush_ready(display);
}

void Run(int ms) {
    for (int t = 0; t < ms; t += 10) {
        g_tick += 10;
        lv_timer_handler();
    }
}

int g_failures = 0;

void Check(bool ok, const char* test, const char* what) {
    if (!ok) {
        printf("FAIL %s: %s\n", test, what);
        g_failures++;
    }
}

void SetScreen(lv_color_t color) {
    lv_obj_set_style_bg_color(lv_screen_active(), color, 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
}

std::vector<uint16_t> FullRedraw() {
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(nullptr);
    return g_framebuffer;
}

// Partial redraws must leave no stale pixels behind.
void TestNoStalePixels() {
    SetScreen(lv_color_black());
    WalleFace face(lv_screen_active(), kSize, kSize);
    float level = 0.0f;
    face.SetVoiceLevelSource([&level] { return level; });
    const char* emotions[] = {"happy",  "crying", "angry",   "surprised", "winking",
                              "sleepy", "loving", "neutral", "confused",  "laughing"};
    const WalleFace::Mode modes[] = {WalleFace::Mode::kIdle, WalleFace::Mode::kListening,
                                     WalleFace::Mode::kSpeaking};
    int differing_steps = 0;
    for (int step = 0; step < 120; step++) {
        if (step % 6 == 0) {
            face.SetEmotion(emotions[(step / 6) % 10]);
        }
        face.SetMode(modes[step % 3]);
        level = static_cast<float>((step * 37) % 100) / 100.0f;
        Run(130);
        lv_refr_now(nullptr);  // flush areas invalidated by the last tick
        std::vector<uint16_t> incremental = g_framebuffer;
        if (incremental != FullRedraw()) {
            differing_steps++;
        }
    }
    Check(differing_steps == 0, __func__, "incremental frame differs from a full redraw");
}

int CountBlinks(uint32_t start_tick) {
    g_tick = start_tick;
    WalleFace face(lv_screen_active(), kSize, kSize);
    int blinks = 0;
    bool was_blinking = false;
    for (int t = 0; t < 30000; t += 10) {
        g_tick += 10;
        lv_timer_handler();
        if (face.blinking_ && !was_blinking) {
            blinks++;
        }
        was_blinking = face.blinking_;
    }
    return blinks;
}

// The LVGL tick wraps after 49.7 days; blinking must carry on across it.
void TestTickWrap() {
    for (uint32_t k = 0; k <= 4000; k += 50) {
        if (CountBlinks(0xFFFFFFFFu - k) < 3) {
            Check(false, __func__, "eyes stopped blinking across the tick wrap");
            return;
        }
    }
}

// A burst of neutral requests must not keep a mood on screen forever.
void TestNeutralHoldNotExtended() {
    WalleFace face(lv_screen_active(), kSize, kSize);
    face.SetEmotion("happy");
    Run(200);
    for (int i = 0; i < 5; i++) {
        face.SetEmotion("neutral");
        Run(1000);
    }
    Check(strcmp(face.active_->name, "neutral") == 0, __func__,
          "still showing the mood 5 s after the first neutral request");
}

void TestNeutralHoldKeepsMood() {
    WalleFace face(lv_screen_active(), kSize, kSize);
    face.SetEmotion("sad");
    Run(200);
    face.SetEmotion("neutral");
    Run(2000);
    Check(strcmp(face.active_->name, "sad") == 0, __func__, "mood dropped before the hold ended");
    Run(2500);
    Check(strcmp(face.active_->name, "neutral") == 0, __func__, "mood kept after the hold ended");
}

void TestUnknownAndNullEmotion() {
    WalleFace face(lv_screen_active(), kSize, kSize);
    Check(!face.SetEmotion(nullptr), __func__, "null emotion accepted");
    Check(!face.SetEmotion("wifi"), __func__, "icon name accepted as a mood");
    Check(!face.SetEmotion(""), __func__, "empty emotion accepted");
    Check(face.SetEmotion("happy"), __func__, "known mood rejected");
    Check(strcmp(face.active_->name, "happy") == 0, __func__, "known mood not applied");
}

// The face must look the same whatever the screen behind it shows.
void TestIndependentOfScreenColor() {
    const char* emotions[] = {"happy", "angry", "surprised", "crying"};
    for (const char* emotion : emotions) {
        std::vector<uint16_t> frames[2];
        const lv_color_t screens[2] = {lv_color_black(), lv_color_white()};
        for (int i = 0; i < 2; i++) {
            SetScreen(screens[i]);
            g_tick = 1000;
            g_random_state = 12345;
            WalleFace face(lv_screen_active(), kSize, kSize);
            face.SetEmotion(emotion);
            Run(1500);
            frames[i] = FullRedraw();
        }
        if (frames[0] != frames[1]) {
            Check(false, __func__, emotion);
        }
    }
    SetScreen(lv_color_black());
}

// Showing an already visible face must not force a full-screen redraw.
void TestSetVisibleIsIdempotent() {
    WalleFace face(lv_screen_active(), kSize, kSize);
    Run(200);
    lv_refr_now(nullptr);
    g_flushes = 0;
    face.SetVisible(true);
    lv_refr_now(nullptr);
    Check(g_flushes == 0, __func__, "SetVisible(true) redrew a visible face");

    face.SetVisible(false);
    face.SetVisible(true);
    lv_refr_now(nullptr);
    Check(g_flushes > 0, __func__, "showing a hidden face did not redraw it");
}

}  // namespace

int main() {
    lv_init();
    lv_tick_set_cb(TickCallback);
    lv_display_t* display = lv_display_create(kSize, kSize);
    static uint8_t buffer[kSize * kSize * 2];
    lv_display_set_buffers(display, buffer, nullptr, sizeof(buffer),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, FlushCallback);
    SetScreen(lv_color_black());

    TestNoStalePixels();
    TestTickWrap();
    TestNeutralHoldNotExtended();
    TestNeutralHoldKeepsMood();
    TestUnknownAndNullEmotion();
    TestIndependentOfScreenColor();
    TestSetVisibleIsIdempotent();

    if (g_failures == 0) {
        printf("OK\n");
    }
    return g_failures == 0 ? 0 : 1;
}
