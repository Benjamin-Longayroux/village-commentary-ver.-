#ifndef ROOM_RENDERER_H
#define ROOM_RENDERER_H

#include <mcpp/mcpp.h>
#include "room_subdivision.h"

// Renders a planned interior above the supplied floor Y coordinate.
void render_interior(
    const FloorLayout& layout,
    int floor_y,
    mcpp::MinecraftConnection& mc
);

#endif