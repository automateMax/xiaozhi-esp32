#include "application.h"
#include "button.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "display/lcd_display.h"
#include "esp_video.h"
#include "led/gpio_led.h"
#include "mcp_server.h"
#include "walle_motors.h"
#include "wifi_board.h"

#include <driver/spi_common.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>

#include <string>

#define TAG "XiaoEsp32s3Walle"

class XiaoEsp32s3Walle : public WifiBoard {
private:
    Button boot_button_;
    LcdDisplay* display_ = nullptr;
    EspVideo* camera_ = nullptr;
    WalleMotors* motors_ = nullptr;

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        // Jumper-wired module: stay at 40 MHz for signal integrity.
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        esp_lcd_panel_disp_on_off(panel, true);

        display_ = new SpiLcdDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                     DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X,
                                     DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    void InitializeCamera() {
        static esp_cam_ctlr_dvp_pin_config_t dvp_pin_config = {
            .data_width = CAM_CTLR_DATA_WIDTH_8,
            .data_io =
                {
                    [0] = CAMERA_PIN_D0,
                    [1] = CAMERA_PIN_D1,
                    [2] = CAMERA_PIN_D2,
                    [3] = CAMERA_PIN_D3,
                    [4] = CAMERA_PIN_D4,
                    [5] = CAMERA_PIN_D5,
                    [6] = CAMERA_PIN_D6,
                    [7] = CAMERA_PIN_D7,
                },
            .vsync_io = CAMERA_PIN_VSYNC,
            .de_io = CAMERA_PIN_HREF,
            .pclk_io = CAMERA_PIN_PCLK,
            .xclk_io = CAMERA_PIN_XCLK,
        };

        esp_video_init_sccb_config_t sccb_config = {
            .init_sccb = true,
            .i2c_config =
                {
                    .port = 1,
                    .scl_pin = CAMERA_PIN_SIOC,
                    .sda_pin = CAMERA_PIN_SIOD,
                },
            .freq = 100000,
        };

        esp_video_init_dvp_config_t dvp_config = {
            .sccb_config = sccb_config,
            .reset_pin = CAMERA_PIN_RESET,
            .pwdn_pin = CAMERA_PIN_PWDN,
            .dvp_pin = dvp_pin_config,
            .xclk_freq = CAMERA_XCLK_FREQ_HZ,
        };

        esp_video_init_config_t video_config = {
            .dvp = &dvp_config,
        };

        // Orientation comes from the camera_hmirror/camera_vflip build options.
        camera_ = new EspVideo(video_config);
    }

    void InitializeMotors() {
        motors_ = new WalleMotors(MOTOR_LEFT_IN_A_PIN, MOTOR_LEFT_IN_B_PIN, MOTOR_RIGHT_IN_A_PIN,
                                  MOTOR_RIGHT_IN_B_PIN, MOTOR_LEFT_INVERT, MOTOR_RIGHT_INVERT);
    }

    void DriveTracks(int left_speed, int right_speed, int duration_ms) {
        if (MOTOR_SWAP_SIDES) {
            motors_->DriveFor(right_speed, left_speed, duration_ms);
        } else {
            motors_->DriveFor(left_speed, right_speed, duration_ms);
        }
    }

    void InitializeTools() {
        auto& mcp_server = McpServer::GetInstance();

        mcp_server.AddTool(
            "self.walle.move",
            "Drive WALL-E on its tracks for a short time, then stop automatically.\n"
            "Args:\n"
            "  `direction`: one of `forward`, `backward`, `left` (turn left in place), "
            "`right` (turn right in place).\n"
            "  `speed`: 1-100.\n"
            "  `duration_ms`: how long to drive, 100-5000 milliseconds.",
            PropertyList({Property("direction", kPropertyTypeString),
                          Property("speed", kPropertyTypeInteger, 70, 1, 100),
                          Property("duration_ms", kPropertyTypeInteger, 1000, 100, 5000)}),
            [this](const PropertyList& properties) -> ReturnValue {
                auto direction = properties["direction"].value<std::string>();
                int speed = properties["speed"].value<int>();
                int duration_ms = properties["duration_ms"].value<int>();
                if (direction == "forward") {
                    DriveTracks(speed, speed, duration_ms);
                } else if (direction == "backward") {
                    DriveTracks(-speed, -speed, duration_ms);
                } else if (direction == "left") {
                    DriveTracks(-speed, speed, duration_ms);
                } else if (direction == "right") {
                    DriveTracks(speed, -speed, duration_ms);
                } else {
                    ESP_LOGW(TAG, "Unknown direction: %s", direction.c_str());
                    return false;
                }
                return true;
            });

        mcp_server.AddTool("self.walle.stop", "Stop WALL-E's tracks immediately.", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               motors_->Stop();
                               return true;
                           });
    }

public:
    XiaoEsp32s3Walle() : boot_button_(BOOT_BUTTON_GPIO) {
        // Motors first, so the H-bridge inputs are driven low as early as possible.
        InitializeMotors();
        InitializeSpi();
        InitializeDisplay();
        InitializeButtons();
        InitializeCamera();
        InitializeTools();
    }

    virtual Led* GetLed() override {
        static GpioLed led(BUILTIN_LED_GPIO, 1);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplexPdm audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                                  AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK,
                                                  AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK,
                                                  AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override { return display_; }

    virtual Camera* GetCamera() override { return camera_; }
};

DECLARE_BOARD(XiaoEsp32s3Walle);
