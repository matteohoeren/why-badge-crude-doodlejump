# Requirements

# Copilot Instructions:
- Refer to other example applicatons in /sdk_apps
- You build code for an ESP32-P4
- Use SDL 


# How to run:
Use gcc sdk_apps/$APPNAME/$APPNAME|MAIN.c -o $APPNAME \
  -I/Users/matteo/Downloads/vendored/SDL/include \
  -L/Users/matteo/Downloads/vendored/SDL/build \
  -lSDL3.0


# Goal
- Build a fully functional doodle jump game.
- Inputs should include Arrow Keys of the Keyboard (use SDL) or the BMI270 Sensor to detect the badge tilting in order to steer the doodle from left to right
- Jumping up from a block should start fast and decrease speed while jumping up, considering gravity.
- With the enter key, the doodle can shoot upwards to destroy monsters.
- Monsters will only be implemented later.


# Graphics:
- Right now there are no assets. Use generic Rectangles first.

# Compiling:
1. Compile the game:
   ```bash
   gcc main.c -o doodle-jump \
     -I/Users/matteo/Downloads/vendored/SDL/include \
     -L/Users/matteo/Downloads/vendored/SDL/build \
     -lSDL3.0
   ```

2. Fix the SDL library path:
   ```bash
   install_name_tool -change @rpath/libSDL3.0.dylib /Users/matteo/Downloads/vendored/SDL/build/libSDL3.0.dylib doodle-jump
   ```

3. Run the game:
   ```bash
   ./doodle-jump
   ```

**One-liner (compile, fix library, and run):**
```bash
cd sdk_apps/doodle-jump && 
gcc main.c -o doodle-jump -I/Users/matteo/Downloads/vendored/SDL/include -L/Users/matteo/Downloads/vendored/SDL/build -lSDL3.0 && install_name_tool -change @rpath/libSDL3.0.dylib /Users/matteo/Downloads/vendored/SDL/build/libSDL3.0.dylib doodle-jump && ./doodle-jump
```

# Intro Text of the Badge:
Meet the Badge: Modular, Powerful, and Ready to Hack
At the core of the badge is a Compute Unit powered by the all-new Espressif ESP32-P4-32MB.
This little powerhouse is designed with after-camp life in mind.
We want you to take this unit, build your own Carrier Boards, and give it a second (or third) life in future projects.
Compute Unit
ESP32-P4 – Your program’s main stage, fast and flexible.
Carrier Board
ESP32-C6 – Dedicated for connectivity (Wi-Fi + Bluetooth)
Display – A crisp 4” square screen will be your window to the camp magic.
Frontplate – Currently undergoing the last round of refinement to make sure it both looks and feels great.
Keyboard – Coming with a little twist, more on that soon.
Sensor BMI270 – Measures motion and orientation (accelerometer + gyroscope).
Sensor BME690 – Measures air quality, temperature, humidity, and pressure.