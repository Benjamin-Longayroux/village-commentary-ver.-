#include <string>

#include "room_renderer.h"
#include "plots.h"
#include "utils.h"


// Rendering helpers are only used by this implementation file.
namespace {

void render_wall(
    const InteriorWall& wall,
    int floor_y,
    mcpp::MinecraftConnection& mc
) {
    const int bottom_y = floor_y + 1;
    const int top_y = floor_y + floor_height - 1;
    const mcpp::BlockType material(mcpp::Blocks::SPRUCE_WOOD_PLANK);

    // A wall is rendered once, not once for each adjacent room.
    if (wall.along_x) {

        profiled_mcpp("mcpp_set_blocks", [&]() { return mc.setBlocks(
            mcpp::Coordinate(wall.coordinate, bottom_y, wall.start),
            mcpp::Coordinate(wall.coordinate, top_y, wall.end - 1),
            material); });
    } else {

        profiled_mcpp("mcpp_set_blocks", [&]() { return mc.setBlocks(
            mcpp::Coordinate(wall.start, bottom_y, wall.coordinate),
            mcpp::Coordinate(wall.end - 1, top_y, wall.coordinate),
            material); });
    }
}

void render_door(
    const InteriorDoor& door,
    int floor_y,
    mcpp::MinecraftConnection& mc
) {
    std::string facing = "north";

    // Closed doors face across their partition, not along it.
    if (door.along_x) {
        facing = "east";
    }

    const std::string position =
        std::to_string(door.x) + " " +
        std::to_string(floor_y + 1) + " " +
        std::to_string(door.z);

    const std::string top_position =
        std::to_string(door.x) + " " +
        std::to_string(floor_y + 2) + " " +
        std::to_string(door.z);

    const std::string block_state =
        "minecraft:spruce_door[facing=" +
        facing +
        ",hinge=left,open=false,powered=false,half=";

    profiled_command(
        mc, "setblock " + position + " " + block_state + "lower] replace");
    profiled_command(
        mc, "setblock " + top_position + " " +
        block_state + "upper] replace");
}

}

void render_interior(
    const FloorLayout& layout,
    int floor_y,
    mcpp::MinecraftConnection& mc
) {
    // Render one recorded partition per call; shared walls are not duplicated.
    {
        PerformanceStage timer("interior.render_walls");
        for (const InteriorWall& wall : layout.walls) {
            render_wall(wall, floor_y, mc);
        }
    }

    // Replace the lower two blocks of each wall at planned door positions.
    {
        PerformanceStage timer("interior.render_doors");
        for (const InteriorDoor& door : layout.doors) {
            render_door(door, floor_y, mc);
        }
    }
}