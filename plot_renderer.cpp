#include "plot_renderer.h"
#include "plots.h"
#include "utils.h"

void render_plot_boundaries(
    const std::vector<Plot>& plots,
    mcpp::MinecraftConnection& mc
) {
    mcpp::BlockType marker(mcpp::Blocks::GOLD_BLOCK);

    for (const Plot& plot : plots) {
        mcpp::Coordinate origin = plot.getOrigin();

        int min_x = origin.x;
        int min_z = origin.z;

        // Plot boundaries are exclusive so subtract 1
        int max_x = plot.get_Max_X() - 1;
        int max_z = plot.get_Max_Z() - 1;

        int y = origin.y;

        // North boundary

        profiled_mcpp("mcpp_set_blocks", [&]() { return mc.setBlocks(
            mcpp::Coordinate(min_x, y, min_z),
            mcpp::Coordinate(max_x, y, min_z),
            marker
        ); });

        // South boundary

        profiled_mcpp("mcpp_set_blocks", [&]() { return mc.setBlocks(
            mcpp::Coordinate(min_x, y, max_z),
            mcpp::Coordinate(max_x, y, max_z),
            marker
        ); });

        // West boundary

        profiled_mcpp("mcpp_set_blocks", [&]() { return mc.setBlocks(
            mcpp::Coordinate(min_x, y, min_z),
            mcpp::Coordinate(min_x, y, max_z),
            marker
        ); });

        // East boundary

        profiled_mcpp("mcpp_set_blocks", [&]() { return mc.setBlocks(
            mcpp::Coordinate(max_x, y, min_z),
            mcpp::Coordinate(max_x, y, max_z),
            marker
        ); });
    }
}
