#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Seeed Studio XIAO ESP32-S3 Sense wired as a WALL-E desk robot.
// Header pin to GPIO map (XIAO ESP32S3):
//   D0=1  D1=2  D2=3  D3=4  D4=5  D5=6  D6=43  D7=44  D8=7  D9=8  D10=9

// Audio: onboard PDM microphone + MAX98357A I2S amplifier
#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_42   // onboard PDM mic CLK
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_41   // onboard PDM mic DATA
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_5   // D4 -> MAX98357A DIN
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_6   // D5 -> MAX98357A BCLK
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_43  // D6 -> MAX98357A LRC

// Buttons and LED
#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define BUILTIN_LED_GPIO GPIO_NUM_21  // user LED, active low

// Display: 1.54" 240x240 ST7789 SPI, CS tied to GND, BL tied to 3V3
#define DISPLAY_SPI_SCLK_PIN GPIO_NUM_9  // D10 -> SCL
#define DISPLAY_SPI_MOSI_PIN GPIO_NUM_8  // D9  -> SDA
#define DISPLAY_RST_PIN GPIO_NUM_7       // D8  -> RST
#define DISPLAY_DC_PIN GPIO_NUM_44       // D7  -> DC
#define DISPLAY_SPI_CS_PIN GPIO_NUM_NC
#define DISPLAY_SPI_MODE 3

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR true
#define DISPLAY_RGB_ORDER LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0

// Track motors: 2 N20 gear motors through an MX1508 dual H-bridge
#define MOTOR_LEFT_IN_A_PIN GPIO_NUM_4   // D3 -> IN1 (motor A)
#define MOTOR_LEFT_IN_B_PIN GPIO_NUM_3   // D2 -> IN2 (motor A)
#define MOTOR_RIGHT_IN_A_PIN GPIO_NUM_2  // D1 -> IN3 (motor B)
#define MOTOR_RIGHT_IN_B_PIN GPIO_NUM_1  // D0 -> IN4 (motor B)
// Flip these if a track runs backwards, instead of rewiring.
#define MOTOR_LEFT_INVERT false
#define MOTOR_RIGHT_INVERT false
// Swap these if "left" turns the robot right.
#define MOTOR_SWAP_SIDES false

// Camera: XIAO ESP32S3 Sense DVP connector (OV2640 or OV3660 stock, OV5640 upgrade)
#define CAMERA_PIN_PWDN GPIO_NUM_NC
#define CAMERA_PIN_RESET GPIO_NUM_NC
#define CAMERA_PIN_XCLK GPIO_NUM_10
#define CAMERA_PIN_SIOD GPIO_NUM_40
#define CAMERA_PIN_SIOC GPIO_NUM_39

#define CAMERA_PIN_D7 GPIO_NUM_48
#define CAMERA_PIN_D6 GPIO_NUM_11
#define CAMERA_PIN_D5 GPIO_NUM_12
#define CAMERA_PIN_D4 GPIO_NUM_14
#define CAMERA_PIN_D3 GPIO_NUM_16
#define CAMERA_PIN_D2 GPIO_NUM_18
#define CAMERA_PIN_D1 GPIO_NUM_17
#define CAMERA_PIN_D0 GPIO_NUM_15
#define CAMERA_PIN_VSYNC GPIO_NUM_38
#define CAMERA_PIN_HREF GPIO_NUM_47
#define CAMERA_PIN_PCLK GPIO_NUM_13

#define CAMERA_XCLK_FREQ_HZ 20000000

#endif  // _BOARD_CONFIG_H_
