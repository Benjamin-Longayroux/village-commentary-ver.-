#include <stdexcept>
#include <string>
#include <algorithm>

#include "exterior_renderer.h"
#include "utils.h"

namespace {

void render_exterior_walls(
    const Plot& plot,
    mcpp::MinecraftConnection& mc
) {
    const mcpp::Coordinate origin = plot.getOrigin();

    const int min_x = origin.x;
    const int min_z = origin.z;
    const int max_x = plot.get_Max_X() - 1;
    const int max_z = plot.get_Max_Z() - 1;

    const int bottom_y = origin.y + 1;
    const int top_y = plot.get_Max_Y() - 1;


    // Minecraft block states used for the timber-frame exterior.
    const std::string infill = "minecraft:stripped_spruce_wood";
    const std::string vertical_log = "minecraft:spruce_log[axis=y]";
    const std::string horizontal_x = "minecraft:spruce_log[axis=x]";
    const std::string horizontal_z = "minecraft:spruce_log[axis=z]";

    // Place rectangular regions using a single Minecraft command.
    // Coordinates passed to /fill are inclusive.
    const auto fill = [&mc](
        int x1, int y1, int z1,
        int x2, int y2, int z2,
        const std::string& block
    ) {
        performance_profile().add("mcpp_do_command");
        mc.doCommand(
            "fill " +
            std::to_string(x1) + " " +
            std::to_string(y1) + " " +
            std::to_string(z1) + " " +
            std::to_string(x2) + " " +
            std::to_string(y2) + " " +
            std::to_string(z2) + " " +
            block + " replace"
        );
    };

    // 1. Construct the four wall faces with stripped spruce.
    fill(min_x, bottom_y, min_z,
         max_x, top_y, min_z, infill);

    fill(min_x, bottom_y, max_z,
         max_x, top_y, max_z, infill);

    fill(min_x, bottom_y, min_z + 1,
         min_x, top_y, max_z - 1, infill);

    fill(max_x, bottom_y, min_z + 1,
         max_x, top_y, max_z - 1, infill);


    // 2. Add two-block-high horizontal beams between storeys.
    // The lower block represents the previous storey's ceiling.
    // The upper block represents the next storey's floor.
    for (int level = 0; level <= plot.getFloors(); ++level) {
        const int beam_bottom_y = origin.y + level * floor_height - 1;
         const int beam_top_y =
            level == plot.getFloors()
                ? beam_bottom_y
                : beam_bottom_y + 1;

        // North and south: logs run along X.
        fill(min_x + 1, beam_bottom_y, min_z,
             max_x - 1, beam_top_y, min_z, horizontal_x);

        fill(min_x + 1, beam_bottom_y, max_z,
             max_x - 1, beam_top_y, max_z, horizontal_x);

        // East and west: logs run along Z.
        fill(min_x, beam_bottom_y, min_z + 1,
             min_x, beam_top_y, max_z - 1, horizontal_z);

        fill(max_x, beam_bottom_y, min_z + 1,
             max_x, beam_top_y, max_z - 1, horizontal_z);
    }


    // 3. Finish with uninterrupted vertical corner pillars.
    fill(min_x, bottom_y, min_z,
         min_x, top_y, min_z, vertical_log);

    fill(max_x, bottom_y, min_z,
         max_x, top_y, min_z, vertical_log);

    fill(min_x, bottom_y, max_z,
         min_x, top_y, max_z, vertical_log);

    fill(max_x, bottom_y, max_z,
         max_x, top_y, max_z, vertical_log);
}


void render_roof(const Plot& plot, mcpp::MinecraftConnection& mc) {
    const mcpp::Coordinate origin = plot.getOrigin();

    const int min_x = origin.x;
    const int min_z = origin.z;
    const int max_x = plot.get_Max_X() - 1;
    const int max_z = plot.get_Max_Z() - 1;

    const int roof_y = plot.get_Max_Y();

    const int width = max_x - min_x + 1;
    const int depth = max_z - min_z + 1;

    // The roof ridge follows the longest dimension.
    const bool ridge_along_x = width >= depth;
    const int span = ridge_along_x ? depth : width;

    for (int i = 0; i < span; ++i) {
        // Rise towards the centre, then descend symmetrically.
        const int half_steps = std::min(i, span - 1 - i);

        const int y = roof_y + half_steps / 2;

        // Alternate slab halves to produce a 0.5-block rise.
        const std::string block =
        (half_steps % 2 == 0)
            ? "minecraft:deepslate_tile_slab[type=bottom]"
            : "minecraft:deepslate_tiles";

        // Each strip runs parallel to the ridge.
        const int x1 = ridge_along_x ? min_x : min_x + i;
        const int x2 = ridge_along_x ? max_x : min_x + i;

        const int z1 = ridge_along_x ? min_z + i : min_z;
        const int z2 = ridge_along_x ? min_z + i : max_z;

        performance_profile().add("mcpp_do_command");

        mc.doCommand(
            "fill " +
            std::to_string(x1) + " " +
            std::to_string(y) + " " +
            std::to_string(z1) + " " +
            std::to_string(x2) + " " +
            std::to_string(y) + " " +
            std::to_string(z2) + " " +
            block + " replace"
        );
    }
}


void render_roof_gables(
    const Plot& plot,
    mcpp::MinecraftConnection& mc
) {
    const mcpp::Coordinate origin = plot.getOrigin();

    const int min_x = origin.x;
    const int min_z = origin.z;
    const int max_x = plot.get_Max_X() - 1;
    const int max_z = plot.get_Max_Z() - 1;

    const int width = max_x - min_x + 1;
    const int depth = max_z - min_z + 1;

    const bool ridge_along_x = width >= depth;
    const int span = ridge_along_x ? depth : width;

    // Spruce begins immediately above the top-floor timber band.
    const int base_y = plot.get_Max_Y();

    const std::string infill = "minecraft:stripped_spruce_wood";

    // Render an entire vertical column in one API command.
    const auto fill_column = [&](int x, int z, int top_y) {
        performance_profile().add("mcpp_do_command");

        mc.doCommand(
            "fill " +
            std::to_string(x) + " " +
            std::to_string(base_y) + " " +
            std::to_string(z) + " " +
            std::to_string(x) + " " +
            std::to_string(top_y) + " " +
            std::to_string(z) + " " +
            infill + " replace"
        );
    };

    for (int i = 0; i < span; ++i) {
        // Match the existing symmetrical roof pitch.
        const int half_steps = std::min(i, span - 1 - i);

        // Stop one whole block below the roof surface.
        const int infill_top_y = base_y + half_steps / 2 - 1;

        // Fill both gable ends only when a column fits below the roof.
        if (infill_top_y >= base_y) {
            if (ridge_along_x) {
                const int z = min_z + i;

                fill_column(min_x, z, infill_top_y);
                fill_column(max_x, z, infill_top_y);
            } else {
                const int x = min_x + i;

                fill_column(x, min_z, infill_top_y);
                fill_column(x, max_z, infill_top_y);
            }
        }
    }
}


void render_entrance(
    const Entrance& entrance,
    mcpp::MinecraftConnection& mc
) {
    std::string facing = "north";

    if (entrance.facing == Direction::SOUTH) {
        facing = "south";
    } else if (entrance.facing == Direction::EAST) {
        facing = "east";
    } else if (entrance.facing == Direction::WEST) {
        facing = "west";
    }

    const mcpp::Coordinate position = entrance.position;

    const std::string lower_position =
        std::to_string(position.x) + " " +
        std::to_string(position.y) + " " +
        std::to_string(position.z);

    const std::string upper_position =
        std::to_string(position.x) + " " +
        std::to_string(position.y + 1) + " " +
        std::to_string(position.z);

    const std::string block_state =
        "minecraft:spruce_door[facing=" + facing +
        ",hinge=left,open=false,powered=false,half=";

    performance_profile().add("mcpp_do_command", 2);
    mc.doCommand("setblock " + lower_position + " " +
                 block_state + "lower] replace");
    mc.doCommand("setblock " + upper_position + " " +
                 block_state + "upper] replace");
}

}

void render_exterior(const Plot& plot, mcpp::MinecraftConnection& mc) {
    const std::optional<Entrance> entrance = plot.getEntrance();

    if (!entrance.has_value()) {
        throw std::logic_error("Building entrance has not been allocated");
    }

    render_exterior_walls(plot, mc);
    render_roof_gables(plot, mc);
    render_roof(plot, mc);
    render_entrance(*entrance, mc);
}

