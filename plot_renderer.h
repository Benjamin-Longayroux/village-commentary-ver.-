#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include <vector>
#include <mcpp/mcpp.h>

#include "plots.h"

void render_plot_boundaries(
    const std::vector<Plot>& plots,
    mcpp::MinecraftConnection& mc
);

#endif
