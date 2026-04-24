#include "Character.h"

// ===== ANIMATION SPRITES =====

const uint8_t CharacterIDLE[10][10] = {
    {255, 255, 255, 255,255, 255, 255, 255,255,255},
    {255, 0, 0, 255, 255, 255, 255, 0,0,255},
    {255,0,0,0,0,0,0,0,0,255},
    {255,0,255,0,0,0,0,255,0,255},
    {255,0,0,0,0,0,0,0,0,255},
    {255,0,0,255,255,255,255,0,0,255},
    {255,0,0,255,255,255,255,0,0,255},
    {255,0,0,0,0,0,0,0,0,255},
    {255,0,0,255,255,255,255,0,0,255},
    {255,0,0,255,255,255,255,0,0,255}
};

/**
 * @brief WALKING animation frame 1
 * 8x8 pixel sprite
 */
const uint8_t CharacterWALK1[10][10] = {
    {0, 255, 255, 255,255, 255, 0, 0,255,255},
    {0, 0, 0, 0,0, 0, 0, 0,255,255},
    {0, 255, 255, 0,0, 0, 255, 0,255,255},
    {0, 255, 255, 0,0, 0, 0, 0,255,255},
    {0,0,0,0,255,255,255,0,255,255},
    {0,0,0,0,0,255,255,0,255,0},
    {0,0,0,0,0,0,0,0,0},
    {0,0,255,255,255,255,0,0,0,0},
    {0,0,255,255,255,255,0,0,0,0},
    {0,0,0,255,255,255,255,255,255,255}
};

/**
 * @brief WALKING animation frame 2
 * 8x8 pixel sprite
 */
const uint8_t CharacterWALK2[10][10] = {
    {255, 255, 0, 0,255, 255, 255, 255,255,0},
    {255, 255, 0, 0, 0, 0, 0, 0,0,0},
    {255, 255, 0, 0, 0, 0, 255, 0,255,0},
    {255,255,0,0,0,0,0,0,0,0},
    {255,255,0,255,255,255,0,0,0,0},
    {255,255,0,255,255,255,0,0,0,0},
    {0,255,0,0,0,0,0,0,0,0},
    {0,0,0,255,255,255,255,255,0,0},
    {0,0,0,255,255,255,255,255,0,0},
    {255,255,255,255,255,255,255,0,0,0}
};

/**
 * @brief DASHING animation - Speed lines around character
 * 8x8 pixel sprite showing dashing/moving fast
 */
const uint8_t CharacterDASHING[10][10] = {
    {255, 0, 0, 255,255, 255, 255, 255,255,255},
    {255, 0, 0, 255,255, 255, 255, 255,255,255},
    {255,0,0,0,0,0,0,0,0,0},
    {255,0,0,0,0,0,0,255,0,0},
    {255,255,255,0,255,255,0,0,0,255},
    {255,255,255,0,0,255,0,0,0,255},
    {255,255,255,0,0,255,0,0,0,255},
    {0,0,0,0,255,255,0,0,0,255},
    {0,0,0,0,0,0,0,255,0,0},
    {0,0,0,0,0,0,0,0,0,0}
};

// ===== IMPLEMENTATION =====

/**
 * Initialize character at screen center
 */
void Character_Init(Character_t* character) {
    character->x = 120;
    character->y = 120;
    character->state = CHAR_IDLE;
    character->animation_frame = 0;
    character->frame_counter = 0;
    character->dash_counter = 0;
}

/**
 * Update character position and state
 * 
 * Movement: Use joystick->direction for 8-way movement
 * State: IDLE when stopped, WALKING when moving, DASHING on button press
 */
void Character_Update(Character_t* character, Joystick_t* joy, uint8_t dash_pressed) {
    
    // ===== STEP 1: Calculate movement based on joystick direction =====
    int16_t move_x = 0;
    int16_t move_y = 0;
    
    switch (joy->direction) {
        case N:  move_y = -1; break;
        case NE: move_x = 1; move_y = -1; break;
        case E:  move_x = 1; break;
        case SE: move_x = 1; move_y = 1; break;
        case S:  move_y = 1; break;
        case SW: move_x = -1; move_y = 1; break;
        case W:  move_x = -1; break;
        case NW: move_x = -1; move_y = -1; break;
        default: break;  // CENTRE - no movement
    }
    
    // ===== STEP 2: Handle dash button =====
    if (dash_pressed && character->dash_counter == 0) {
        character->dash_counter = CHAR_DASH_DURATION;
    }
    
    // ===== STEP 3: Apply movement with speed (normal or dash) =====
    uint8_t current_speed = CHAR_SPEED;
    if (character->dash_counter > 0) {
        current_speed = CHAR_DASH_SPEED;
        character->dash_counter--;
    }
    
    int16_t new_x = character->x + (move_x * current_speed);
    int16_t new_y = character->y + (move_y * current_speed);
    
    // Keep on screen (sprite is 32x32 after 4x scaling)
    if (new_x < 20) new_x = 20;
    if (new_x > 220) new_x = 220;
    if (new_y < 20) new_y = 20;
    if (new_y > 220) new_y = 220;
    
    if (move_x != 0 || move_y != 0) {
        character->x = new_x;
        character->y = new_y;
    }
    
    // ===== STEP 4: Update state (IDLE, WALKING, DASHING) =====
    uint8_t is_moving = (move_x != 0 || move_y != 0);
    
    if (character->dash_counter > 0) 
    {
        character->state = CHAR_DASHING;
    } 
    else if (is_moving) 
    {
        character->state = CHAR_WALKING;
    } 
    else
    {
        character->state = CHAR_IDLE;
    }
    
    // ===== STEP 5: Update animation frame for walk cycle =====
    if (character->state == CHAR_WALKING) {
        character->frame_counter++;
        if (character->frame_counter >= 10) {
            character->frame_counter = 0;
            character->animation_frame = (character->animation_frame + 1) % 2;
        }
    } else {
        character->animation_frame = 0;
        character->frame_counter = 0;
    }
}

/**
 * Draw character sprite based on current state
 */
void Character_Draw(Character_t* character) {
    
    int16_t x_pos = character->x - 20;  // 10x10 sprite(5) * 4x scale = 20 offset
    int16_t y_pos = character->y - 20;
    
    switch (character->state) {
        case CHAR_IDLE:
            LCD_Draw_Sprite_Colour_Scaled(x_pos, y_pos, 10, 10, (uint8_t*)CharacterIDLE, 3, 4);
            break;
        
        case CHAR_WALKING:
            if (character->animation_frame == 0) 
            {
                LCD_Draw_Sprite_Colour_Scaled(x_pos, y_pos, 10, 10, (uint8_t*)CharacterWALK1, 6, 4);
            }
            else 
            {
                LCD_Draw_Sprite_Colour_Scaled(x_pos, y_pos, 10, 10, (uint8_t*)CharacterWALK2,14, 4);
            }
            break;
        
        case CHAR_DASHING:
            LCD_Draw_Sprite_Colour_Scaled(x_pos, y_pos, 10, 10, (uint8_t*)CharacterDASHING, 2, 4);
            break;
    }
}
