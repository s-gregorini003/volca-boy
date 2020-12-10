#ifndef CONFIG_H
#define CONFIG_H

//  Hardware pins configuration

#define BANK_A_PIN A1
#define BANK_B_PIN A2
#define OPTIONS_PIN A3

#define POT_SW_PIN 44
#define POT_DT_PIN 43
#define POT_CLK_PIN 42

#define OLED_RST_PIN 8

#define PUSH_BUTTON_PIN 34


//  Sequencer configuration

#define TRACK1_CHANNEL  1
#define TRACK2_CHANNEL  2
#define TRACK3_CHANNEL  3
#define TRACK4_CHANNEL  4

#define STEP_MAX_SIZE 8
#define CHANNEL_MAX_SIZE 4
#define BPM 220

//  Color palette configuration

uint32_t SELECTED_COLOR[4] = {0x00D9EC, 0x17E5DB, 0x57EFC1, 0x8DF6A3};
uint32_t ARMED_COLOR = 0x1300EC;
uint32_t  CURSOR_COLOR[4] = { 0xFFC800, 0xFF6900, 0xFF3600, 0xFF1300};

#endif
