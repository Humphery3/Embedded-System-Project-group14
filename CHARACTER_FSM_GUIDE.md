# Character FSM Demo - Educational Guide

## Overview

This demonstrates **Finite State Machines (FSMs)** for game objects - a fundamental pattern in game development and embedded interactive systems.

The demo features a Mario-like character with an internal FSM that controls:
- **Character FSM**: Internal state machine (IDLE, RUNNING, JUMPING)
- **Update/Render Separation**: Clean architecture separating game logic from drawing
- **Physics Integration**: Gravity, velocity, and collision detection

## Key Concepts Demonstrated

### 1. **State Machine Concept**
An FSM has:
- **States**: Different modes of operation (IDLE, RUNNING, JUMPING)
- **Transitions**: Rules for switching between states
- **Inputs**: External events triggering transitions (joystick, buttons, physics)
- **Outputs**: Behaviors specific to each state (different animations)

### 2. **Object-Oriented FSMs**
Each game object can have its own internal FSM:
```
Main Game Loop (30ms per frame)
    │
    ├─ Read Input (joystick, buttons)
    │
    ├─ Update Character FSM
    │   ├─ Process input
    │   ├─ Apply physics (gravity, velocity)
    │   ├─ Check collisions
    │   ├─ Update state (IDLE/RUNNING/JUMPING)
    │   └─ Update animation frame
    │
    └─ Render Game
        ├─ Clear screen
        ├─ Draw environment (ground)
        ├─ Draw character sprite
        └─ Draw debug info
```

This is how professional games work - each entity has its own state machine!

### 3. **Physics Integration**
The character demonstrates:
- **Gravity**: Applied each frame during jump/fall (0.6 pixels/frame²)
- **State-Based Animation**: Different sprites for different states
- **Ground Collision**: Automatic state transition when landing
- **Velocity Management**: Tracking vertical motion with terminal velocity

### 4. **Update/Render Separation**
Clean architecture pattern:
- **update_character()**: All game logic, no drawing
- **render_game()**: All drawing, no logic
- Makes code easier to understand, test, and extend

## Character States

### IDLE State
- **Trigger**: Character is on ground + joystick centered
- **Animation**: Standing still sprite
- **Transitions**:
  - → RUNNING: Joystick X moved
  - → JUMPING: Jump button pressed

### RUNNING State
- **Trigger**: Character moving horizontally on ground
- **Animation**: Alternating leg animation (2 frames)
- **Transitions**:
  - → IDLE: Joystick X centered
  - → JUMPING: Jump button pressed

### JUMPING State
- **Trigger**: Jump button pressed while on ground
- **Animation**: Looking up sprite with extended legs
- **Behavior**:
  - Initial velocity: CHAR_JUMP_VELOCITY (-10.0 pixels/frame)
  - Gravity applied: CHAR_GRAVITY (1.1 pixels/frame²)
  - Can still move left/right while in air
  - Terminal velocity: 15.0 pixels/frame (max fall speed)
- **Transitions**:
  - → IDLE or RUNNING: When character lands (y_position >= GROUND_Y)

## Input Control

| Input | Action |
|-------|--------|
| **Joystick X-axis** | Move character left/right (4 pixels/frame) |
| **BTN2** | Jump (only when on ground) |

## Technical Implementation

### File Structure
```
Character/
├── Character.h          # FSM state definitions, structures, function prototypes
└── Character.c          # Implementation with sprites, state machine logic, and physics

Core/Src/main.c          # Main game loop with update_character() and render_game()
```

### Character Structure
```c
typedef struct {
    int16_t x, y;                   // Position
    float velocity_y;               // Vertical velocity (for physics)
    CharacterState_t state;         // Current internal FSM state
    uint8_t animation_frame;        // Animation frame counter
    int8_t direction;               // -1=left, 0=idle, 1=right
} Character_t;
```

### State Machine Implementation
The character's FSM runs every frame with separate update and render phases:

**update_character() - Game Logic:**
1. **Read Input**: Get joystick and button states
2. **Horizontal Movement**: Move character based on joystick X-axis
3. **Jump Handling**: Set upward velocity when jump button pressed (if on ground)
4. **Apply Physics**: Apply gravity, update vertical position, check collisions
5. **Update FSM State**: Transition based on current state + input + physics
6. **Update Animation**: Increment frame counter, cycle animation frames

**render_game() - Drawing:**
1. **Clear Screen**: Fill buffer with black
2. **Draw Environment**: Ground line and platforms
3. **Draw Character**: Select and draw sprite based on current state
4. **Draw Debug Info**: Display state, position, velocity
5. **Refresh LCD**: Push buffer to screen

### Physics Constants
```c
#define CHAR_SPEED 4                    // Horizontal pixels per frame
#define CHAR_GRAVITY 1.1f               // Gravity acceleration
#define CHAR_JUMP_VELOCITY -10.0f       // Initial jump velocity (upward)
#define CHAR_MAX_FALL_VELOCITY 15.0f    // Terminal velocity
#define GROUND_Y 200                    // Ground position on screen
#define CHAR_WIDTH 32                   // Character width (8x8 sprite scaled 4x)
#define CHAR_HEIGHT 32                  // Character height (8x8 sprite scaled 4x)
```

## Educational Value

### Why Object-Oriented FSMs?
1. **Scalability**: Add more game objects (enemies, items) without main loop getting complex
2. **Reusability**: Character code is independent and can be instantiated multiple times
3. **Maintainability**: State logic is clear and organized within the Character module
4. **Extensibility**: Easy to add new states or animations without affecting other code
5. **Clean Architecture**: Update/Render separation follows game engine best practices

### Example: Adding a New State
To add a "FALLING" state between JUMPING and landing:

1. Add to enum in Character.h:
```c
typedef enum {
    CHAR_IDLE = 0,
    CHAR_RUNNING,
    CHAR_JUMPING,
    CHAR_FALLING,    // NEW
    CHAR_LANDING
} CharacterState_t;
```

2. Add state case in Character_Update() switch statement
3. Add drawing code in Character_Draw() for new animation
4. That's it! Rest of game is unaffected.

## Running the Demo

1. Build and flash the code to STM32L476
2. Character starts in IDLE state at ground level
3. Controls:
   - **Move joystick left/right**: Character runs and enters RUNNING state
   - **Press BTN2**: Character jumps (enters JUMPING state)
   - **Release joystick**: Character returns to IDLE when landing
4. Watch the LCD display:
   - Character sprite changes based on state
   - Debug info shows: State, Y position, vertical velocity
   - Character animates while running (alternating leg sprites)

## Advanced Enhancements

### Suggested Additions
- [ ] Add FALLING state with different sprite
- [ ] Add wall collision detection
- [ ] Add double jump mechanic (jump while falling)
- [ ] Add facing direction sprite flipping
- [ ] Add more animation frames for smoother running
- [ ] Add dash/sprint ability (new state)
- [ ] Add multiple characters with separate FSMs
- [ ] Add enemy AI with its own FSM

### Physics Improvements
- Implement air acceleration (harder to change direction mid-jump)
- Add friction/damping for more realistic movement
- Add velocity integration instead of simple addition
- Add coyote time (brief grace period for jumping after leaving ground)

## Debugging

The LCD displays real-time debug information:
- **State**: Current character FSM state (IDLE/RUNNING/JUMPING)
- **Y**: Vertical position (200 = ground, <200 = in air)
- **VY**: Vertical velocity (negative = going up, positive = falling)

The debug display is in `render_game()` and updates every frame (30ms).

You can add more debug info by modifying `render_game()`:
```c
// Example: Display horizontal position
char x_str[16];
sprintf(x_str, "X:%d", game_character.x);
LCD_printString(x_str, 180, 90, 1, 2);
```

## Key Code Locations

- **Main game loop**: [Core/Src/main.c](Core/Src/main.c) - update_character() and render_game()
- **Character FSM update**: [Character/Character.c](Character/Character.c) - Character_Update()
- **Character rendering**: [Character/Character.c](Character/Character.c) - Character_Draw()
- **Character sprites**: [Character/Character.c](Character/Character.c) - CharacterIDLE, CharacterRUN1/2, CharacterJUMP
- **Jump button interrupt**: [Core/Src/main.c](Core/Src/main.c) - HAL_GPIO_EXTI_Callback()
- **Physics constants**: [Character/Character.h](Character/Character.h) - CHAR_SPEED, CHAR_GRAVITY, etc.

## Summary

This demo teaches:
✓ How FSMs work in embedded systems and games
✓ How to implement state-based animation
✓ How to integrate physics (gravity, velocity) with state machines
✓ How to use object-oriented FSMs for game entities
✓ How to handle input with interrupts and debouncing
✓ How to separate game logic (update) from rendering (draw)
✓ How to build interactive LCD graphics systems with sprites

**Key Takeaway**: This pattern scales to complex games - each enemy, NPC, collectible, power-up, etc. can have its own FSM! The update/render separation keeps code clean and maintainable as systems grow.
