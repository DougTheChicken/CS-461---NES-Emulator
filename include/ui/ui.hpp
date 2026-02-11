#pragma once

class console;

namespace ui {
    // Initializes the UI subsystem. Returns true on success, false on failure
    bool init();

    // Steps the UI, processing events and rendering. Returns false if the user requested to exit
    bool step(console& emu, bool& is_running);

    // Shuts down the UI subsystem, cleaning up resources
    void shutdown();
}