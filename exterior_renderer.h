#ifndef EXTERIOR_RENDERER_H
#define EXTERIOR_RENDERER_H

#include "plots.h"

// Render the building perimeter, roof, and previously allocated entrance.
void render_exterior(
    const Plot& plot,
    mcpp::MinecraftConnection& mc
);

#endif