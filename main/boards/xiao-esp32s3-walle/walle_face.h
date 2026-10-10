#ifndef WALLE_FACE_H
#define WALLE_FACE_H

#include <lvgl.h>

#include <cstdint>
#include <functional>

// Animated robot face (eyes and mouth) drawn with LVGL primitives: rounded
// rectangles, triangles and arcs, so it needs no image assets in flash. The
// eyes blink, look around while idle and pulse while speaking; the mouth moves
// while speaking. Both change shape and colour with the emotion the server sends.
//
// Every method must be called with the display lock held.
class WalleFace {
public:
    enum class Mode { kIdle, kListening, kSpeaking };

    WalleFace(lv_obj_t* parent, int width, int height);
    ~WalleFace();

    // Returns false when the emotion has no eye expression, so the caller can
    // show the matching status icon instead.
    bool SetEmotion(const char* emotion);
    void SetMode(Mode mode);
    void SetVisible(bool visible);
    lv_obj_t* obj() const { return obj_; }
    // Loudness of the audio being played, 0..1. When set, the mouth follows it
    // instead of animating from the speaking state. Called from the LVGL task.
    void SetVoiceLevelSource(std::function<float()> source) { voice_level_ = std::move(source); }

private:
    struct Pose {
        float open;       // eye height, 1 = normal
        float lid_inner;  // top lid cover at the nose side, 0..1 of eye height
        float lid_outer;  // top lid cover at the outer side
        float smile;      // bottom cut for happy "^ ^" eyes, 0..1
        float look_x;     // gaze offset in pixels
        float look_y;
        float scale;      // overall eye size
        float left_open;  // per-eye multiplier, for winks
        float right_open;
        float mouth_curve;  // -1 frown .. 1 smile, used while the mouth is closed
        float mouth_open;   // 0 closed .. 1 wide open ("O")
        float mouth_width;  // 1 = normal
    };

    struct EyeShape {
        lv_area_t body;
        float lid_inner;
        float lid_outer;
        float smile;
    };

    struct MouthShape {
        int32_t center_x;
        int32_t center_y;
        int32_t width;
        int32_t open_height;
        float curve;
    };

    struct Expression {
        const char* name;
        Pose pose;
        uint32_t color;
        bool tear;
    };

    static const Expression kExpressions[];

    static void TimerCallback(lv_timer_t* timer);
    static void DrawCallback(lv_event_t* e);

    void Apply(const Expression& expression);
    void Tick();
    void UpdateShapes(float blink, float bob, float pulse_open, float pulse_scale);
    lv_area_t DirtyArea() const;
    void Draw(lv_layer_t* layer);
    void DrawEye(lv_layer_t* layer, const EyeShape& eye, bool is_left);
    void DrawTear(lv_layer_t* layer, const EyeShape& eye, bool is_left);
    void DrawMouth(lv_layer_t* layer);

    lv_obj_t* obj_ = nullptr;
    lv_timer_t* timer_ = nullptr;
    int width_;
    int height_;

    Pose current_;
    Pose target_;
    Mode mode_ = Mode::kIdle;
    const Expression* active_ = &kExpressions[0];
    bool neutral_pending_ = false;
    uint32_t neutral_at_ms_ = 0;
    bool tear_ = false;
    float tear_phase_ = 0.0f;
    bool visible_ = true;

    lv_color_t color_from_;
    lv_color_t color_to_;
    lv_color_t color_;
    float color_progress_ = 1.0f;

    uint32_t next_blink_ms_ = 0;
    uint32_t blink_start_ms_ = 0;
    bool blinking_ = false;
    uint32_t next_glance_ms_ = 0;
    float glance_x_ = 0.0f;
    float glance_y_ = 0.0f;

    std::function<float()> voice_level_;
    uint32_t next_syllable_ms_ = 0;
    float syllable_ = 0.0f;
    float talk_open_ = 0.0f;

    EyeShape eyes_[2] = {};
    MouthShape mouth_ = {};
    lv_area_t drawn_area_ = {};
    bool has_drawn_area_ = false;
};

#endif  // WALLE_FACE_H
