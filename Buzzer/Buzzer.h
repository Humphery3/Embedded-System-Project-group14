#pragma once
#include <stdint.h>
#include "stm32l4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// note frequencies (Hz) C4-C8
typedef enum {
    NOTE_C4  = 262,  NOTE_CS4 = 277,  NOTE_D4  = 294,  NOTE_DS4 = 311,
    NOTE_E4  = 330,  NOTE_F4  = 349,  NOTE_FS4 = 370,  NOTE_G4  = 392,
    NOTE_GS4 = 415,  NOTE_A4  = 440,  NOTE_AS4 = 466,  NOTE_B4  = 494,
    NOTE_C5  = 523,  NOTE_CS5 = 554,  NOTE_D5  = 587,  NOTE_DS5 = 622,
    NOTE_E5  = 659,  NOTE_F5  = 698,  NOTE_FS5 = 740,  NOTE_G5  = 784,
    NOTE_GS5 = 831,  NOTE_A5  = 880,  NOTE_AS5 = 932,  NOTE_B5  = 988,
    NOTE_C6  = 1047, NOTE_CS6 = 1109, NOTE_D6  = 1175, NOTE_DS6 = 1245,
    NOTE_E6  = 1319, NOTE_F6  = 1397, NOTE_FS6 = 1480, NOTE_G6  = 1568,
    NOTE_GS6 = 1661, NOTE_A6  = 1760, NOTE_AS6 = 1865, NOTE_B6  = 1976,
    NOTE_C7  = 2093, NOTE_CS7 = 2217, NOTE_D7  = 2349, NOTE_DS7 = 2489,
    NOTE_E7  = 2637, NOTE_F7  = 2794, NOTE_FS7 = 2960, NOTE_G7  = 3136,
    NOTE_GS7 = 3322, NOTE_A7  = 3520, NOTE_AS7 = 3729, NOTE_B7  = 3951,
    NOTE_C8  = 4186
} Buzzer_Note_t;

typedef struct {
    TIM_HandleTypeDef* htim;
    uint32_t channel;
    uint32_t tick_freq_hz;
    uint32_t min_freq_hz;
    uint32_t max_freq_hz;
    uint8_t  setup_done;
    uint8_t  pwm_started;
} Buzzer_cfg_t;

void    buzzer_init(Buzzer_cfg_t* cfg);
void    buzzer_tone(Buzzer_cfg_t* cfg, uint32_t freq_hz, uint8_t volume_percent);
void    buzzer_note(Buzzer_cfg_t* cfg, Buzzer_Note_t note, uint8_t volume_percent);
void    buzzer_off(Buzzer_cfg_t* cfg);
uint8_t buzzer_is_running(Buzzer_cfg_t* cfg);

void    buzzer_game_start(Buzzer_cfg_t* cfg);
void    buzzer_game_over(Buzzer_cfg_t* cfg);

#ifdef __cplusplus
}
#endif
