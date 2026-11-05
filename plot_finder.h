#ifndef PLOT_FINDER_H
#define PLOT_FINDER_H

#include <optional>
#include <vector>
#include <mcpp/mcpp.h>
#include <random>
#include <cmath>

#include "plots.h"

// All required arguments passed from main.cpp
std::vector<Plot> find_plots(
    int loc_x,
    int loc_z,
    int village_size,
    std::mt19937& rng,
    bool debug,
    int density
);

std::optional<int> find_ground_height(
    const Plot& candidate,
    mcpp::MinecraftConnection& mc
);

bool check_obstacles(
    const Plot& candidate,
    mcpp::MinecraftConnection& mc,
    int ground_y
);

bool check_collision(
    const Plot& candidate,
    const std::vector<Plot>& existing_plots
);

#endif
