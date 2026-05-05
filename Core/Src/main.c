// Desert Dash — 2D side-scrolling runner on the STM32L476 Nucleo.
//
// Pin map:
//   LCD  : PB15 MOSI | PB13 SCK | PB12 CS | PB11 DC | PB1 BL
//   Joy  : PC0 VRx (ADC_CH1)  PC1 VRy (ADC_CH2)  PC3 SW
//   LED  : PB6 (TIM4 CH1 PWM)    Buzzer: PB10 (TIM2 CH3)
//   BTN3 : PC3 (EXTI3, falling edge) — confirm / start / retry
//
// NOTE: PA5 is the Nucleo LD2 LED and cannot be used as ADC_IN10 at the same
//       time — joystick VRx is wired to PC0 (ADC_CH1) instead.

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

#include "Game.h"
#include "Player.h"
#include "Renderer.h"

#include <stdint.h>
#include <stdio.h>

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

// set by EXTI callback in stm32l4xx_it.c, cleared after reading
volatile uint8_t g_btn3_pressed = 0;

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

// Buzzer on TIM2 CH3 (PB10)
Buzzer_cfg_t buzzer_cfg = {
    .htim = &htim2, .channel = TIM_CHANNEL_3,
    .tick_freq_hz = 1000000,
    .min_freq_hz  = 20,
    .max_freq_hz  = 20000,
    .setup_done   = 0
};

// Atmosphere LED on TIM4 CH1 (PB6)
PWM_cfg_t pwm_cfg = {
    .htim = &htim4, .channel = TIM_CHANNEL_1,
    .tick_freq_hz = 1000000,
    .min_freq_hz  = 10,
    .max_freq_hz  = 50000,
    .setup_done   = 0
};

// Joystick on ADC1 (PC0 = VRx, PC1 = VRy)
Joystick_cfg_t joystick_cfg = {
    .adc          = &hadc1,
    .x_channel    = ADC_CHANNEL_1,
    .y_channel    = ADC_CHANNEL_2,
    .sampling_time = ADC_SAMPLETIME_47CYCLES_5,
    .center_x     = JOYSTICK_DEFAULT_CENTER_X,
    .center_y     = JOYSTICK_DEFAULT_CENTER_Y,
    .deadzone     = JOYSTICK_DEADZONE,
    .setup_done   = 0
};
Joystick_t joystick_data;

static Game_t    game;
static Player_t  player;
static uint32_t  s_green_off_tick = 0;

#define FPS           30
#define FRAME_TIME_MS (1000 / FPS)

static uint8_t btn3_consume(void) {
    if (!g_btn3_pressed) return 0;
    g_btn3_pressed = 0;
    return 1;
}

static void start_new_game(void) {
    Game_Init(&game);
    game.state = STATE_PLAYING;
    Player_Init(&player);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
    s_green_off_tick = HAL_GetTick() + 8000;  // keep it on for a while, looks good on the board
    buzzer_game_start(&buzzer_cfg);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    PeriphCommonClock_Config();

    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();
    MX_RNG_Init();

    // LCD before TIM4 — both share GPIOB pins, order matters
    LCD_init(&cfg0);

    MX_TIM2_Init();
    buzzer_init(&buzzer_cfg);

    MX_TIM4_Init();
    PWM_Init(&pwm_cfg);
    PWM_SetFreq(&pwm_cfg, 1000);
    PWM_SetDuty(&pwm_cfg, 0);

    buzzer_tone(&buzzer_cfg, 523, 100); HAL_Delay(150);
    buzzer_tone(&buzzer_cfg, 784, 100); HAL_Delay(150);
    buzzer_off(&buzzer_cfg);

    // LED startup test — green on D8 (PA9), red on D10 (PC8)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    HAL_Delay(400);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(400);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);

    Joystick_Init(&joystick_cfg);
    Joystick_Calibrate(&joystick_cfg);   // ~500 ms blocking, keep stick centred

    Game_Init(&game);       // sets state = STATE_MENU
    Player_Init(&player);
    Renderer_Init(&cfg0);

    printf("Desert Dash ready.\n");

    while (1)
    {
        uint32_t frame_start = HAL_GetTick();
        uint8_t  btn         = btn3_consume();

        if (game.state == STATE_MENU) {
            if (btn) start_new_game();

            LCD_Fill_Buffer(0);
            Renderer_DrawMenu();
            Renderer_Present();

            uint32_t elapsed = HAL_GetTick() - frame_start;
            if (elapsed < FRAME_TIME_MS) HAL_Delay(FRAME_TIME_MS - elapsed);
            continue;
        }

        if (game.state == STATE_GAMEOVER) {
            if (btn) {
                game.state = STATE_MENU;
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
            }

            LCD_Fill_Buffer(0);
            Renderer_DrawBackground(game.day);
            Renderer_DrawGameOver(game.score, game.coins);
            Renderer_Present();

            uint32_t elapsed = HAL_GetTick() - frame_start;
            if (elapsed < FRAME_TIME_MS) HAL_Delay(FRAME_TIME_MS - elapsed);
            continue;
        }

        Joystick_Read(&joystick_cfg, &joystick_data);
        float joyX = joystick_data.coord_mapped.x;
        float joyY = joystick_data.coord_mapped.y;

        Player_Update(&player, joyX, joyY);
        GameState_t prev_state = game.state;
        Game_Update(&game, player.lane, player.jumping, player.sliding);
        if (prev_state == STATE_PLAYING && game.state == STATE_GAMEOVER) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
            s_green_off_tick = 0;
            buzzer_game_over(&buzzer_cfg);
        }

        if (s_green_off_tick && HAL_GetTick() >= s_green_off_tick) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
            s_green_off_tick = 0;
        }

        LCD_Fill_Buffer(0);
        Renderer_DrawBackground(game.day);
        Renderer_DrawObstacles(game.obstacles, MAX_OBSTACLES);
        Renderer_DrawCoins(game.coinPool, MAX_COINS);
        Renderer_DrawPlayer(&player);
        Renderer_DrawHUD(game.score, game.coins);
        Renderer_Present();

        uint32_t elapsed = HAL_GetTick() - frame_start;
        if (elapsed < FRAME_TIME_MS) HAL_Delay(FRAME_TIME_MS - elapsed);
    }
}


// STM32CubeMX generated — do not edit
void SystemClock_Config(void)
{
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

void PeriphCommonClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    PeriphClkInit.PeriphClockSelection      = RCC_PERIPHCLK_RNG | RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection         = RCC_ADCCLKSOURCE_PLLSAI1;
    PeriphClkInit.RngClockSelection         = RCC_RNGCLKSOURCE_PLLSAI1;
    PeriphClkInit.PLLSAI1.PLLSAI1Source     = RCC_PLLSOURCE_HSI;
    PeriphClkInit.PLLSAI1.PLLSAI1M         = 1;
    PeriphClkInit.PLLSAI1.PLLSAI1N         = 8;
    PeriphClkInit.PLLSAI1.PLLSAI1P         = RCC_PLLP_DIV7;
    PeriphClkInit.PLLSAI1.PLLSAI1Q         = RCC_PLLQ_DIV4;
    PeriphClkInit.PLLSAI1.PLLSAI1R         = RCC_PLLR_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1ClockOut  = RCC_PLLSAI1_48M2CLK | RCC_PLLSAI1_ADC1CLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line;
}
#endif
