/*
 * Doodle Jump Game for BadgeVMS
 * A gravity-based jumping game where the player controls a character
 * that bounces on platforms while moving upward.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// Game constants
#define WINDOW_WIDTH           576
#define WINDOW_HEIGHT          432
#define PLAYER_WIDTH           20
#define PLAYER_HEIGHT          20
#define PLATFORM_WIDTH         80
#define PLATFORM_HEIGHT        15
#define MAX_PLATFORMS          100
#define PLATFORM_SPACING_MIN   70
#define PLATFORM_SPACING_MAX   110
#define GRAVITY                0.8f
#define JUMP_FORCE            -18.0f
#define SPRING_JUMP_FORCE     -22.0f
#define PLAYER_SPEED           6.0f
#define PLAYER_ACCELERATION    0.3f
#define PLAYER_FRICTION        0.85f

// Platform types
typedef enum {
    PLATFORM_NORMAL = 0,
    PLATFORM_MOVING,
    PLATFORM_BREAKABLE,
    PLATFORM_SPRING
} PlatformType;

// Platform structure
typedef struct {
    float x, y;
    float width, height;
    PlatformType type;
    bool active;
    float move_direction; // For moving platforms
    float move_speed; // Speed for moving platforms
} Platform;

// Player structure
typedef struct {
    float x, y;
    float vx, vy; // velocity
    float width, height;
    bool on_ground;
} Player;

// Game state
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Player player;
    Platform platforms[MAX_PLATFORMS];
    int num_platforms;
    float camera_y; // Camera position for scrolling
    int score;
    int platforms_landed; // Count of platforms landed on
    int last_platform_landed; // Index of last platform landed on to avoid double counting
    bool game_running;
    Uint64 last_time;
} GameState;

// Function declarations
void init_game(GameState *game);
void generate_platforms(GameState *game);
void generate_new_platforms(GameState *game);
void update_game(GameState *game, float delta_time);
void render_game(GameState *game);
void handle_input(GameState *game, const bool *keyboard_state, float delta_time);
void update_camera(GameState *game);
bool check_platform_collision(Player *player, Platform *platform);
void render_number(SDL_Renderer *renderer, int number, float x, float y, float scale);

// Initialize the game state
void init_game(GameState *game) {
    // Initialize player - position above starting platform
    game->player.x = WINDOW_WIDTH / 2 - PLAYER_WIDTH / 2;
    game->player.y = WINDOW_HEIGHT - PLATFORM_HEIGHT - 50 - PLAYER_HEIGHT - 5; // Above starting platform
    game->player.vx = 0;
    game->player.vy = 0;
    game->player.width = PLAYER_WIDTH;
    game->player.height = PLAYER_HEIGHT;
    game->player.on_ground = false;
    
    // Initialize camera
    game->camera_y = 0;
    game->score = 0;
    game->platforms_landed = 0;
    game->last_platform_landed = -1;
    game->game_running = true;
    
    // Generate initial platforms
    generate_platforms(game);
}

// Generate platforms for the game
void generate_platforms(GameState *game) {
    game->num_platforms = 0;
    
    // Add a starting platform at the bottom - always clearly visible with good padding
    Platform *start_platform = &game->platforms[game->num_platforms++];
    start_platform->x = WINDOW_WIDTH / 2 - PLATFORM_WIDTH / 2;
    start_platform->y = WINDOW_HEIGHT - PLATFORM_HEIGHT - 50; // 50px offset from bottom for better visibility
    start_platform->width = PLATFORM_WIDTH;
    start_platform->height = PLATFORM_HEIGHT;
    start_platform->type = PLATFORM_NORMAL;
    start_platform->active = true;
    start_platform->move_direction = 0;
    start_platform->move_speed = 0;
    
    // Generate platforms going upward
    float current_y = start_platform->y - PLATFORM_SPACING_MIN;
    
    for (int i = 1; i < MAX_PLATFORMS && current_y > -1000; i++) {
        Platform *platform = &game->platforms[game->num_platforms++];
        
        // Random horizontal position
        platform->x = (float)(SDL_rand(WINDOW_WIDTH - (int)PLATFORM_WIDTH));
        platform->y = current_y;
        platform->width = PLATFORM_WIDTH;
        platform->height = PLATFORM_HEIGHT;
        
        // Randomly assign platform types (85% normal, 5% moving, 5% breakable, 5% spring)
        int rand_type = SDL_rand(100);
        if (rand_type < 85) {
            platform->type = PLATFORM_NORMAL;
            platform->move_direction = 0;
            platform->move_speed = 0;
        } else if (rand_type < 90) {
            platform->type = PLATFORM_MOVING;
            platform->move_direction = (SDL_rand(2) == 0) ? 1.0f : -1.0f; // Random direction
            // Random speed between 0.5x and 1.25x (reduced by factor of 4)
            platform->move_speed = 0.5f + (float)(SDL_rand(15)) / 20.0f; // 0.5f to 1.25f
        } else if (rand_type < 95) {
            platform->type = PLATFORM_BREAKABLE;
            platform->move_direction = 0;
            platform->move_speed = 0;
        } else {
            platform->type = PLATFORM_SPRING;
            platform->move_direction = 0;
            platform->move_speed = 0;
        }
        
        platform->active = true;
        
        // Move to next platform position with difficulty scaling
        // As we go higher (more negative Y), increase spacing between platforms
        float height_factor = (-current_y) / 500.0f; // Difficulty increases every 500 units
        int additional_spacing = (int)(height_factor * 15); // Reduced from 20 to 15 for easier jumps
        
        // Clamp the additional spacing to prevent impossible gaps
        if (additional_spacing > 30) additional_spacing = 30; // Reduced from 40 to 30
        
        current_y -= PLATFORM_SPACING_MIN + (SDL_rand(PLATFORM_SPACING_MAX - PLATFORM_SPACING_MIN)) + additional_spacing;
    }
}

// Generate new platforms as the player moves upward (infinite generation)
void generate_new_platforms(GameState *game) {
    // Find the highest platform
    float highest_y = game->camera_y; // Start with camera position
    for (int i = 0; i < game->num_platforms; i++) {
        if (game->platforms[i].active && game->platforms[i].y < highest_y) {
            highest_y = game->platforms[i].y;
        }
    }
    
    // Generate new platforms if we need more above the highest point
    float target_height = game->camera_y - WINDOW_HEIGHT - 200; // Generate well above camera
    float current_y = highest_y - PLATFORM_SPACING_MIN;
    
    while (current_y > target_height && game->num_platforms < MAX_PLATFORMS) {
        // Find an inactive platform slot to reuse
        int slot = -1;
        for (int i = 0; i < game->num_platforms; i++) {
            if (!game->platforms[i].active) {
                slot = i;
                break;
            }
        }
        
        // If no inactive slot, try to add a new one
        if (slot == -1 && game->num_platforms < MAX_PLATFORMS) {
            slot = game->num_platforms++;
        }
        
        // If we found a slot, create a new platform
        if (slot != -1) {
            Platform *platform = &game->platforms[slot];
            
            // Random horizontal position
            platform->x = (float)(SDL_rand(WINDOW_WIDTH - (int)PLATFORM_WIDTH));
            platform->y = current_y;
            platform->width = PLATFORM_WIDTH;
            platform->height = PLATFORM_HEIGHT;
            
            // Randomly assign platform types (85% normal, 5% moving, 5% breakable, 5% spring)
            int rand_type = SDL_rand(100);
            if (rand_type < 85) {
                platform->type = PLATFORM_NORMAL;
                platform->move_direction = 0;
                platform->move_speed = 0;
            } else if (rand_type < 90) {
                platform->type = PLATFORM_MOVING;
                platform->move_direction = (SDL_rand(2) == 0) ? 1.0f : -1.0f; // Random direction
                // Random speed between 0.5x and 1.25x, factoring in score (reduced by factor of 4)
                float base_speed = 0.5f + (float)(SDL_rand(15)) / 20.0f; // 0.5f to 1.25f
                float score_multiplier = 1.0f + (float)game->score / 200.0f; // Gradual increase
                platform->move_speed = base_speed * score_multiplier;
            } else if (rand_type < 95) {
                platform->type = PLATFORM_BREAKABLE;
                platform->move_direction = 0;
                platform->move_speed = 0;
            } else {
                platform->type = PLATFORM_SPRING;
                platform->move_direction = 0;
                platform->move_speed = 0;
            }
            
            platform->active = true;
            
            // Move to next platform position with difficulty scaling
            // As we go higher (more negative Y), increase spacing between platforms
            float height_factor = (-current_y) / 500.0f; // Difficulty increases every 500 units
            int additional_spacing = (int)(height_factor * 15); // Reduced from 20 to 15 for easier jumps
            
            // Clamp the additional spacing to prevent impossible gaps
            if (additional_spacing > 30) additional_spacing = 30; // Reduced from 40 to 30
            
            current_y -= PLATFORM_SPACING_MIN + (SDL_rand(PLATFORM_SPACING_MAX - PLATFORM_SPACING_MIN)) + additional_spacing;
        } else {
            // No more slots available, break out
            break;
        }
    }
}

// Check collision between player and platform
bool check_platform_collision(Player *player, Platform *platform) {
    if (!platform->active) return false;
    
    // Check if player is falling down (positive velocity)
    if (player->vy <= 0) return false;
    
    // Basic AABB collision detection
    if (player->x < platform->x + platform->width &&
        player->x + player->width > platform->x &&
        player->y < platform->y + platform->height &&
        player->y + player->height > platform->y) {
        
        // Make sure player is above the platform (landing on top)
        if (player->y < platform->y) {
            return true;
        }
    }
    
    return false;
}

// Render a number using simple pixel patterns
void render_number(SDL_Renderer *renderer, int number, float x, float y, float scale) {
    // Define digit patterns as 3x5 bitmaps
    static const unsigned char digit_patterns[10][5] = {
        // 0
        {0b111, 0b101, 0b101, 0b101, 0b111},
        // 1
        {0b010, 0b110, 0b010, 0b010, 0b111},
        // 2
        {0b111, 0b001, 0b111, 0b100, 0b111},
        // 3
        {0b111, 0b001, 0b111, 0b001, 0b111},
        // 4
        {0b101, 0b101, 0b111, 0b001, 0b001},
        // 5
        {0b111, 0b100, 0b111, 0b001, 0b111},
        // 6
        {0b111, 0b100, 0b111, 0b101, 0b111},
        // 7
        {0b111, 0b001, 0b001, 0b001, 0b001},
        // 8
        {0b111, 0b101, 0b111, 0b101, 0b111},
        // 9
        {0b111, 0b101, 0b111, 0b001, 0b111}
    };
    
    if (number == 0) {
        // Special case for 0
        const unsigned char *pattern = digit_patterns[0];
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                if (pattern[row] & (1 << (2 - col))) {
                    SDL_FRect pixel = {
                        x + col * scale,
                        y + row * scale,
                        scale,
                        scale
                    };
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        return;
    }
    
    // Convert number to string to render digits
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", number);
    
    float current_x = x;
    for (int i = 0; buffer[i] != '\0'; i++) {
        int digit = buffer[i] - '0';
        if (digit >= 0 && digit <= 9) {
            const unsigned char *pattern = digit_patterns[digit];
            
            // Render the digit
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 3; col++) {
                    if (pattern[row] & (1 << (2 - col))) {
                        SDL_FRect pixel = {
                            current_x + col * scale,
                            y + row * scale,
                            scale,
                            scale
                        };
                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
            }
        }
        
        // Move to next digit position (3 pixels + 1 space)
        current_x += 4 * scale;
    }
}

// Update camera to follow the player
void update_camera(GameState *game) {
    // Keep camera positioned to show platforms below player
    // Increased offset to keep more platforms visible below
    float camera_offset = 50.0f + PLATFORM_HEIGHT; // Increased from 20.0f to 50.0f
    float target_camera_y = game->player.y - WINDOW_HEIGHT / 2 + camera_offset;
    
    // Only move camera up (never down)
    if (target_camera_y < game->camera_y) {
        game->camera_y = target_camera_y;
    }
}

// Update game logic
void update_game(GameState *game, float delta_time) {
    if (!game->game_running) return;
    
    Player *player = &game->player;
    
    // Apply gravity
    player->vy += GRAVITY * delta_time;
    
    // Update player position
    player->x += player->vx * delta_time;
    player->y += player->vy * delta_time;
    
    // Keep player within screen bounds horizontally (wrap around)
    if (player->x + player->width < 0) {
        player->x = WINDOW_WIDTH;
    } else if (player->x > WINDOW_WIDTH) {
        player->x = -player->width;
    }
    
    // Update platforms
    for (int i = 0; i < game->num_platforms; i++) {
        Platform *platform = &game->platforms[i];
        if (!platform->active) continue;
        
        // Update moving platforms
        if (platform->type == PLATFORM_MOVING) {
            // Factor in score for increased speed (1.0 + score/100 gives gradual speed increase)
            float score_multiplier = 1.0f + (float)game->score / 100.0f;
            float actual_speed = platform->move_speed * score_multiplier;
            
            platform->x += platform->move_direction * actual_speed * delta_time;
            
            // Bounce off screen edges with proper bounds checking
            if (platform->x <= 0) {
                platform->x = 0;
                platform->move_direction = 1.0f; // Move right
            } else if (platform->x + platform->width >= WINDOW_WIDTH) {
                platform->x = WINDOW_WIDTH - platform->width;
                platform->move_direction = -1.0f; // Move left
            }
        }
    }
    
    // Deactivate platforms that are too far below the camera (off-screen cleanup)
    for (int i = 0; i < game->num_platforms; i++) {
        Platform *platform = &game->platforms[i];
        if (platform->active && platform->y > game->camera_y + WINDOW_HEIGHT + 200) {
            platform->active = false; // Deactivate platforms far below screen
        }
    }
    
    // Generate new platforms as needed for infinite gameplay
    generate_new_platforms(game);
    
    // Check platform collisions
    player->on_ground = false;
    for (int i = 0; i < game->num_platforms; i++) {
        if (check_platform_collision(player, &game->platforms[i])) {
            Platform *platform = &game->platforms[i];
            
            player->y = platform->y - player->height;
            
            // Different jump forces for different platform types
            float jump_force = JUMP_FORCE;
            if (platform->type == PLATFORM_MOVING) {
                jump_force = JUMP_FORCE * 1.1f; // Slightly higher jump for moving platforms
            } else if (platform->type == PLATFORM_SPRING) {
                jump_force = SPRING_JUMP_FORCE; // Much higher jump for spring platforms
            }
            
            player->vy = jump_force;
            player->on_ground = true;
            
            // Handle breakable platforms
            if (platform->type == PLATFORM_BREAKABLE) {
                platform->active = false; // Platform breaks after being used
            }
            
            // Increment platforms landed counter if this is a new platform
            if (i != game->last_platform_landed) {
                game->platforms_landed++;
                game->last_platform_landed = i;
            }
            break;
        }
    }
    
    // Update camera
    update_camera(game);
    
    // Update score based on height (platforms passed)
    int new_score = (int)(-game->camera_y / 10);
    if (new_score > game->score) {
        game->score = new_score;
    }
    
    // Game over if player falls completely off screen (whole body exits screen)
    if (player->y > game->camera_y + WINDOW_HEIGHT) {
        game->game_running = false;
    }
}

// Handle player input
void handle_input(GameState *game, const bool *keyboard_state, float delta_time) {
    Player *player = &game->player;
    
    // Horizontal movement with acceleration
    float target_vx = 0;
    
    // Left/Right movement
    if (keyboard_state[SDL_SCANCODE_LEFT] || keyboard_state[SDL_SCANCODE_A]) {
        target_vx = -PLAYER_SPEED;
    }
    if (keyboard_state[SDL_SCANCODE_RIGHT] || keyboard_state[SDL_SCANCODE_D]) {
        target_vx = PLAYER_SPEED;
    }
    
    // Apply acceleration towards target velocity
    if (target_vx != 0) {
        player->vx += (target_vx - player->vx) * PLAYER_ACCELERATION * delta_time;
    } else {
        // Apply friction when no input
        player->vx *= PLAYER_FRICTION;
    }
    
    // Restart game
    if (keyboard_state[SDL_SCANCODE_R] && !game->game_running) {
        init_game(game);
    }
}

// Render the game
void render_game(GameState *game) {
    // Clear screen with sky blue color
    SDL_SetRenderDrawColor(game->renderer, 135, 206, 235, 255);
    SDL_RenderClear(game->renderer);
    
    // Render platforms
    for (int i = 0; i < game->num_platforms; i++) {
        Platform *platform = &game->platforms[i];
        if (!platform->active) continue;
        
        // Set color based on platform type
        switch (platform->type) {
            case PLATFORM_NORMAL:
                SDL_SetRenderDrawColor(game->renderer, 34, 139, 34, 255); // Forest green
                break;
            case PLATFORM_MOVING:
                SDL_SetRenderDrawColor(game->renderer, 255, 165, 0, 255); // Orange
                break;
            case PLATFORM_BREAKABLE:
                SDL_SetRenderDrawColor(game->renderer, 139, 69, 19, 255); // Brown
                break;
            case PLATFORM_SPRING:
                SDL_SetRenderDrawColor(game->renderer, 255, 20, 147, 255); // Deep pink
                break;
        }
        
        // Convert world coordinates to screen coordinates
        SDL_FRect rect = {
            platform->x,
            platform->y - game->camera_y,
            platform->width,
            platform->height
        };
        
        // Only render if platform is visible on screen
        if (rect.y > -platform->height && rect.y < WINDOW_HEIGHT + platform->height) {
            SDL_RenderFillRect(game->renderer, &rect);
        }
    }
    
    // Render player
    SDL_SetRenderDrawColor(game->renderer, 255, 100, 100, 255); // Light red
    SDL_FRect player_rect = {
        game->player.x,
        game->player.y - game->camera_y,
        game->player.width,
        game->player.height
    };
    SDL_RenderFillRect(game->renderer, &player_rect);
    
    // Render score - display platforms landed count at top of screen
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
    
    // Simple "SCORE:" text using rectangles
    // S
    SDL_FRect s_rects[] = {
        {10, 10, 8, 2}, {10, 10, 2, 4}, {10, 14, 6, 2}, {16, 14, 2, 4}, {10, 18, 8, 2}
    };
    for (int i = 0; i < 5; i++) {
        SDL_RenderFillRect(game->renderer, &s_rects[i]);
    }
    
    // C
    SDL_FRect c_rects[] = {
        {22, 10, 8, 2}, {22, 10, 2, 10}, {22, 18, 8, 2}
    };
    for (int i = 0; i < 3; i++) {
        SDL_RenderFillRect(game->renderer, &c_rects[i]);
    }
    
    // O
    SDL_FRect o_rects[] = {
        {34, 10, 8, 2}, {34, 10, 2, 10}, {40, 10, 2, 10}, {34, 18, 8, 2}
    };
    for (int i = 0; i < 4; i++) {
        SDL_RenderFillRect(game->renderer, &o_rects[i]);
    }
    
    // R
    SDL_FRect r_rects[] = {
        {46, 10, 8, 2}, {46, 10, 2, 10}, {52, 10, 2, 4}, {46, 14, 6, 2}, {50, 14, 4, 6}
    };
    for (int i = 0; i < 5; i++) {
        SDL_RenderFillRect(game->renderer, &r_rects[i]);
    }
    
    // E
    SDL_FRect e_rects[] = {
        {58, 10, 8, 2}, {58, 10, 2, 10}, {58, 14, 6, 2}, {58, 18, 8, 2}
    };
    for (int i = 0; i < 4; i++) {
        SDL_RenderFillRect(game->renderer, &e_rects[i]);
    }
    
    // :
    SDL_FRect colon_rects[] = {
        {70, 13, 2, 2}, {70, 17, 2, 2}
    };
    for (int i = 0; i < 2; i++) {
        SDL_RenderFillRect(game->renderer, &colon_rects[i]);
    }
    
    // Render the actual score using our render_number function
    render_number(game->renderer, game->score, 80, 10, 3.0f);
    
    // Game over screen with big banner
    if (!game->game_running) {
        // Semi-transparent background overlay
        SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 200);
        SDL_FRect overlay = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(game->renderer, &overlay);
        
        // Large "GAME OVER" banner using rectangles
        SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
        
        int char_size = 16; // Much bigger characters (increased from 8)
        int banner_y = WINDOW_HEIGHT / 2 - char_size * 2; // Center vertically
        
        // Calculate total width to center the text
        int total_width = 8 * char_size * 4; // Approximate total width for "GAME OVER"
        int start_x = (WINDOW_WIDTH - total_width) / 2;
        
        // G
        SDL_FRect g_rects[] = {
            {start_x, banner_y, char_size * 3, char_size}, // Top
            {start_x, banner_y, char_size, char_size * 4}, // Left side
            {start_x, banner_y + char_size * 3, char_size * 3, char_size}, // Bottom
            {start_x + char_size * 2, banner_y + char_size, char_size, char_size * 2}, // Right side middle
            {start_x + char_size, banner_y + char_size * 2, char_size, char_size} // Middle bar
        };
        for (int i = 0; i < 5; i++) {
            SDL_RenderFillRect(game->renderer, &g_rects[i]);
        }
        start_x += char_size * 4;
        
        // A
        SDL_FRect a_rects[] = {
            {start_x, banner_y, char_size * 3, char_size}, // Top
            {start_x, banner_y, char_size, char_size * 4}, // Left side
            {start_x + char_size * 2, banner_y, char_size, char_size * 4}, // Right side
            {start_x, banner_y + char_size * 2, char_size * 3, char_size} // Middle bar
        };
        for (int i = 0; i < 4; i++) {
            SDL_RenderFillRect(game->renderer, &a_rects[i]);
        }
        start_x += char_size * 4;
        
        // M
        SDL_FRect m_rects[] = {
            {start_x, banner_y, char_size, char_size * 4}, // Left side
            {start_x + char_size * 2, banner_y, char_size, char_size * 4}, // Right side
            {start_x + char_size, banner_y, char_size, char_size * 2} // Middle
        };
        for (int i = 0; i < 3; i++) {
            SDL_RenderFillRect(game->renderer, &m_rects[i]);
        }
        start_x += char_size * 4;
        
        // E
        SDL_FRect e_rects[] = {
            {start_x, banner_y, char_size * 3, char_size}, // Top
            {start_x, banner_y, char_size, char_size * 4}, // Left side
            {start_x, banner_y + char_size * 2, char_size * 2, char_size}, // Middle
            {start_x, banner_y + char_size * 3, char_size * 3, char_size} // Bottom
        };
        for (int i = 0; i < 4; i++) {
            SDL_RenderFillRect(game->renderer, &e_rects[i]);
        }
        start_x += char_size * 5;
        
        // O
        SDL_FRect o_rects[] = {
            {start_x, banner_y, char_size * 3, char_size}, // Top
            {start_x, banner_y, char_size, char_size * 4}, // Left side
            {start_x + char_size * 2, banner_y, char_size, char_size * 4}, // Right side
            {start_x, banner_y + char_size * 3, char_size * 3, char_size} // Bottom
        };
        for (int i = 0; i < 4; i++) {
            SDL_RenderFillRect(game->renderer, &o_rects[i]);
        }
        start_x += char_size * 4;
        
        // V
        SDL_FRect v_rects[] = {
            {start_x, banner_y, char_size, char_size * 3}, // Left side
            {start_x + char_size * 2, banner_y, char_size, char_size * 3}, // Right side
            {start_x + char_size, banner_y + char_size * 3, char_size, char_size} // Bottom point
        };
        for (int i = 0; i < 3; i++) {
            SDL_RenderFillRect(game->renderer, &v_rects[i]);
        }
        start_x += char_size * 4;
        
        // E
        SDL_FRect e2_rects[] = {
            {start_x, banner_y, char_size * 3, char_size}, // Top
            {start_x, banner_y, char_size, char_size * 4}, // Left side
            {start_x, banner_y + char_size * 2, char_size * 2, char_size}, // Middle
            {start_x, banner_y + char_size * 3, char_size * 3, char_size} // Bottom
        };
        for (int i = 0; i < 4; i++) {
            SDL_RenderFillRect(game->renderer, &e2_rects[i]);
        }
        start_x += char_size * 4;
        
        // R
        SDL_FRect r_rects[] = {
            {start_x, banner_y, char_size * 3, char_size}, // Top
            {start_x, banner_y, char_size, char_size * 4}, // Left side
            {start_x + char_size * 2, banner_y, char_size, char_size * 2}, // Right side top
            {start_x, banner_y + char_size * 2, char_size * 2, char_size}, // Middle
            {start_x + char_size, banner_y + char_size * 2, char_size * 2, char_size * 2} // Bottom diagonal
        };
        for (int i = 0; i < 5; i++) {
            SDL_RenderFillRect(game->renderer, &r_rects[i]);
        }
        
        // Score display below banner
        SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
        render_number(game->renderer, game->score, WINDOW_WIDTH / 2 - 30, WINDOW_HEIGHT / 2 + 60, 5.0f);
        
        // Restart hint - "PRESS R" 
        SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255); // Yellow
        int hint_y = WINDOW_HEIGHT / 2 + 120;
        int hint_size = 6;
        start_x = WINDOW_WIDTH / 2 - 12; // Center the R
        
        // Simple "R" for restart hint
        SDL_FRect hint_r_rects[] = {
            {start_x, hint_y, hint_size * 2, hint_size}, // Top
            {start_x, hint_y, hint_size, hint_size * 3}, // Left side
            {start_x + hint_size, hint_y + hint_size, hint_size, hint_size}, // Middle
            {start_x + hint_size, hint_y + hint_size * 2, hint_size * 2, hint_size} // Bottom diagonal
        };
        for (int i = 0; i < 4; i++) {
            SDL_RenderFillRect(game->renderer, &hint_r_rects[i]);
        }
    }
    
    SDL_RenderPresent(game->renderer);
}

// SDL Callback functions
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    GameState *game = (GameState *)SDL_calloc(1, sizeof(GameState));
    if (!game) {
        return SDL_APP_FAILURE;
    }
    
    *appstate = game;
    
    // Create window
    game->window = SDL_CreateWindow("Doodle Jump - BadgeVMS", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!game->window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // Create renderer
    game->renderer = SDL_CreateRenderer(game->window, NULL);
    if (!game->renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // Initialize game
    init_game(game);
    game->last_time = SDL_GetTicks();
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    GameState *game = (GameState *)appstate;
    
    // Calculate delta time
    Uint64 current_time = SDL_GetTicks();
    float delta_time = (float)(current_time - game->last_time) / 16.0f; // Normalize to ~60 FPS
    game->last_time = current_time;
    
    // Handle input
    const bool *keyboard_state = SDL_GetKeyboardState(NULL);
    handle_input(game, keyboard_state, delta_time);
    
    // Update game
    update_game(game, delta_time);
    
    // Render game
    render_game(game);
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN:
            if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
                return SDL_APP_SUCCESS;
            }
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if (appstate != NULL) {
        GameState *game = (GameState *)appstate;
        SDL_DestroyRenderer(game->renderer);
        SDL_DestroyWindow(game->window);
        SDL_free(game);
    }
}
