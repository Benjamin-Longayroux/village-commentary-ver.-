#ifndef ENTRANCE_PLANNER_H
#define ENTRANCE_PLANNER_H

#include <random>
#include <vector>

#include "plots.h"
#include "room_subdivision.h"

// Village maximums are exclusive, like Plot's upper bounds.
struct VillageBounds {
    int min_x;
    int min_z;
    int max_x_exclusive;
    int max_z_exclusive;
};

// Select an interior-safe entrance with a clear exterior approach.
// Prefer three clear cells, but allow shorter legal approaches if needed.
void allocate_exterior_entrance(
    Plot& plot, const FloorLayout& layout, std::mt19937& rng,
    const std::vector<Plot>& plots, const VillageBounds& bounds,
    mcpp::MinecraftConnection& mc
);

#endif
