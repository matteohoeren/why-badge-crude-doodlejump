# Doodle Jump Game Implementation Task List

## ✅ Completed Tasks

### Phase 1: Basic Setup and Map Rendering
- [x] Create TASKLIST.md 
- [x] Set up SDL3 window and renderer (640x480 to match SDL examples)
- [x] Fixed keyboard input type issue (const bool * instead of const Uint8 *)
- [x] Implement basic game structure with SDL callbacks
- [x] Create platform data structure
- [x] Implement map generation with platforms
- [x] Render platforms as rectangles
- [x] Implement camera/viewport system that follows player
- [x] Add platform spacing and positioning logic

### Phase 2: Player Character (Doodle)
- [x] Create player data structure (position, velocity, etc.)
- [x] Implement player rendering as rectangle
- [x] Add gravity system
- [x] Implement jumping physics (fast start, slowing due to gravity)
- [x] Add collision detection with platforms
- [x] Implement automatic bouncing when landing on platforms
- [x] Horizontal movement with arrow keys (A/D also supported)
- [x] Screen wrapping (player wraps around horizontally)

### Phase 3: Score System
- [x] Platform counter that increments each time player lands on a new platform
- [x] Visual score display at top of screen using rectangle patterns
- [x] Prevent double-counting same platform

### Phase 4: Enhanced Physics and Controls
- [x] **COMPLETED** - Improved jump physics (faster initial speed, slower as it goes up)
- [x] **COMPLETED** - Better gravity simulation with more realistic physics constants
- [x] **COMPLETED** - Enhanced horizontal movement with acceleration and friction
- [x] **COMPLETED** - Variable jump forces for different platform types
- [ ] BMI270 sensor integration for tilt controls (hardware integration phase)

## 🔄 In Progress Tasks

### Phase 5: Platform Types
- [x] **COMPLETED** - Normal platforms (green, static)
- [x] **COMPLETED** - Moving platforms (orange, horizontal movement with edge bouncing)
- [x] **COMPLETED** - Breakable platforms (brown, disappear after landing)
- [x] **COMPLETED** - Spring platforms (deep pink, higher jump) ✅ Just completed!

### Phase 6: Game Mechanics
- [x] **COMPLETED** - Implement game over conditions ✅ Enhanced - triggers when whole player body exits screen
- [x] **COMPLETED** - Add restart functionality ✅ R key works with improved game over screen
- [x] **COMPLETED** - Implement infinite map generation (procedural platforms) ✅ Already working
- [x] **COMPLETED** - Difficulty scaling (wider gaps, fewer platforms as you go higher) ✅ Just implemented!

### Phase 6.5: Bug Fixes & Polish
- [x] **COMPLETED** - Fix moving platform speed (reduced from 20 to 10 speed units)
- [x] **COMPLETED** - Fix game over condition (triggers when whole player exits screen)
- [x] **COMPLETED** - Add platform cleanup (remove off-screen platforms for performance)
- [x] **COMPLETED** - Tested compilation and execution process
- [x] **COMPLETED** - Fix moving platform speed again (reduced from 5.0f to 0.5f - 10x slower)
- [x] **COMPLETED** - Increase jump height (from -18.0f to -20.0f for better reachability)
- [x] **COMPLETED** - Fix starting platform visibility (20px offset from bottom + platform height)
- [x] **COMPLETED** - Fix camera scroll mechanism (added offset to keep last platform visible)
- [x] **COMPLETED** - Fix player starting position (positioned properly above starting platform)
- [x] **COMPLETED** - Fix scrolling mechanism and jump force (reduced jump to -18.0f, improved camera offset)
- [x] **COMPLETED** - Fix game over banner centering and make font much bigger
- [x] **COMPLETED** - Platform cleanup (platforms disappear when off-screen) ✅ Just implemented!
- [x] **COMPLETED** - Improved moving platform physics (proper speed and bounds checking) ✅ Just fixed!
- [x] **COMPLETED** - Make jumps slightly easier to reach (increased jump force to -20.0f and reduced platform spacing)
- [x] **COMPLETED** - Implement infinite platform generation (platforms generate infinitely as player moves up) ✅ Just implemented!

### Phase 7: Monster System
- [x] **COMPLETED** - Implement shooting mechanism (Enter key to shoot upwards) ✅ Working with projectiles!
- [x] **COMPLETED** - Add visual feedback for shooting ✅ Player changes to yellow when shooting!
- [ ] **IN PROGRESS** - Monster data structures and basic rendering
  - [ ] Step 7.1: Add Monster structure (position, type, active state)
  - [ ] Step 7.2: Add monster texture loading from game_tiles.png (sprite extraction)
  - [ ] Step 7.3: Add monster spawning system (score 1000-1500 trigger, max at 5000)
  - [ ] Step 7.4: Implement monster rendering
  - [ ] Step 7.5: Add monster movement patterns (simple horizontal movement)
- [ ] Add collision detection between player and monsters (game over)
- [ ] Add collision detection between projectiles and monsters (monster destruction)
- [ ] Implement monster difficulty scaling (more monsters at higher scores)
- [ ] Add different monster types and behaviors
- [ ] Add visual effects for monster destruction

### Phase 8: Visual Enhancements
- [ ] Particle effects for jumping
- [ ] Better visual representation (sprites instead of rectangles)
- [ ] Background parallax scrolling
- [ ] Platform destruction animations
- [ ] Player animation states (jumping, falling)

### Phase 9: Audio
- [ ] Jump sound
- [ ] Platform land sound
- [ ] Game over sound
- [ ] Background music

### Phase 10: Game States
- [ ] Main menu
- [ ] Pause functionality
- [ ] High score system
- [ ] Game over screen with restart option ✅ Basic restart works

### Phase 11: Hardware Integration
- [ ] BMI270 accelerometer/gyroscope integration for tilt controls
- [ ] BME690 sensor integration (maybe for environmental effects?)
- [ ] Proper badge keyboard support
- [ ] Performance optimization for ESP32-P4

### Phase 12: Optimization and Polish
- [ ] Memory usage optimization
- [ ] Frame rate optimization
- [ ] Code cleanup and documentation
- [ ] Error handling improvements
- [ ] Configuration system (difficulty levels, controls)

## Current Status
- ✅ **Enhanced playable game with improved physics!**
- ✅ Player can move left/right with smooth acceleration and friction
- ✅ Camera follows player upward smoothly
- ✅ Score counter shows platforms landed at top of screen
- ✅ **NEW:** Four platform types - Normal (green), Moving (orange), Breakable (brown), Spring (deep pink)
- ✅ **NEW:** Enhanced jump physics with better gravity simulation
- ✅ **NEW:** Variable jump forces based on platform type
- ✅ **NEW:** Difficulty scaling - platforms get further apart as you go higher
- ✅ **NEW:** Big centered "GAME OVER" banner with much larger text
- ✅ **NEW:** Improved moving platform physics (proper speed and edge bouncing)
- ✅ **NEW:** Platform cleanup system (platforms disappear when off-screen)
- ✅ Game over when player's whole body exits screen (more precise)
- ✅ Restart with 'R' key when game is over
- ❌ No monsters yet (planned for later phases)
- ❌ No BMI270 sensor integration yet

## Next Priority Tasks
1. **Improve jump physics** - Make jumping feel more natural with initial burst and gravity
2. **Add platform variety** - Moving and breakable platforms for more interesting gameplay
3. **BMI270 integration** - Add tilt controls for ESP32-P4 badge
4. **Monster system** - Add enemies and shooting mechanics

## Build Instructions
```bash
# Navigate to doodle-jump directory
cd sdk_apps/doodle-jump

# Compile (replace paths as needed)
gcc main.c -o doodle-jump \
  -I/Users/matteo/Downloads/vendored/SDL/include \
  -L/Users/matteo/Downloads/vendored/SDL/build \
  -lSDL3.0

# Fix library path for proper execution
install_name_tool -change @rpath/libSDL3.0.dylib /Users/matteo/Downloads/vendored/SDL/build/libSDL3.0.dylib doodle-jump

# Run the game
./doodle-jump
```
