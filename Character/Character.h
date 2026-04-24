#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>
#include "Joystick.h"
#include "LCD.h"

/**
 * @file Character.h
 * @brief Simple sprite controller with directional movement and state
 * 
 * Use Joystick_t direction (N/S/E/W) to move sprite around screen.
 * Supports IDLE, WALKING, and DASHING states based on movement.
 */

// ===== CHARACTER STATES =====

typedef enum {
    CHAR_IDLE = 0,      // Not moving
    CHAR_WALKING,       // Moving in a direction
    CHAR_DASHING        // Fast movement (button triggered)
} CharacterState_t;

// ===== CHARACTER DATA =====

/**
 * @struct Character_t
 * @brief Sprite position and state
 * 
 * Minimal structure: just the data needed
 * - Position on screen
 * - Current state (IDLE, WALKING, DASHING)
 * - Animation frame (for walking animation)
 */
typedef struct {
    int16_t x;                      // X position
    int16_t y;                      // Y position
    CharacterState_t state;         // Current state
    uint8_t animation_frame;        // 0 or 1 (walk cycle)
    uint8_t frame_counter;          // Counter for animation timing
    uint8_t dash_counter;           // Frames remaining in dash
} Character_t;

// ===== CONSTANTS =====

#define CHAR_SPEED 2                // Pixels per frame (normal)
#define CHAR_DASH_SPEED 5           // Pixels per frame (dashing)
#define CHAR_DASH_DURATION 20       // Frames (dash lasts this long)

// ===== FUNCTIONS =====

/**
 * @brief Initialize character at screen center
 */
void Character_Init(Character_t* character);

/**
 * @brief Update character position and state
 * 
 * Uses joy->direction for 8-way movement
 * Sets state to WALKING when moving, IDLE when stopped
 * Handles dash countdown and speed boost
 */
void Character_Update(Character_t* character, Joystick_t* joy, uint8_t dash_pressed);

/**
 * @brief Draw character sprite on LCD
 * 
 * Draws different sprite based on current state:
 * - IDLE: standing sprite
 * - WALKING: animated walk cycle
 * - DASHING: speed lines sprite
 */
void Character_Draw(Character_t* character);

#endif // CHARACTER_H
