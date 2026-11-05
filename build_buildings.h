#ifndef BUILD_BUILDINGS_H
#define BUILD_BUILDINGS_H

#include <vector>
#include <iostream>
#include <random>

#include "plots.h"
#include "plot_finder.h"
#include "entrance_planner.h"

void build_floor(
    const Plot& plot,
    mcpp::MinecraftConnection& mc
);

std::size_t build_house(
    Plot& plot,
    mcpp::MinecraftConnection& mc,
    std::mt19937& rng,
    bool debug,
    const std::vector<Plot>& all_plots,
    const VillageBounds& bounds
);

void build_buildings(
    std::vector<Plot>& plots,
    mcpp::MinecraftConnection& mc,
    std::mt19937& rng,
    bool debug,
    const VillageBounds& bounds
);

#endif