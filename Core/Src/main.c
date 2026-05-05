// Fireboy & Watergirl — STM32L476 embedded game
// Inspired by the classic Fireboy & Watergirl game
// Built on top of the Pong foundation from the lab
//
// Game loop: INPUT > UPDATE > RENDER
//
// Architecture:
//   GameEngine  : responsible for game states and loading screens
//   Player      : handles player physics and collision  logic
//   Level       : creates the map layout for each level
//   Sprites     : writes the sprite data onto the LCD
//   Music       : buzzer sounds including for death and win
//
// Hardware:
//   LCD   : 240x240
//   Input : Joystick 1 = Fireboy, Joystick 2 = Watergirl
//   Audio : Buzzer

#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "adc.h"
#include "rng.h"

void SystemClock_Config(void);
void PeriphCommonClock_Config(void);

#include "Buzzer.h"
#include "PWM.h"
#include "LCD.h"
#include "Joystick.h"
#include "GameEngine.h"
#include "Player.h"
#include "Utils.h"

#include <stdint.h>
#include <stdio.h>

// buzzer on TIM2 channel 3
Buzzer_cfg_t buzzer_cfg = {
    .htim        = &htim2,
    .channel     = TIM_CHANNEL_3,
    .tick_freq_hz = 1000000,
    .min_freq_hz = 20,
    .max_freq_hz = 20000,
    .setup_done  = 0
};

// LCD on SPI2
ST7789V2_cfg_t cfg0 = {
    .setup_done = 0,
    .spi  = SPI2,
    .RST  = {.port = GPIOB, .pin = GPIO_PIN_2},
    .BL   = {.port = GPIOB, .pin = GPIO_PIN_1},
    .DC   = {.port = GPIOB, .pin = GPIO_PIN_11},
    .CS   = {.port = GPIOB, .pin = GPIO_PIN_12},
    .MOSI = {.port = GPIOB, .pin = GPIO_PIN_15},
    .SCLK = {.port = GPIOB, .pin = GPIO_PIN_13},
    .dma  = {.instance = DMA1, .channel = DMA1_Channel5}
};

// fireboy joystick — ADC1 channels 1 and 2
Joystick_cfg_t joystick_fb = {
    .adc           = &hadc1,
    .x_channel     = ADC_CHANNEL_1,
    .y_channel     = ADC_CHANNEL_2,
    .sampling_time = ADC_SAMPLETIME_47CYCLES_5,
    .center_x      = JOYSTICK_DEFAULT_CENTER_X,
    .center_y      = JOYSTICK_DEFAULT_CENTER_Y,
    .deadzone      = JOYSTICK_DEADZONE,
    .setup_done    = 0
};

// watergirl joystick — ADC1 channels 5 and 6, SW button at PC10
Joystick_cfg_t joystick_wg = {
    .adc           = &hadc1,
    .x_channel     = ADC_CHANNEL_6,
    .y_channel     = ADC_CHANNEL_5,
    .sampling_time = ADC_SAMPLETIME_47CYCLES_5,
    .center_x      = JOYSTICK_DEFAULT_CENTER_X,
    .center_y      = JOYSTICK_DEFAULT_CENTER_Y,
    .deadzone      = JOYSTICK_DEADZONE,
    .setup_done    = 0
};

// PWM on TIM4 channel 1 (used for green LED on PB6)
PWM_cfg_t pwm_cfg = {
    .htim        = &htim4,
    .channel     = TIM_CHANNEL_1,
    .tick_freq_hz = 1000000,
    .min_freq_hz = 10,
    .max_freq_hz = 50000,
    .setup_done  = 0
};

static GameEngine ge;
static Joystick_t joy_fb_data, joy_wg_data;

#define FPS           30
#define FRAME_TIME_MS (1000 / FPS)

// redirect printf to UART for debugging
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

int main(void) {

    HAL_Init();
    SystemClock_Config();
    PeriphCommonClock_Config();

    // freeze watchdogs so the debugger can halt the CPU
    __HAL_DBGMCU_FREEZE_IWDG();
    __HAL_DBGMCU_FREEZE_WWDG();

    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();

    // watergirl SW button at PC10 — active-low with pull-up
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef sw_gpio = {0};
    sw_gpio.Pin  = GPIO_PIN_10;
    sw_gpio.Mode = GPIO_MODE_INPUT;
    sw_gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &sw_gpio);
    MX_RNG_Init();

    LCD_init(&cfg0);

    MX_TIM2_Init();
    buzzer_init(&buzzer_cfg);

    // startup beep to confirm buzzer is wired correctly
    buzzer_tone(&buzzer_cfg, 880, 16);
    HAL_Delay(180);
    buzzer_tone(&buzzer_cfg, 440, 16);
    HAL_Delay(180);
    buzzer_off(&buzzer_cfg);
    HAL_Delay(60);

    // init TIM4 after LCD to avoid GPIO conflict on PB6
    MX_TIM4_Init();
    PWM_Init(&pwm_cfg);
    PWM_SetDuty(&pwm_cfg, 0);

    // green LED on PB6
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led_gpio = {0};
    led_gpio.Pin   = GPIO_PIN_6;
    led_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    led_gpio.Pull  = GPIO_NOPULL;
    led_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led_gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

    Joystick_Init(&joystick_fb);
    Joystick_Init(&joystick_wg);

    GameEngine_Init(&ge);

    printf("Fireboy & Watergirl initialised.\n");

    uint32_t last_tick = HAL_GetTick();
    bool sw_prev = false;

    while (1) {
        uint32_t now = HAL_GetTick();
        if ((now - last_tick) < FRAME_TIME_MS) continue;
        last_tick = now;

        // INPUT
        Joystick_Read(&joystick_fb, &joy_fb_data);
        UserInput fb_in = Joystick_GetInput(&joy_fb_data);

        Joystick_Read(&joystick_wg, &joy_wg_data);
        UserInput wg_in = Joystick_GetInput(&joy_wg_data);

        // SW at PC10 active-low — detect rising edge for pause
        bool sw_now   = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_10) == GPIO_PIN_RESET);
        bool pause_btn = sw_now && !sw_prev;
        sw_prev = sw_now;

        // also use SW as watergirl jump when not pausing
        if (sw_now && !pause_btn && ge.state == GS_PLAY) {
            if (wg_in.direction == CENTRE)
                wg_in.direction = N;
        }

        // UPDATE
        GameEngine_Update(&ge, fb_in, wg_in, pause_btn);

        // green LED on while playing, off in menus/pause/dead
        if (ge.state == GS_PLAY) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        }

        // RENDER
        LCD_Fill_Buffer(0);
        GameEngine_DrawScene(&ge);
        LCD_Refresh(&cfg0);

        // draw sprites on top — RGB565 direct to hardware
        if (ge.state == GS_PLAY || ge.state == GS_PAUSE || ge.state == GS_TITLE) {
            Player_Draw(&ge.fireboy);
            Player_Draw(&ge.watergirl);
        }
    }
}

// auto-generated STM32 functions below — do not edit

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
        Error_Handler();

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 1;
    RCC_OscInitStruct.PLL.PLLN            = 10;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        Error_Handler();
}

void PeriphCommonClock_Config(void) {
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    PeriphClkInit.PeriphClockSelection        = RCC_PERIPHCLK_RNG | RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection           = RCC_ADCCLKSOURCE_PLLSAI1;
    PeriphClkInit.RngClockSelection           = RCC_RNGCLKSOURCE_PLLSAI1;
    PeriphClkInit.PLLSAI1.PLLSAI1Source      = RCC_PLLSOURCE_HSI;
    PeriphClkInit.PLLSAI1.PLLSAI1M           = 1;
    PeriphClkInit.PLLSAI1.PLLSAI1N           = 8;
    PeriphClkInit.PLLSAI1.PLLSAI1P           = RCC_PLLP_DIV7;
    PeriphClkInit.PLLSAI1.PLLSAI1Q           = RCC_PLLQ_DIV4;
    PeriphClkInit.PLLSAI1.PLLSAI1R           = RCC_PLLR_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1ClockOut    = RCC_PLLSAI1_48M2CLK | RCC_PLLSAI1_ADC1CLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        Error_Handler();
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
