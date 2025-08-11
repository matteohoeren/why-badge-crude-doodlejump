# Doodle Jump Game - Task List

## Phase 1: Basic Setup and Map Rendering
- [x] Create TASKLIST.md 
- [x] Set up SDL3 window and renderer (following sdl_test example)
- [x] Implement basic game structure with SDL callbacks
- [x] Create platform data structure
- [x] Implement map generation with platforms
- [x] Render platforms as rectangles
- [x] Implement camera/viewport system that follows player
- [x] Add platform spacing and positioning logic

## Phase 2: Player Character (Doodle)
- [x] Create player data structure (position, velocity, etc.)
- [x] Implement player rendering as rectangle
- [x] Add gravity system
- [x] Implement jumping physics (fast start, slowing due to gravity)
- [x] Add collision detection with platforms
- [x] Implement automatic bouncing when landing on platforms

## Phase 3: Input System
- [x] Implement keyboard controls (arrow keys for left/right movement)
- [ ] Add BMI270 sensor integration for tilt controls
- [ ] Handle input smoothly during gameplay
- [ ] Implement shooting mechanism (Enter key to shoot upwards)

## Phase 4: Game Mechanics
- [ ] Implement platform types (normal, moving, breakable, etc.)
- [ ] Add scoring system based on height reached
- [ ] Implement game over conditions
- [ ] Add restart functionality
- [ ] Implement infinite map generation (procedural platforms)

## Phase 5: Advanced Features (Later - Monsters)
- [ ] Add monster data structures
- [ ] Implement monster AI and movement
- [ ] Add collision detection between bullets and monsters
- [ ] Add monster destruction mechanics
- [ ] Add visual feedback for shooting

## Phase 6: Polish and Optimization
- [ ] Add sound effects (if supported)
- [ ] Optimize rendering performance
- [ ] Add game state management (menu, playing, game over)
- [ ] Add visual effects and polish
- [ ] Test on actual hardware

## Current Focus: Phase 1 - Map Rendering Logic
Starting with basic SDL setup and platform rendering system.
