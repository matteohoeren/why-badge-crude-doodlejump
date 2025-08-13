/*
 * Doodle Jump Game for BadgeVMS
 * A gravity-based jumping game where the player controls a character
 * that bounces on platforms while moving upward.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// Image loading support - declaration only
// Implementation is in image.c to avoid TLS issues
unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
void stbi_image_free(void *retval_from_stbi_load);
const char *stbi_failure_reason(void);

// Include BadgeVMS device support for BMI270 (only when building for badge hardware)
#ifdef BADGEVMS_BUILD
#include "badgevms/device.h"
#endif

// Memory optimization flag
#define USE_IMAGES 0  // Set to 0 to use colored rectangles instead of images

// Game constants
#define WINDOW_WIDTH           720  // Full screen width for BadgeVMS
#define WINDOW_HEIGHT          720  // Full screen height for BadgeVMS
#define PLAYER_WIDTH           40
#define PLAYER_HEIGHT          40
#define PLATFORM_WIDTH         65
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
#define MAX_PROJECTILES        10
#define PROJECTILE_SPEED       -7.0f
#define PROJECTILE_WIDTH       8
#define PROJECTILE_HEIGHT      16

// Monster constants
#define MAX_MONSTERS           20
#define MONSTER_WIDTH          70
#define MONSTER_HEIGHT         90
#define MONSTER_SPAWN_SCORE_MIN 100
#define MONSTER_SPAWN_SCORE_MAX 1500
#define MONSTER_MAX_SCORE      5000
#define MONSTER_SPEED          0.03f

// Platform types
typedef enum {
    PLATFORM_NORMAL = 0,
    PLATFORM_MOVING,
    PLATFORM_BREAKABLE,
    PLATFORM_SPRING
} PlatformType;

// Projectile structure
typedef struct {
    float x, y;
    float vy; // Only vertical velocity needed
    bool active;
} Projectile;

// Monster types
typedef enum {
    MONSTER_BASIC = 0
} MonsterType;

// Monster structure
typedef struct {
    float x, y;
    float vx; // Horizontal velocity
    float width, height;
    MonsterType type;
    bool active;
    float move_direction; // -1 for left, 1 for right
} Monster;

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
    int facing_direction; // -1 for left, 1 for right
    bool is_shooting; // Animation state for shooting
    float shoot_timer; // Timer for shooting animation
} Player;

// Game state
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Player player;
    Platform platforms[MAX_PLATFORMS];
    int num_platforms;
    Projectile projectiles[MAX_PROJECTILES];
    int num_projectiles;
    Monster monsters[MAX_MONSTERS];
    int num_monsters;
    float camera_y; // Camera position for scrolling
    int score;
    int platforms_landed; // Count of platforms landed on
    int last_platform_landed; // Index of last platform landed on to avoid double counting
    bool game_running;
    Uint32 last_time;  // Changed from Uint64 to avoid 64-bit float conversion issues
    
    // Input state for event-based controls
    bool left_pressed;
    bool right_pressed;
    bool shoot_pressed;
    bool restart_pressed;
    
    // Textures for graphics
    SDL_Texture *player_left_texture;
    SDL_Texture *player_right_texture;
    SDL_Texture *player_shoot_texture;
    SDL_Texture *projectile_texture;
    SDL_Texture *monster_basic_texture;
    SDL_Texture *platform_normal_texture;
    SDL_Texture *platform_moving_texture;
    SDL_Texture *platform_breakable_texture;
    SDL_Texture *platform_spring_texture;
    SDL_Texture *background_texture;
    
#ifdef BADGEVMS_BUILD
    orientation_device_t *orientation; // BMI270 orientation sensor
#endif
} GameState;

// Function declarations
void init_game(GameState *game);
void generate_platforms(GameState *game, bool is_initial_generation);
void update_game(GameState *game, float delta_time);
void render_game(GameState *game);
void handle_input(GameState *game, const bool *keyboard_state, float delta_time);
void update_camera(GameState *game);
bool check_platform_collision(Player *player, Platform *platform);
void render_number(SDL_Renderer *renderer, int number, float x, float y, float scale);
void render_text(SDL_Renderer *renderer, const char *text, float x, float y, float scale, int r, int g, int b, int a);
void shoot_projectile(GameState *game);
void update_projectiles(GameState *game, float delta_time);
void render_projectiles(GameState *game);
void spawn_monster(GameState *game, float x, float y);
void update_monsters(GameState *game, float delta_time);
void render_monsters(GameState *game);
bool check_monster_collision(Player *player, Monster *monster);
bool check_projectile_monster_collision(Projectile *projectile, Monster *monster);
bool is_monster_nearby(GameState *game, float x, float y, float min_distance);

// Image loading functions
SDL_Texture* load_texture_from_file(SDL_Renderer *renderer, const char *filename);
SDL_Texture* load_tile_from_sprite_sheet(SDL_Renderer *renderer, const char *filename, int tile_x, int tile_y, int tile_width, int tile_height);
void load_game_textures(GameState *game);
void cleanup_textures(GameState *game);

// Load texture from image file using stb_image
SDL_Texture* load_texture_from_file(SDL_Renderer *renderer, const char *filename) {
    int width, height, channels;
    unsigned char *image_data = stbi_load(filename, &width, &height, &channels, 4); // Force 4 channels (RGBA)
    
    if (!image_data) {
        printf("Failed to load image %s: %s\n", filename, stbi_failure_reason());
        return NULL;
    }
    
    // Create SDL surface from image data
    SDL_Surface *surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, image_data, width * 4);
    if (!surface) {
        printf("Failed to create surface from image %s: %s\n", filename, SDL_GetError());
        stbi_image_free(image_data);
        return NULL;
    }
    
    // Create texture from surface
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("Failed to create texture from image %s: %s\n", filename, SDL_GetError());
    }
    
    // Cleanup
    SDL_DestroySurface(surface);
    stbi_image_free(image_data);
    
    return texture;
}

// Load a specific tile from a sprite sheet
SDL_Texture* load_tile_from_sprite_sheet(SDL_Renderer *renderer, const char *filename, int tile_x, int tile_y, int tile_width, int tile_height) {
    int width, height, channels;
    unsigned char *image_data = stbi_load(filename, &width, &height, &channels, 4); // Force 4 channels (RGBA)
    
    if (!image_data) {
        printf("Failed to load sprite sheet %s: %s\n", filename, stbi_failure_reason());
        return NULL;
    }
    
    // Create a smaller image buffer for the tile
    unsigned char *tile_data = (unsigned char*)malloc(tile_width * tile_height * 4);
    if (!tile_data) {
        printf("Failed to allocate memory for tile\n");
        stbi_image_free(image_data);
        return NULL;
    }
    
    // Extract the tile from the sprite sheet
    for (int y = 0; y < tile_height; y++) {
        for (int x = 0; x < tile_width; x++) {
            int src_x = tile_x + x;
            int src_y = tile_y + y;
            
            // Make sure we don't go out of bounds
            if (src_x >= width || src_y >= height) {
                // Fill with transparent pixels if out of bounds
                tile_data[(y * tile_width + x) * 4 + 0] = 0;     // R
                tile_data[(y * tile_width + x) * 4 + 1] = 0;     // G
                tile_data[(y * tile_width + x) * 4 + 2] = 0;     // B
                tile_data[(y * tile_width + x) * 4 + 3] = 0;     // A
            } else {
                // Copy pixel from source
                int src_index = (src_y * width + src_x) * 4;
                int dst_index = (y * tile_width + x) * 4;
                tile_data[dst_index + 0] = image_data[src_index + 0]; // R
                tile_data[dst_index + 1] = image_data[src_index + 1]; // G
                tile_data[dst_index + 2] = image_data[src_index + 2]; // B
                tile_data[dst_index + 3] = image_data[src_index + 3]; // A
            }
        }
    }
    
    // Create SDL surface from tile data
    SDL_Surface *surface = SDL_CreateSurfaceFrom(tile_width, tile_height, SDL_PIXELFORMAT_RGBA32, tile_data, tile_width * 4);
    if (!surface) {
        printf("Failed to create surface from tile: %s\n", SDL_GetError());
        free(tile_data);
        stbi_image_free(image_data);
        return NULL;
    }
    
    // Create texture from surface
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("Failed to create texture from tile: %s\n", SDL_GetError());
    }
    
    // Cleanup
    SDL_DestroySurface(surface);
    free(tile_data);
    stbi_image_free(image_data);
    
    return texture;
}

// Load all game textures
void load_game_textures(GameState *game) {
    // Initialize all textures to NULL
    game->player_left_texture = NULL;
    game->player_right_texture = NULL;
    game->player_shoot_texture = NULL;
    game->projectile_texture = NULL;
    game->monster_basic_texture = NULL;
    game->platform_normal_texture = NULL;
    game->platform_moving_texture = NULL;
    // Initialize all texture pointers to NULL first
    game->player_left_texture = NULL;
    game->player_right_texture = NULL;
    game->player_shoot_texture = NULL;
    game->projectile_texture = NULL;
    game->monster_basic_texture = NULL;
    game->platform_normal_texture = NULL;
    game->platform_moving_texture = NULL;
    game->platform_breakable_texture = NULL;
    game->platform_spring_texture = NULL;
    game->background_texture = NULL;
    
#if USE_IMAGES
    printf("Loading textures using BadgeVMS file paths...\n");
    // Try to load textures using BadgeVMS file paths
    game->player_left_texture = load_texture_from_file(game->renderer, "APPS:[DOODLE-JUMP]player_left.png");
    game->player_right_texture = load_texture_from_file(game->renderer, "APPS:[DOODLE-JUMP]player_right.png");
    game->player_shoot_texture = load_texture_from_file(game->renderer, "APPS:[DOODLE-JUMP]player_shoot.png");
    game->projectile_texture = load_texture_from_file(game->renderer, "APPS:[DOODLE-JUMP]projectile.png");
    
    // Load platform textures from sprite sheet using BadgeVMS file paths
    // Standard platform: made taller to include bottom pixels
    game->platform_normal_texture = load_tile_from_sprite_sheet(game->renderer, "APPS:[DOODLE-JUMP]game_tiles.png", 0, 0, 65, 18);
    // Moving platform: works perfectly, keep as is
    game->platform_moving_texture = load_tile_from_sprite_sheet(game->renderer, "APPS:[DOODLE-JUMP]game_tiles.png", 0, 18, 65, 18);
    // Breakable platform: normal state
    game->platform_breakable_texture = load_tile_from_sprite_sheet(game->renderer, "APPS:[DOODLE-JUMP]game_tiles.png", 0, 70, 65, 18);
    // Spring platform: try different position
    game->platform_spring_texture = load_tile_from_sprite_sheet(game->renderer, "APPS:[DOODLE-JUMP]game_tiles.png", 0, 35, 65, 18);

    // Load monster textures from sprite sheet (to the right of platforms)
    game->monster_basic_texture = load_tile_from_sprite_sheet(game->renderer, "APPS:[DOODLE-JUMP]game_tiles.png", 65, 0, 70, 90);

    game->background_texture = load_texture_from_file(game->renderer, "APPS:[DOODLE-JUMP]background.png");
    
    printf("Loaded textures: left=%p, right=%p, normal=%p, moving=%p, breakable=%p, spring=%p, monster=%p, bg=%p\n",
           (void*)game->player_left_texture, (void*)game->player_right_texture, 
           (void*)game->platform_normal_texture, (void*)game->platform_moving_texture,
           (void*)game->platform_breakable_texture, (void*)game->platform_spring_texture,
           (void*)game->monster_basic_texture, (void*)game->background_texture);
#else
    printf("Skipping texture loading - using colored rectangles for better memory usage\n");
#endif
}

// Cleanup all textures
void cleanup_textures(GameState *game) {
    if (game->player_left_texture) SDL_DestroyTexture(game->player_left_texture);
    if (game->player_right_texture) SDL_DestroyTexture(game->player_right_texture);
    if (game->player_shoot_texture) SDL_DestroyTexture(game->player_shoot_texture);
    if (game->projectile_texture) SDL_DestroyTexture(game->projectile_texture);
    if (game->monster_basic_texture) SDL_DestroyTexture(game->monster_basic_texture);
    if (game->platform_normal_texture) SDL_DestroyTexture(game->platform_normal_texture);
    if (game->platform_moving_texture) SDL_DestroyTexture(game->platform_moving_texture);
    if (game->platform_breakable_texture) SDL_DestroyTexture(game->platform_breakable_texture);
    if (game->platform_spring_texture) SDL_DestroyTexture(game->platform_spring_texture);
    if (game->background_texture) SDL_DestroyTexture(game->background_texture);
}

// Initialize the game state
void init_game(GameState *game) {
    // Initialize basic player properties first
    game->player.x = WINDOW_WIDTH / 2 - PLAYER_WIDTH / 2;
    game->player.vx = 0;
    game->player.vy = 0;
    game->player.width = PLAYER_WIDTH;
    game->player.height = PLAYER_HEIGHT;
    game->player.on_ground = true; // Start on ground
    game->player.facing_direction = 1; // Start facing right
    game->player.is_shooting = false;
    game->player.shoot_timer = 0;
    
    // Initialize projectiles
    game->num_projectiles = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        game->projectiles[i].active = false;
    }
    
    // Initialize monsters
    game->num_monsters = 0;
    for (int i = 0; i < MAX_MONSTERS; i++) {
        game->monsters[i].active = false;
    }
    
    // Initialize game state
    game->score = 0;
    game->platforms_landed = 0;
    game->last_platform_landed = -1;
    game->game_running = true;
    
    // Initialize input state
    game->left_pressed = false;
    game->right_pressed = false;
    game->shoot_pressed = false;
    game->restart_pressed = false;
    
#ifdef BADGEVMS_BUILD
    // Initialize BMI270 orientation sensor
    game->orientation = (orientation_device_t *)device_get("ORIENTATION0");
    if (game->orientation == NULL) {
        printf("Warning: BMI270 orientation sensor not found - tilt controls disabled\n");
    }
#endif
    
    // Load game textures
    load_game_textures(game);
    
    // Generate initial platforms FIRST
    generate_platforms(game, true);
    
    // NOW position player on the starting platform (first platform should be the starting one)
    if (game->num_platforms > 0) {
        Platform *start_platform = &game->platforms[0];
        game->player.y = start_platform->y - game->player.height - 40;
        // Center player on the starting platform
        game->player.x = start_platform->x + (start_platform->width - game->player.width) / 2;
        
        // Initialize camera - position it to show the starting platform
        game->camera_y = start_platform->y - WINDOW_HEIGHT + 100; // Show platform with some margin
    } else {
        // Fallback if no platforms generated (shouldn't happen)
        float starting_platform_y = WINDOW_HEIGHT - PLATFORM_HEIGHT - 50;
        game->player.y = starting_platform_y - PLAYER_HEIGHT;
        game->camera_y = starting_platform_y - WINDOW_HEIGHT + 100;
    }
}

// Generate platforms for the game (unified function for initial and ongoing generation)
void generate_platforms(GameState *game, bool is_initial_generation) {
    if (is_initial_generation) {
        // Reset platform count for initial generation
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
        
        // Generate initial platforms going upward
        float current_y = start_platform->y - PLATFORM_SPACING_MIN;
        
        for (int i = 1; i < MAX_PLATFORMS && current_y > -1000; i++) {
            Platform *platform = &game->platforms[game->num_platforms++];
            
            platform->x = (float)(SDL_rand(WINDOW_WIDTH - (int)PLATFORM_WIDTH));
            platform->y = current_y;
            platform->width = PLATFORM_WIDTH;
            platform->height = PLATFORM_HEIGHT;
            platform->active = true;
            
            // Set platform type and properties using helper logic
            int rand_type = SDL_rand(100);
            if (rand_type < 85) {
                platform->type = PLATFORM_NORMAL;
                platform->move_direction = 0;
                platform->move_speed = 0;
            } else if (rand_type < 90) {
                platform->type = PLATFORM_MOVING;
                platform->move_direction = (SDL_rand(2) == 0) ? 1.0f : -1.0f;
                platform->move_speed = game->score / 800.0f + (float)(SDL_rand(4)) / 20.0f;
                if (platform->move_speed < 0.5f) platform->move_speed = 0.5f;
                else if (platform->move_speed > 1.0f) platform->move_speed = 1.0f;
            } else if (rand_type < 95) {
                platform->type = PLATFORM_BREAKABLE;
                platform->move_direction = 0;
                platform->move_speed = 0;
            } else {
                platform->type = PLATFORM_SPRING;
                platform->move_direction = 0;
                platform->move_speed = 0;
            }
            
            // Calculate next platform position with difficulty scaling
            float height_factor = (-current_y) / 500.0f;
            int additional_spacing = (int)(height_factor * 15);
            if (additional_spacing > 30) additional_spacing = 30;
            
            current_y -= PLATFORM_SPACING_MIN + (SDL_rand(PLATFORM_SPACING_MAX - PLATFORM_SPACING_MIN)) + additional_spacing;
        }
    } else {
        // Continuous generation during gameplay
        // Find the highest platform
        float highest_y = game->camera_y;
        for (int i = 0; i < game->num_platforms; i++) {
            if (game->platforms[i].active && game->platforms[i].y < highest_y) {
                highest_y = game->platforms[i].y;
            }
        }
        
        // Generate new platforms if we need more above the highest point
        float target_height = game->camera_y - WINDOW_HEIGHT - 200;
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
                
                platform->x = (float)(SDL_rand(WINDOW_WIDTH - (int)PLATFORM_WIDTH));
                platform->y = current_y;
                platform->width = PLATFORM_WIDTH;
                platform->height = PLATFORM_HEIGHT;
                platform->active = true;
                
                // Set platform type and properties using helper logic
                int rand_type = SDL_rand(100);
                if (rand_type < 85) {
                    platform->type = PLATFORM_NORMAL;
                    platform->move_direction = 0;
                    platform->move_speed = 0;
                } else if (rand_type < 90) {
                    platform->type = PLATFORM_MOVING;
                    platform->move_direction = (SDL_rand(2) == 0) ? 1.0f : -1.0f;
                    platform->move_speed = game->score / 800.0f + (float)(SDL_rand(4)) / 20.0f;
                    if (platform->move_speed < 0.5f) platform->move_speed = 0.5f;
                    else if (platform->move_speed > 1.0f) platform->move_speed = 1.0f;
                } else if (rand_type < 95) {
                    platform->type = PLATFORM_BREAKABLE;
                    platform->move_direction = 0;
                    platform->move_speed = 0;
                } else {
                    platform->type = PLATFORM_SPRING;
                    platform->move_direction = 0;
                    platform->move_speed = 0;
                }
                
                // Calculate next platform position with difficulty scaling
                float height_factor = (-current_y) / 500.0f;
                int additional_spacing = (int)(height_factor * 15);
                if (additional_spacing > 30) additional_spacing = 30;
                
                current_y -= PLATFORM_SPACING_MIN + (SDL_rand(PLATFORM_SPACING_MAX - PLATFORM_SPACING_MIN)) + additional_spacing;
            } else {
                break;
            }
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

// Render text using bitmap font patterns (A-Z, 0-9)
void render_text(SDL_Renderer *renderer, const char *text, float x, float y, float scale, int r, int g, int b, int a) {
    // Set the color
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    
    // Define character patterns as 5x7 bitmaps for better quality
    // Each character is represented as 7 rows of 5-bit patterns
    static const unsigned char char_patterns[36][7] = {
        // A (0)
        {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001},
        // B (1)
        {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110},
        // C (2)
        {0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111},
        // D (3)
        {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110},
        // E (4)
        {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111},
        // F (5)
        {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000},
        // G (6)
        {0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111},
        // H (7)
        {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001},
        // I (8)
        {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111},
        // J (9)
        {0b11111, 0b00001, 0b00001, 0b00001, 0b00001, 0b10001, 0b01110},
        // K (10)
        {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001},
        // L (11)
        {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111},
        // M (12)
        {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001},
        // N (13)
        {0b10001, 0b11001, 0b10101, 0b10101, 0b10011, 0b10001, 0b10001},
        // O (14)
        {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110},
        // P (15)
        {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000},
        // Q (16)
        {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101},
        // R (17)
        {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001},
        // S (18)
        {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110},
        // T (19)
        {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100},
        // U (20)
        {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110},
        // V (21)
        {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100},
        // W (22)
        {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001},
        // X (23)
        {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001},
        // Y (24)
        {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100},
        // Z (25)
        {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111},
        // 0 (26)
        {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110},
        // 1 (27)
        {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
        // 2 (28)
        {0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111},
        // 3 (29)
        {0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110},
        // 4 (30)
        {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010},
        // 5 (31)
        {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110},
        // 6 (32)
        {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110},
        // 7 (33)
        {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000},
        // 8 (34)
        {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110},
        // 9 (35)
        {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}
    };
    
    float current_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        int char_index = -1;
        
        // Convert character to index
        if (c >= 'A' && c <= 'Z') {
            char_index = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            char_index = c - 'a'; // Convert lowercase to uppercase
        } else if (c >= '0' && c <= '9') {
            char_index = 26 + (c - '0');
        } else if (c == ' ') {
            // Space character - just advance position
            current_x += 6 * scale;
            continue;
        } else {
            // Unknown character - skip
            continue;
        }
        
        // Render the character
        if (char_index >= 0 && char_index < 36) {
            const unsigned char *pattern = char_patterns[char_index];
            
            for (int row = 0; row < 7; row++) {
                for (int col = 0; col < 5; col++) {
                    if (pattern[row] & (1 << (4 - col))) {
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
        
        // Move to next character position (5 pixels + 1 space)
        current_x += 6 * scale;
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

// Shooting system functions
void shoot_projectile(GameState *game) {
    if (game->player.is_shooting || game->num_projectiles >= MAX_PROJECTILES) return;
    
    Player *player = &game->player;
    
    // Create new projectile
    Projectile *projectile = &game->projectiles[game->num_projectiles];
    projectile->x = player->x + player->width / 2 - PROJECTILE_WIDTH / 2;
    projectile->y = player->y;
    projectile->vy = PROJECTILE_SPEED; // Negative speed means upward
    projectile->active = true;
    
    game->num_projectiles++;
    
    // Set player shooting state
    player->is_shooting = true;
    player->shoot_timer = 32.0f; // Show shooting animation for 0.8 seconds
}

void update_projectiles(GameState *game, float delta_time) {
    // Update player shooting timer
    Player *player = &game->player;
    if (player->is_shooting) {
        player->shoot_timer -= delta_time;
        if (player->shoot_timer <= 0) {
            player->is_shooting = false;
        }
    }
    
    // Update all projectiles
    for (int i = 0; i < game->num_projectiles; i++) {
        Projectile *projectile = &game->projectiles[i];
        if (!projectile->active) continue;
        
        // Update position
        projectile->y += projectile->vy * delta_time;
        
        // Deactivate projectiles that are off-screen (too far up)
        if (projectile->y + PROJECTILE_HEIGHT < game->camera_y - 100) {
            projectile->active = false;
        }
    }
    
    // Remove inactive projectiles by compacting the array
    int write_index = 0;
    for (int read_index = 0; read_index < game->num_projectiles; read_index++) {
        if (game->projectiles[read_index].active) {
            if (write_index != read_index) {
                game->projectiles[write_index] = game->projectiles[read_index];
            }
            write_index++;
        }
    }
    game->num_projectiles = write_index;
}

void render_projectiles(GameState *game) {
    for (int i = 0; i < game->num_projectiles; i++) {
        Projectile *projectile = &game->projectiles[i];
        if (!projectile->active) continue;
        
        SDL_FRect dest_rect = {
            projectile->x,
            projectile->y - game->camera_y,
            PROJECTILE_WIDTH,
            PROJECTILE_HEIGHT
        };
        
        if (game->projectile_texture) {
            // Render projectile texture
            SDL_RenderTexture(game->renderer, game->projectile_texture, NULL, &dest_rect);
        } else {
            // Fallback: render yellow rectangle
            SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255); // Yellow
            SDL_RenderFillRect(game->renderer, &dest_rect);
        }
    }
}

// Check if there's already a monster nearby to prevent clustering
bool is_monster_nearby(GameState *game, float x, float y, float min_distance) {
    for (int i = 0; i < game->num_monsters; i++) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) continue;
        
        float dx = monster->x - x;
        float dy = monster->y - y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        if (distance < min_distance) {
            return true;
        }
    }
    return false;
}

// Monster system functions
void spawn_monster(GameState *game, float x, float y) {
    if (game->num_monsters >= MAX_MONSTERS) return;
    
    // Check if there's already a monster nearby (prevent clustering)
    if (is_monster_nearby(game, x, y, 150.0f)) { // 150 pixel minimum distance
        return;
    }
    
    Monster *monster = &game->monsters[game->num_monsters];
    monster->x = x;
    monster->y = y;
    monster->vx = MONSTER_SPEED;
    monster->width = MONSTER_WIDTH;
    monster->height = MONSTER_HEIGHT;
    monster->type = MONSTER_BASIC;
    monster->active = true;
    monster->move_direction = (rand() % 2) * 2 - 1; // Random -1 or 1
    
    game->num_monsters++;
}

void update_monsters(GameState *game, float delta_time) {
    // Update all monsters
    for (int i = 0; i < game->num_monsters; i++) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) continue;
        
        // Update position (horizontal movement)
        monster->x += monster->move_direction * monster->vx * delta_time * 60.0f; // Scale by ~60 FPS
        
        // Bounce off screen edges
        if (monster->x <= 0) {
            monster->x = 0;
            monster->move_direction = 1.0f; // Move right
        } else if (monster->x + monster->width >= WINDOW_WIDTH) {
            monster->x = WINDOW_WIDTH - monster->width;
            monster->move_direction = -1.0f; // Move left
        }
        
        // Deactivate monsters that are too far below the camera (off-screen cleanup)
        if (monster->y > game->camera_y + WINDOW_HEIGHT + 200) {
            monster->active = false;
        }
    }
    
    // Remove inactive monsters by compacting the array
    int write_index = 0;
    for (int read_index = 0; read_index < game->num_monsters; read_index++) {
        if (game->monsters[read_index].active) {
            if (write_index != read_index) {
                game->monsters[write_index] = game->monsters[read_index];
            }
            write_index++;
        }
    }
    game->num_monsters = write_index;
    
    // Spawn new monsters based on score
    if (game->score >= MONSTER_SPAWN_SCORE_MIN) {
        // Count active monsters in current screen area
        int monsters_on_screen = 0;
        float screen_top = game->camera_y - WINDOW_HEIGHT;
        float screen_bottom = game->camera_y + WINDOW_HEIGHT;
        
        for (int i = 0; i < game->num_monsters; i++) {
            Monster *monster = &game->monsters[i];
            if (monster->active && monster->y >= screen_top && monster->y <= screen_bottom) {
                monsters_on_screen++;
            }
        }
        
        // Only spawn if there's no monster on the current screen
        if (monsters_on_screen == 0) {
            // Calculate spawn probability based on score
            float spawn_chance = 0.0005f; // Base chance
            if (game->score >= MONSTER_MAX_SCORE) {
                spawn_chance = 0.005f; // Maximum spawn rate
            } else {
                // Linear increase from min to max score
                float progress = (float)(game->score - MONSTER_SPAWN_SCORE_MIN) / (MONSTER_MAX_SCORE - MONSTER_SPAWN_SCORE_MIN);
                spawn_chance = 0.0005f + progress * 0.0045f; // Increase to 0.005f
            }
            
            // Try to spawn a monster (small random chance each frame)
            if ((float)rand() / RAND_MAX < spawn_chance) {
                float spawn_x = rand() % (WINDOW_WIDTH - MONSTER_WIDTH);
                float spawn_y = game->camera_y - 100; // Spawn above camera view
                spawn_monster(game, spawn_x, spawn_y);
            }
        }
    }
}

void render_monsters(GameState *game) {
    for (int i = 0; i < game->num_monsters; i++) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) continue;
        
        SDL_FRect dest_rect = {
            monster->x,
            monster->y - game->camera_y,
            monster->width,
            monster->height
        };
        
        if (game->monster_basic_texture) {
            // Render monster texture
            SDL_RenderTexture(game->renderer, game->monster_basic_texture, NULL, &dest_rect);
        } else {
            // Fallback: render red rectangle
            SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255); // Red
            SDL_RenderFillRect(game->renderer, &dest_rect);
        }
    }
}

bool check_monster_collision(Player *player, Monster *monster) {
    return (player->x < monster->x + monster->width &&
            player->x + player->width > monster->x &&
            player->y < monster->y + monster->height &&
            player->y + player->height > monster->y);
}

bool check_projectile_monster_collision(Projectile *projectile, Monster *monster) {
    return (projectile->x < monster->x + monster->width &&
            projectile->x + PROJECTILE_WIDTH > monster->x &&
            projectile->y < monster->y + monster->height &&
            projectile->y + PROJECTILE_HEIGHT > monster->y);
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
    generate_platforms(game, false);
    
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
    
    // Update projectiles
    update_projectiles(game, delta_time);
    
    // Update monsters
    update_monsters(game, delta_time);
    
    // Check projectile-monster collisions
    for (int i = 0; i < game->num_projectiles; i++) {
        Projectile *projectile = &game->projectiles[i];
        if (!projectile->active) continue;
        
        for (int j = 0; j < game->num_monsters; j++) {
            Monster *monster = &game->monsters[j];
            if (!monster->active) continue;
            
            if (check_projectile_monster_collision(projectile, monster)) {
                // Destroy both projectile and monster
                projectile->active = false;
                monster->active = false;
                break; // Exit inner loop since projectile is destroyed
            }
        }
    }
    
    // Check player-monster collisions (game over)
    for (int i = 0; i < game->num_monsters; i++) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) continue;
        
        if (check_monster_collision(player, monster)) {
            game->game_running = false; // Game over
            break;
        }
    }
    
    // Update score based on height (platforms passed)
    int new_score = (int)(-game->camera_y / 10);
    if (new_score > game->score) {
        game->score = new_score;
    }
    
    // Game over if player touches the bottom of the screen (regardless of platforms)
    if (player->y + player->height >= game->camera_y + WINDOW_HEIGHT) {
        game->game_running = false;
    }
}

// Handle player input
void handle_input(GameState *game, const bool *keyboard_state, float delta_time) {
    Player *player = &game->player;
    
    // Horizontal movement with acceleration
    float target_vx = 0;
    
    // Use event-based controls instead of keyboard polling for better badge compatibility
    if (game->left_pressed) {
        target_vx = -PLAYER_SPEED;
        player->facing_direction = -1; // Face left
    }
    if (game->right_pressed) {
        target_vx = PLAYER_SPEED;
        player->facing_direction = 1; // Face right
    }
    
#ifdef BADGEVMS_BUILD
    // BMI270 tilt controls (if sensor is available)
    if (game->orientation != NULL) {
        int degrees = game->orientation->_get_orientation_degrees(game->orientation);
        
        // Convert tilt angle to movement speed
        // Tilt range: -45° to +45° for left/right movement
        // 0° = no movement, -45° = full left, +45° = full right
        if (degrees >= 0 && degrees <= 180) {
            // Right tilt (0° to 45°)
            if (degrees <= 45) {
                float tilt_factor = (float)degrees / 45.0f; // 0.0 to 1.0
                target_vx = PLAYER_SPEED * tilt_factor;
                if (tilt_factor > 0.1f) player->facing_direction = 1; // Face right
            }
            // Left tilt (135° to 180°, then 180° to 225° mapped to negative)
            else if (degrees >= 135) {
                float tilt_factor;
                if (degrees <= 180) {
                    tilt_factor = (180.0f - (float)degrees) / 45.0f; // 1.0 to 0.0
                } else {
                    tilt_factor = ((float)degrees - 180.0f) / 45.0f; // 0.0 to 1.0
                }
                target_vx = -PLAYER_SPEED * tilt_factor;
                if (tilt_factor > 0.1f) player->facing_direction = -1; // Face left
            }
        } else if (degrees > 180) {
            // Handle 315° to 360° range (left tilt)
            if (degrees >= 315) {
                float tilt_factor = (360.0f - (float)degrees) / 45.0f; // 1.0 to 0.0
                target_vx = -PLAYER_SPEED * tilt_factor;
                if (tilt_factor > 0.1f) player->facing_direction = -1; // Face left
            }
        }
    }
#endif
    
    // Apply acceleration towards target velocity
    if (target_vx != 0) {
        player->vx += (target_vx - player->vx) * PLAYER_ACCELERATION * delta_time;
    } else {
        // Apply friction when no input
        player->vx *= PLAYER_FRICTION;
    }
    
    // Shooting input - use event-based controls
    if (game->shoot_pressed && game->game_running) {
        shoot_projectile(game);
    }
    
    // Restart game - use event-based controls
    if (game->restart_pressed && !game->game_running) {
        init_game(game);
    }
}

void renderBackground(GameState *game){
    if (game->background_texture) {
        // Get background texture dimensions
        float tex_width, tex_height;
        SDL_GetTextureSize(game->background_texture, &tex_width, &tex_height);

        // Calculate scale to fit width and maintain aspect ratio
        float scale = (float)WINDOW_WIDTH / tex_width;
        float scaled_height = tex_height * scale;

        // Center the background vertically (allow height overflow)
        float bg_y = (WINDOW_HEIGHT - scaled_height) / 2.0f;

        SDL_FRect bg_rect = {0, bg_y, WINDOW_WIDTH, scaled_height};
        SDL_RenderTexture(game->renderer, game->background_texture, NULL, &bg_rect);
    }
}

// Render the game
void render_game(GameState *game) {
    // Clear screen
    SDL_SetRenderDrawColor(game->renderer, 135, 206, 235, 255); // Sky blue fallback
    SDL_RenderClear(game->renderer);
    
    // Render background if available
    renderBackground(game);
 
    // Render platforms
    for (int i = 0; i < game->num_platforms; i++) {
        Platform *platform = &game->platforms[i];
        if (!platform->active) continue;
        
        // Convert world coordinates to screen coordinates
        SDL_FRect rect = {
            platform->x,
            platform->y - game->camera_y,
            platform->width,
            platform->height
        };
        
        // Only render if platform is visible on screen
        if (rect.y > -platform->height && rect.y < WINDOW_HEIGHT + platform->height) {
            // Try to use texture first, fall back to colored rectangle
            SDL_Texture *texture = NULL;
            switch (platform->type) {
                case PLATFORM_NORMAL:
                    texture = game->platform_normal_texture;
                    SDL_SetRenderDrawColor(game->renderer, 34, 139, 34, 255); // Forest green fallback
                    break;
                case PLATFORM_MOVING:
                    texture = game->platform_moving_texture;
                    SDL_SetRenderDrawColor(game->renderer, 255, 165, 0, 255); // Orange fallback
                    break;
                case PLATFORM_BREAKABLE:
                    texture = game->platform_breakable_texture;
                    SDL_SetRenderDrawColor(game->renderer, 139, 69, 19, 255); // Brown fallback
                    break;
                case PLATFORM_SPRING:
                    texture = game->platform_spring_texture;
                    SDL_SetRenderDrawColor(game->renderer, 255, 20, 147, 255); // Deep pink fallback
                    break;
            }
            
            if (texture) {
                // Render using texture
                SDL_RenderTexture(game->renderer, texture, NULL, &rect);
            } else {
                // Fallback to colored rectangle
                SDL_RenderFillRect(game->renderer, &rect);
            }
        }
    }
    
    // Render projectiles
    render_projectiles(game);
    
    // Render monsters
    render_monsters(game);
    
    // Render player
    SDL_FRect player_rect = {
        game->player.x,
        game->player.y - game->camera_y,
        game->player.width,
        game->player.height
    };
    
    // Choose the correct sprite based on facing direction and shooting state
    SDL_Texture *player_texture = NULL;
    if (game->player.is_shooting && game->player_shoot_texture) {
        // Use shooting texture when shooting
        player_texture = game->player_shoot_texture;
    } else if (game->player.facing_direction < 0) {
        player_texture = game->player_left_texture;
    } else {
        player_texture = game->player_right_texture;
    }
    
    if (player_texture) {
        // Render using directional texture
        SDL_RenderTexture(game->renderer, player_texture, NULL, &player_rect);
    } else {
        // Fallback to colored rectangle
        if (game->player.is_shooting) {
            // Bright yellow when shooting
            SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255); // Bright yellow
        } else {
            // Normal light red
            SDL_SetRenderDrawColor(game->renderer, 255, 100, 100, 255); // Light red
        }
        SDL_RenderFillRect(game->renderer, &player_rect);
    }
    
    // Render the score using our new text rendering function
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "SCORE %d", game->score);
    render_text(game->renderer, score_text, 10, 10, 2.0f, 0, 0, 0, 255); // Black text
    
    // Game over screen with big banner
    if (!game->game_running) {

        
        renderBackground(game);

        
        // Large "GAME OVER" banner using our new text rendering function
        float text_scale = 6.5f; // Make it much bigger
        float text_x = WINDOW_WIDTH / 2 - (strlen("GAME OVER") * 6 * text_scale) / 2; // Center text
        float text_y = (WINDOW_HEIGHT / 2 - (7 * text_scale) / 2) - 60; // Center vertically
        
        // Create bold effect by rendering multiple times with slight offsets
        // Shadow effect (darker red)
        render_text(game->renderer, "GAME OVER", text_x + 2, text_y + 2, text_scale, 128, 0, 0, 255);
        // Main text (bright red)
        render_text(game->renderer, "GAME OVER", text_x, text_y, text_scale, 255, 0, 0, 255);
        // Bold effect (render again slightly offset)
        render_text(game->renderer, "GAME OVER", text_x + 1, text_y, text_scale, 255, 0, 0, 255);
        render_text(game->renderer, "GAME OVER", text_x, text_y + 1, text_scale, 255, 0, 0, 255);
        
        // Score display below banner
        char score_text[32];
        snprintf(score_text, sizeof(score_text), "SCORE %d", game->score);
        float score_text_scale = 2.0f;
        float score_text_x = WINDOW_WIDTH / 2 - (strlen(score_text) * 6 * score_text_scale) / 2;
        float score_text_y = WINDOW_HEIGHT / 2 + 40;
        
        render_text(game->renderer, score_text, score_text_x, score_text_y, score_text_scale, 0, 0, 0, 0); // Black text
        
        // Restart hint - "PRESS R" 
        float hint_text_scale = 1.5f;
        float hint_text_x = WINDOW_WIDTH / 2 - (strlen("PRESS R") * 6 * hint_text_scale) / 2;
        float hint_text_y = WINDOW_HEIGHT / 2 + 100;
        
        render_text(game->renderer, "PRESS R", hint_text_x, hint_text_y, hint_text_scale, 255, 165, 0, 255);
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
    
    // Create fullscreen window
    game->window = SDL_CreateWindow("Doodle Jump - BadgeVMS", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_FULLSCREEN);
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
    game->last_time = (Uint32)SDL_GetTicks();  // Cast to 32-bit
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    GameState *game = (GameState *)appstate;
    
    // Calculate delta time using 32-bit arithmetic to avoid __floatundisf
    Uint32 current_time = (Uint32)SDL_GetTicks();  // Cast to 32-bit
    Uint32 time_diff = current_time - game->last_time;
    
    // Cap delta_time to prevent physics explosions on first frame or long pauses
    if (time_diff > 100) time_diff = 100;  // Max 100ms per frame
    
    float delta_time = (float)time_diff / 16.0f; // Normalize to ~60 FPS
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
    GameState *game = (GameState *)appstate;
    
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN:
            if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
                return SDL_APP_SUCCESS;
            }
            // Handle button presses
            switch (event->key.scancode) {
                case SDL_SCANCODE_LEFT:
                case SDL_SCANCODE_A:
                    game->left_pressed = true;
                    break;
                case SDL_SCANCODE_RIGHT:
                case SDL_SCANCODE_D:
                    game->right_pressed = true;
                    break;
                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_SPACE:
                    game->shoot_pressed = true;
                    break;
                case SDL_SCANCODE_R:
                    game->restart_pressed = true;
                    break;
            }
            break;
        case SDL_EVENT_KEY_UP:
            // Handle button releases
            switch (event->key.scancode) {
                case SDL_SCANCODE_LEFT:
                case SDL_SCANCODE_A:
                    game->left_pressed = false;
                    break;
                case SDL_SCANCODE_RIGHT:
                case SDL_SCANCODE_D:
                    game->right_pressed = false;
                    break;
                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_SPACE:
                    game->shoot_pressed = false;
                    break;
                case SDL_SCANCODE_R:
                    game->restart_pressed = false;
                    break;
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
        cleanup_textures(game);
        SDL_DestroyRenderer(game->renderer);
        SDL_DestroyWindow(game->window);
        SDL_free(game);
    }
}
