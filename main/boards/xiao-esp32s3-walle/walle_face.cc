#include "walle_face.h"

#include <esp_random.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

constexpr uint32_t kFrameMs = 40;
constexpr uint32_t kBlinkMs = 180;
constexpr uint32_t kTearMs = 1400;
constexpr uint32_t kMoodHoldMs = 4000;
constexpr int32_t kGlow = 10;
constexpr int32_t kMouthStroke = 6;
// How far the mouth can reach above or below its centre, glow included.
constexpr int32_t kMouthReach = 28;
constexpr float kEase = 0.28f;
constexpr float kColorStep = 0.12f;
constexpr float kTwoPi = 6.2831853f;

constexpr uint32_t kCyan = 0x35E0FF;
constexpr uint32_t kPink = 0xFF5FA2;
constexpr uint32_t kRed = 0xFF4A3D;
constexpr uint32_t kBlue = 0x4F8CFF;
constexpr uint32_t kWhite = 0xE8FBFF;
constexpr uint32_t kDim = 0x1E7F94;
constexpr uint32_t kBlush = 0xFF8FB3;
constexpr uint32_t kTearColor = 0x8FD8FF;

uint32_t RandomRange(uint32_t min, uint32_t max) { return min + esp_random() % (max - min + 1); }

void Approach(float& value, float target) {
    value += (target - value) * kEase;
    if (std::fabs(target - value) < 0.002f) {
        value = target;
    }
}

}  // namespace

// Fields: open, lid_inner, lid_outer, smile, look_x, look_y, scale, left_open, right_open,
// mouth_curve, mouth_open, mouth_width.
const WalleFace::Expression WalleFace::kExpressions[] = {
    {"neutral", {1.0f, 0.0f, 0.0f, 0.0f, 0, 0, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.8f}, kCyan, false},
    {"robot_2", {1.0f, 0.0f, 0.0f, 0.0f, 0, 0, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.8f}, kCyan, false},
    {"happy", {1.0f, 0.0f, 0.0f, 0.42f, 0, -2, 1.02f, 1.0f, 1.0f, 0.85f, 0.0f, 1.0f}, kCyan, false},
    {"relaxed",
     {0.9f, 0.12f, 0.12f, 0.38f, 0, 0, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 0.8f},
     kCyan,
     false},
    {"confident",
     {0.9f, 0.18f, 0.05f, 0.3f, 0, -2, 1.02f, 1.0f, 1.0f, 0.6f, 0.0f, 0.9f},
     kCyan,
     false},
    {"delicious",
     {1.0f, 0.0f, 0.0f, 0.45f, 0, -4, 1.05f, 1.0f, 1.0f, 0.8f, 0.25f, 0.8f},
     kCyan,
     false},
    {"laughing",
     {1.0f, 0.0f, 0.0f, 0.55f, 0, -4, 1.06f, 1.0f, 1.0f, 0.9f, 0.55f, 1.1f},
     kCyan,
     false},
    {"funny", {1.0f, 0.0f, 0.0f, 0.5f, 0, -3, 1.04f, 1.0f, 1.0f, 0.8f, 0.35f, 1.0f}, kCyan, false},
    {"silly", {1.0f, 0.0f, 0.0f, 0.45f, 6, 0, 1.03f, 1.0f, 0.75f, 0.7f, 0.3f, 0.9f}, kCyan, false},
    {"loving", {1.0f, 0.0f, 0.0f, 0.42f, 0, -2, 1.08f, 1.0f, 1.0f, 0.7f, 0.0f, 0.8f}, kPink, false},
    {"kissy", {1.0f, 0.0f, 0.0f, 0.48f, 0, 0, 1.0f, 1.0f, 0.12f, 0.0f, 0.35f, 0.35f}, kPink, false},
    {"winking",
     {1.0f, 0.0f, 0.0f, 0.35f, 0, 0, 1.02f, 1.0f, 0.1f, 0.75f, 0.0f, 0.9f},
     kCyan,
     false},
    {"cool", {0.85f, 0.38f, 0.38f, 0.0f, 8, 0, 1.0f, 1.0f, 1.0f, 0.35f, 0.0f, 0.7f}, kCyan, false},
    {"sad", {0.85f, 0.05f, 0.4f, 0.0f, 0, 8, 0.95f, 1.0f, 1.0f, -0.7f, 0.0f, 0.8f}, kBlue, false},
    {"crying",
     {0.8f, 0.05f, 0.45f, 0.0f, 0, 8, 0.95f, 1.0f, 1.0f, -0.85f, 0.2f, 0.9f},
     kBlue,
     true},
    {"angry", {0.9f, 0.5f, 0.05f, 0.0f, 0, 2, 1.0f, 1.0f, 1.0f, -0.6f, 0.0f, 0.9f}, kRed, false},
    {"surprised",
     {1.12f, 0.0f, 0.0f, 0.0f, 0, -3, 1.16f, 1.0f, 1.0f, 0.0f, 0.75f, 0.55f},
     kCyan,
     false},
    {"shocked",
     {1.18f, 0.0f, 0.0f, 0.0f, 0, -4, 1.2f, 1.0f, 1.0f, 0.0f, 0.95f, 0.6f},
     kWhite,
     false},
    {"thinking",
     {0.9f, 0.15f, 0.05f, 0.0f, -14, -10, 0.98f, 1.0f, 1.0f, -0.15f, 0.0f, 0.45f},
     kCyan,
     false},
    {"confused",
     {0.95f, 0.25f, 0.0f, 0.0f, 6, -4, 1.0f, 1.0f, 0.72f, -0.35f, 0.0f, 0.6f},
     kCyan,
     false},
    {"embarrassed",
     {0.9f, 0.0f, 0.0f, 0.35f, -10, 8, 0.96f, 1.0f, 1.0f, 0.35f, 0.0f, 0.55f},
     kBlush,
     false},
    {"sleepy", {0.28f, 0.2f, 0.2f, 0.0f, 0, 6, 1.0f, 1.0f, 1.0f, 0.0f, 0.18f, 0.35f}, kDim, false},
};

WalleFace::WalleFace(lv_obj_t* parent, int width, int height) : width_(width), height_(height) {
    current_ = kExpressions[0].pose;
    target_ = current_;
    color_ = lv_color_hex(kCyan);
    color_from_ = color_;
    color_to_ = color_;

    obj_ = lv_obj_create(parent);
    lv_obj_remove_style_all(obj_);
    lv_obj_set_size(obj_, width_, height_);
    lv_obj_set_pos(obj_, 0, 0);
    lv_obj_remove_flag(obj_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(obj_, DrawCallback, LV_EVENT_DRAW_MAIN, this);

    uint32_t now = lv_tick_get();
    next_blink_ms_ = now + RandomRange(1500, 3500);
    next_glance_ms_ = now + RandomRange(1000, 2500);
    UpdateShapes(1.0f, 0.0f, 1.0f, 1.0f);

    timer_ = lv_timer_create(TimerCallback, kFrameMs, this);
}

WalleFace::~WalleFace() {
    if (timer_ != nullptr) {
        lv_timer_delete(timer_);
    }
    if (obj_ != nullptr) {
        lv_obj_delete(obj_);
    }
}

bool WalleFace::SetEmotion(const char* emotion) {
    if (emotion == nullptr) {
        return false;
    }
    for (const auto& expression : kExpressions) {
        if (strcmp(expression.name, emotion) != 0) {
            continue;
        }
        // The application resets the face to neutral as soon as the device
        // goes back to listening, which would hide each mood after a few
        // seconds. Let a mood linger for a while before going neutral.
        bool is_neutral = &expression == &kExpressions[0];
        if (is_neutral && active_ != &kExpressions[0]) {
            neutral_pending_ = true;
            neutral_at_ms_ = lv_tick_get() + kMoodHoldMs;
            return true;
        }
        neutral_pending_ = false;
        Apply(expression);
        return true;
    }
    return false;
}

void WalleFace::Apply(const Expression& expression) {
    active_ = &expression;
    target_ = expression.pose;
    tear_ = expression.tear;
    lv_color_t color = lv_color_hex(expression.color);
    if (!lv_color_eq(color, color_to_)) {
        color_from_ = color_;
        color_to_ = color;
        color_progress_ = 0.0f;
    }
}

void WalleFace::SetMode(Mode mode) { mode_ = mode; }

void WalleFace::SetVisible(bool visible) {
    visible_ = visible;
    if (visible) {
        lv_obj_remove_flag(obj_, LV_OBJ_FLAG_HIDDEN);
        has_drawn_area_ = false;
        lv_obj_invalidate(obj_);
    } else {
        lv_obj_add_flag(obj_, LV_OBJ_FLAG_HIDDEN);
    }
}

void WalleFace::TimerCallback(lv_timer_t* timer) {
    static_cast<WalleFace*>(lv_timer_get_user_data(timer))->Tick();
}

void WalleFace::DrawCallback(lv_event_t* e) {
    auto* face = static_cast<WalleFace*>(lv_event_get_user_data(e));
    face->Draw(lv_event_get_layer(e));
}

void WalleFace::Tick() {
    if (!visible_) {
        return;
    }
    uint32_t now = lv_tick_get();

    if (neutral_pending_ && static_cast<int32_t>(now - neutral_at_ms_) >= 0) {
        neutral_pending_ = false;
        Apply(kExpressions[0]);
    }

    // Blink every few seconds, sometimes twice in a row.
    float blink = 1.0f;
    if (!blinking_ && now >= next_blink_ms_ && target_.open > 0.4f) {
        blinking_ = true;
        blink_start_ms_ = now;
    }
    if (blinking_) {
        uint32_t t = now - blink_start_ms_;
        if (t >= kBlinkMs) {
            blinking_ = false;
            next_blink_ms_ = now + (esp_random() % 5 == 0 ? 120 : RandomRange(2200, 6000));
        } else {
            float p = static_cast<float>(t) / kBlinkMs;
            float closing = p < 0.5f ? p * 2.0f : (1.0f - p) * 2.0f;
            blink = 1.0f - 0.92f * closing;
        }
    }

    // Glance around while idle or listening; look straight ahead while talking.
    if (mode_ == Mode::kSpeaking) {
        glance_x_ = 0.0f;
        glance_y_ = 0.0f;
    } else if (now >= next_glance_ms_) {
        if (esp_random() % 3 == 0) {
            glance_x_ = 0.0f;
            glance_y_ = 0.0f;
        } else {
            glance_x_ = static_cast<float>(RandomRange(0, 28)) - 14.0f;
            glance_y_ = static_cast<float>(RandomRange(0, 12)) - 6.0f;
        }
        next_glance_ms_ = now + RandomRange(1200, 4000);
    }

    float bob = 0.0f;
    float pulse_open = 1.0f;
    float pulse_scale = 1.0f;
    if (mode_ == Mode::kSpeaking) {
        pulse_open = 1.0f + 0.05f * sinf(kTwoPi * (now % 320) / 320.0f);
        bob = 2.0f * sinf(kTwoPi * (now % 640) / 640.0f);
    } else if (mode_ == Mode::kListening) {
        pulse_scale = 1.05f + 0.015f * sinf(kTwoPi * (now % 1600) / 1600.0f);
    }

    // Talking mouth. With a voice level source the mouth follows the loudness
    // of the audio actually playing; otherwise it makes random "syllables"
    // every 80-160 ms while the device is in the speaking state.
    if (voice_level_) {
        syllable_ = std::clamp(voice_level_(), 0.0f, 1.0f);
    } else if (mode_ == Mode::kSpeaking) {
        if (now >= next_syllable_ms_) {
            syllable_ = esp_random() % 6 == 0 ? 0.0f : 0.15f + RandomRange(0, 60) / 100.0f;
            next_syllable_ms_ = now + RandomRange(80, 160);
        }
    } else {
        syllable_ = 0.0f;
    }
    talk_open_ += (syllable_ - talk_open_) * 0.5f;
    if (std::fabs(syllable_ - talk_open_) < 0.01f) {
        talk_open_ = syllable_;
    }

    Approach(current_.open, target_.open);
    Approach(current_.lid_inner, target_.lid_inner);
    Approach(current_.lid_outer, target_.lid_outer);
    Approach(current_.smile, target_.smile);
    Approach(current_.look_x, target_.look_x + glance_x_);
    Approach(current_.look_y, target_.look_y + glance_y_);
    Approach(current_.scale, target_.scale);
    Approach(current_.left_open, target_.left_open);
    Approach(current_.right_open, target_.right_open);
    Approach(current_.mouth_curve, target_.mouth_curve);
    Approach(current_.mouth_open, target_.mouth_open);
    Approach(current_.mouth_width, target_.mouth_width);

    bool color_changing = color_progress_ < 1.0f;
    if (color_changing) {
        color_progress_ = std::min(1.0f, color_progress_ + kColorStep);
        color_ = lv_color_mix(color_to_, color_from_, static_cast<uint8_t>(color_progress_ * 255));
    }

    if (tear_) {
        tear_phase_ = static_cast<float>(now % kTearMs) / kTearMs;
    }

    EyeShape previous[2];
    memcpy(previous, eyes_, sizeof(eyes_));
    MouthShape previous_mouth = mouth_;
    UpdateShapes(blink, bob, pulse_open, pulse_scale);

    bool changed = memcmp(previous, eyes_, sizeof(eyes_)) != 0 ||
                   memcmp(&previous_mouth, &mouth_, sizeof(mouth_)) != 0 || color_changing || tear_;
    if (!changed && has_drawn_area_) {
        return;
    }

    // Redraw only the face: the union of the old and new areas.
    lv_area_t area = DirtyArea();
    if (has_drawn_area_) {
        lv_area_t dirty = area;
        dirty.x1 = std::min(dirty.x1, drawn_area_.x1);
        dirty.y1 = std::min(dirty.y1, drawn_area_.y1);
        dirty.x2 = std::max(dirty.x2, drawn_area_.x2);
        dirty.y2 = std::max(dirty.y2, drawn_area_.y2);
        lv_obj_invalidate_area(obj_, &dirty);
    } else {
        lv_obj_invalidate(obj_);
    }
    drawn_area_ = area;
    has_drawn_area_ = true;
}

void WalleFace::UpdateShapes(float blink, float bob, float pulse_open, float pulse_scale) {
    const float base_w = width_ * 0.26f;
    const float base_h = height_ * 0.33f;
    const float gap = width_ * 0.15f;
    const float center_y = height_ * 0.40f + current_.look_y + bob;
    const float scale = current_.scale * pulse_scale;

    for (int i = 0; i < 2; i++) {
        bool is_left = i == 0;
        float per_eye = is_left ? current_.left_open : current_.right_open;
        float open = std::max(0.0f, current_.open * per_eye * blink * pulse_open);
        // Squinting eyes get a little wider, which reads as a smile or a frown.
        float w = base_w * scale * (1.0f + (1.0f - std::min(open, 1.0f)) * 0.12f);
        float h = std::max(4.0f, base_h * scale * open);
        float side = is_left ? -1.0f : 1.0f;
        float center_x =
            width_ / 2.0f + side * (gap / 2.0f + base_w * scale / 2.0f) + current_.look_x;

        EyeShape& eye = eyes_[i];
        lv_area_set(&eye.body, static_cast<int32_t>(center_x - w / 2.0f),
                    static_cast<int32_t>(center_y - h / 2.0f),
                    static_cast<int32_t>(center_x + w / 2.0f),
                    static_cast<int32_t>(center_y + h / 2.0f));
        eye.lid_inner = current_.lid_inner;
        eye.lid_outer = current_.lid_outer;
        eye.smile = current_.smile;
    }

    // The mouth follows the gaze a little, so the face turns as one.
    float open = std::max(current_.mouth_open, talk_open_);
    mouth_.center_x = static_cast<int32_t>(width_ / 2.0f + current_.look_x * 0.6f);
    mouth_.center_y = static_cast<int32_t>(height_ * 0.76f + current_.look_y * 0.5f + bob);
    mouth_.width = static_cast<int32_t>(width_ * 0.2f * current_.mouth_width);
    mouth_.open_height = static_cast<int32_t>(open * height_ * 0.11f);
    mouth_.curve = current_.mouth_curve;
}

lv_area_t WalleFace::DirtyArea() const {
    lv_area_t area = eyes_[0].body;
    area.x2 = eyes_[1].body.x2;
    area.y1 = std::min(eyes_[0].body.y1, eyes_[1].body.y1);
    area.y2 = std::max(eyes_[0].body.y2, eyes_[1].body.y2);
    lv_area_increase(&area, kGlow + 2, kGlow + 2);
    if (tear_) {
        area.y2 += 44;
    }
    area.y2 = std::max(area.y2, mouth_.center_y + kMouthReach);
    area.x1 = std::min(area.x1, mouth_.center_x - mouth_.width / 2 - kGlow);
    area.x2 = std::max(area.x2, mouth_.center_x + mouth_.width / 2 + kGlow);
    return area;
}

void WalleFace::Draw(lv_layer_t* layer) {
    for (int i = 0; i < 2; i++) {
        DrawEye(layer, eyes_[i], i == 0);
        if (tear_) {
            DrawTear(layer, eyes_[i], i == 0);
        }
    }
    DrawMouth(layer);
}

void WalleFace::DrawEye(lv_layer_t* layer, const EyeShape& eye, bool is_left) {
    const lv_area_t& body = eye.body;
    int32_t w = lv_area_get_width(&body);
    int32_t h = lv_area_get_height(&body);
    int32_t short_side = std::min(w, h);
    // A nearly closed eye becomes a pill; an open one keeps soft square corners.
    int32_t radius = h < w / 2 ? short_side / 2 : short_side * 38 / 100;

    // Layered halos fading outwards give the screen-glow look.
    static constexpr struct {
        int32_t spread;
        lv_opa_t opa;
    } kHalos[] = {{kGlow, 12}, {kGlow * 7 / 10, 22}, {kGlow * 4 / 10, 40}, {kGlow * 2 / 10, 70}};
    lv_draw_rect_dsc_t glow;
    lv_draw_rect_dsc_init(&glow);
    glow.bg_color = color_;
    for (const auto& halo_layer : kHalos) {
        lv_area_t halo = body;
        lv_area_increase(&halo, halo_layer.spread, halo_layer.spread);
        glow.radius = radius + halo_layer.spread;
        glow.bg_opa = halo_layer.opa;
        lv_draw_rect(layer, &glow, &halo);
    }

    // Eye body, lighter at the top.
    lv_draw_rect_dsc_t fill;
    lv_draw_rect_dsc_init(&fill);
    fill.radius = radius;
    fill.bg_opa = LV_OPA_COVER;
    fill.bg_color = color_;
    fill.bg_grad.dir = LV_GRAD_DIR_VER;
    fill.bg_grad.stops_count = 2;
    fill.bg_grad.stops[0].color = lv_color_lighten(color_, LV_OPA_40);
    fill.bg_grad.stops[0].opa = LV_OPA_COVER;
    fill.bg_grad.stops[0].frac = 0;
    fill.bg_grad.stops[1].color = color_;
    fill.bg_grad.stops[1].opa = LV_OPA_COVER;
    fill.bg_grad.stops[1].frac = 255;
    lv_draw_rect(layer, &fill, &body);

    // Small highlight near the top-left corner.
    if (h > 24) {
        lv_draw_rect_dsc_t glint;
        lv_draw_rect_dsc_init(&glint);
        glint.radius = LV_RADIUS_CIRCLE;
        glint.bg_color = lv_color_white();
        glint.bg_opa = LV_OPA_40;
        lv_area_t spot;
        spot.x1 = body.x1 + w * 16 / 100;
        spot.y1 = body.y1 + h * 10 / 100;
        spot.x2 = spot.x1 + w * 24 / 100;
        spot.y2 = spot.y1 + std::max<int32_t>(4, h * 13 / 100);
        lv_draw_rect(layer, &glint, &spot);
    }

    const lv_color_t background = lv_color_black();
    const int32_t pad = kGlow + 2;

    // Top lid: a slanted cut, lower at the nose for angry and at the outside for sad.
    if (eye.lid_inner > 0.01f || eye.lid_outer > 0.01f) {
        float lid_left = is_left ? eye.lid_outer : eye.lid_inner;
        float lid_right = is_left ? eye.lid_inner : eye.lid_outer;
        float y_left = body.y1 + h * lid_left;
        float y_right = body.y1 + h * lid_right;
        float slope = (y_right - y_left) / std::max<int32_t>(w, 1);
        float x_left = body.x1 - pad;
        float x_right = body.x2 + pad;
        float top = body.y1 - pad - 1;
        y_left -= slope * pad;
        y_right += slope * pad;

        // A rectangle down to the higher corner plus one triangle for the slant,
        // so the only anti-aliased edge is the lid line itself.
        float y_min = std::min(y_left, y_right);
        int32_t rect_bottom = static_cast<int32_t>(y_min);
        lv_draw_rect_dsc_t cover;
        lv_draw_rect_dsc_init(&cover);
        cover.bg_color = background;
        cover.bg_opa = LV_OPA_COVER;
        lv_area_t band;
        lv_area_set(&band, static_cast<int32_t>(x_left), static_cast<int32_t>(top),
                    static_cast<int32_t>(x_right), rect_bottom + 1);
        lv_draw_rect(layer, &cover, &band);

        if (std::fabs(y_left - y_right) >= 1.0f) {
            lv_draw_triangle_dsc_t lid;
            lv_draw_triangle_dsc_init(&lid);
            lid.color = background;
            lid.opa = LV_OPA_COVER;
            lid.p[0].x = static_cast<lv_value_precise_t>(x_left);
            lid.p[0].y = static_cast<lv_value_precise_t>(rect_bottom);
            lid.p[1].x = static_cast<lv_value_precise_t>(x_right);
            lid.p[1].y = static_cast<lv_value_precise_t>(rect_bottom);
            bool left_is_lower = y_left > y_right;
            lid.p[2].x = static_cast<lv_value_precise_t>(left_is_lower ? x_left : x_right);
            lid.p[2].y = static_cast<lv_value_precise_t>(left_is_lower ? y_left : y_right);
            lv_draw_triangle(layer, &lid);
        }
    }

    // Smile: a circle rising from below carves the eye into a "^" arc.
    if (eye.smile > 0.01f) {
        lv_draw_rect_dsc_t cut;
        lv_draw_rect_dsc_init(&cut);
        cut.radius = LV_RADIUS_CIRCLE;
        cut.bg_color = background;
        cut.bg_opa = LV_OPA_COVER;
        int32_t diameter = w * 16 / 10;
        lv_area_t circle;
        circle.x1 = body.x1 + w / 2 - diameter / 2;
        circle.x2 = circle.x1 + diameter;
        circle.y1 = body.y1 + static_cast<int32_t>(h * (1.0f - eye.smile));
        circle.y2 = circle.y1 + diameter;
        lv_draw_rect(layer, &cut, &circle);
    }
}

void WalleFace::DrawTear(lv_layer_t* layer, const EyeShape& eye, bool is_left) {
    int32_t w = lv_area_get_width(&eye.body);
    int32_t size = 10;
    int32_t x = is_left ? eye.body.x1 + w * 15 / 100 : eye.body.x2 - w * 15 / 100 - size;
    int32_t y = eye.body.y2 + 4 + static_cast<int32_t>(tear_phase_ * 30.0f);

    lv_draw_rect_dsc_t drop;
    lv_draw_rect_dsc_init(&drop);
    drop.radius = LV_RADIUS_CIRCLE;
    drop.bg_color = lv_color_hex(kTearColor);
    drop.bg_opa = static_cast<lv_opa_t>(255 * (1.0f - tear_phase_));
    lv_area_t area = {x, y, x + size, y + size + 3};
    lv_draw_rect(layer, &drop, &area);
}

void WalleFace::DrawMouth(lv_layer_t* layer) {
    const MouthShape& m = mouth_;
    if (m.width <= 0) {
        return;
    }
    const int32_t half_w = m.width / 2;

    if (m.open_height >= kMouthStroke) {
        // Open mouth: a glowing ring with a dark inside.
        lv_area_t outer;
        lv_area_set(&outer, m.center_x - half_w, m.center_y - m.open_height / 2,
                    m.center_x + half_w, m.center_y + m.open_height / 2);
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.radius = LV_RADIUS_CIRCLE;
        dsc.bg_color = color_;
        lv_area_t halo = outer;
        lv_area_increase(&halo, kGlow / 2, kGlow / 2);
        dsc.bg_opa = LV_OPA_20;
        lv_draw_rect(layer, &dsc, &halo);
        dsc.bg_opa = LV_OPA_COVER;
        lv_draw_rect(layer, &dsc, &outer);

        lv_area_t inner = outer;
        lv_area_increase(&inner, -kMouthStroke / 2 - 1, -kMouthStroke / 2 - 1);
        if (lv_area_get_height(&inner) > 2 && lv_area_get_width(&inner) > 2) {
            dsc.bg_color = lv_color_black();
            lv_draw_rect(layer, &dsc, &inner);
        }
        return;
    }

    if (std::fabs(m.curve) < 0.12f) {
        // Flat mouth: a short pill.
        lv_area_t line;
        lv_area_set(&line, m.center_x - half_w, m.center_y - kMouthStroke / 2, m.center_x + half_w,
                    m.center_y + kMouthStroke / 2);
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.radius = LV_RADIUS_CIRCLE;
        dsc.bg_color = color_;
        lv_area_t halo = line;
        lv_area_increase(&halo, kGlow / 2, kGlow / 2);
        dsc.bg_opa = LV_OPA_20;
        lv_draw_rect(layer, &dsc, &halo);
        dsc.bg_opa = LV_OPA_COVER;
        lv_draw_rect(layer, &dsc, &line);
        return;
    }

    // Smile or frown: an arc whose chord is the mouth width. A stronger curve
    // uses a smaller circle, so the arc bends more.
    float half_angle = asinf(std::min(0.95f, 0.83f * std::fabs(m.curve)));
    float radius = half_w / sinf(half_angle);
    float sag = radius * (1.0f - cosf(half_angle));
    bool smile = m.curve > 0.0f;
    float center_y = smile ? m.center_y + sag / 2.0f - radius : m.center_y - sag / 2.0f + radius;
    float half_deg = half_angle * 180.0f / 3.14159265f;
    float mid_deg = smile ? 90.0f : 270.0f;

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.center.x = m.center_x;
    arc.center.y = static_cast<int32_t>(center_y);
    arc.start_angle = static_cast<lv_value_precise_t>(mid_deg - half_deg);
    arc.end_angle = static_cast<lv_value_precise_t>(mid_deg + half_deg);
    arc.rounded = 1;
    arc.color = color_;

    arc.width = kMouthStroke + kGlow;
    arc.radius = static_cast<uint16_t>(radius + (kMouthStroke + kGlow) / 2.0f);
    arc.opa = LV_OPA_20;
    lv_draw_arc(layer, &arc);

    arc.width = kMouthStroke;
    arc.radius = static_cast<uint16_t>(radius + kMouthStroke / 2.0f);
    arc.opa = LV_OPA_COVER;
    lv_draw_arc(layer, &arc);
}
