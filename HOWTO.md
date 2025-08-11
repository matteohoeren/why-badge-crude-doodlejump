*Test locally: *

1. ```. ~/esp/esp-idf/export.sh````
2. ```gcc sdk_apps/doodle-jump/main.c -o doodle-jump -I /usr/local/include/SDL3 -L/usr/local/lib -lSDL3``` (on MacOS use ```install_name_tool -add_rpath /Users/matteo/Downloads/vendored/SDL/build ./doodle-jump```)
3. Start with ./doodle-jump