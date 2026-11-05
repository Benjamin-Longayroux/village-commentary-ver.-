#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "build_buildings.h"
#include "room_subdivision.h"
#include "room_renderer.h"
#include "exterior_renderer.h"
#include "entrance_planner.h"
#include "utils.h"
#include "PREFABS/furniture_planner.h"
#include "PREFABS/furniture_catalogue.h"

void build_floor(const Plot& plot, mcpp::MinecraftConnection& mc) {
    const mcpp::Coordinate start = plot.getOrigin();
    const mcpp::Coordinate end = {
        plot.get_Max_X() - 1, start.y, plot.get_Max_Z() - 1
    };
    profiled_mcpp("mcpp_set_blocks", [&]() {
        mc.setBlocks(start, end, mcpp::Blocks::SPRUCE_WOOD_PLANK);
    });
}

namespace {

    constexpr int ladder_clearance = 2;
    constexpr int entrance_landing_length = 2;

    const std::vector<prefabs::Prefab>& furniture_catalogue() {
        // Discover once, then reuse the same order across houses and floors.
        static const std::vector<prefabs::Prefab> assets =
            prefabs::load_furniture_catalogue("PREFABS/ASSETS", std::cerr);
        return assets;
    }

    bool contains_cell(const Room& area, int x, int z) {
        return x >= area.min_x && x < area.max_x &&
            z >= area.min_z && z < area.max_z;
    }

    bool contains_area(const FloorLayout& layout, const Room& area) {
        bool contained = false;
        for (const Room& room : layout.rooms) {
            if (area.min_x >= room.min_x && area.max_x <= room.max_x &&
                area.min_z >= room.min_z && area.max_z <= room.max_z) {
                contained = true;
            }
        }
        return contained;
    }

    bool overlaps_entrance(const Room& area, const Entrance& entrance) {
        int inward_x = 0;
        int inward_z = 0;
        if (entrance.facing == Direction::NORTH) {
            inward_z = 1;
        } else if (entrance.facing == Direction::SOUTH) {
            inward_z = -1;
        } else if (entrance.facing == Direction::EAST) {
            inward_x = -1;
        } else {
            inward_x = 1;
        }
        bool overlap = false;
        for (int step = 1; step <= entrance_landing_length; ++step) {
            const int x = entrance.position.x + step * inward_x;
            const int z = entrance.position.z + step * inward_z;
            if (contains_cell(area, x, z)) {
                overlap = true;
            }
        }
        return overlap;
    }

    prefabs::FloorAccess plan_ladder(
        const Plot& plot, const std::vector<FloorLayout>& layouts,
        std::mt19937& rng
    ) {
        PerformanceStage timer("connector.plan");
        const auto entrance = plot.getEntrance();

        if (!entrance.has_value()) {
            throw std::logic_error("Plan the entrance before the ladder");
        }

        const mcpp::Coordinate origin = plot.getOrigin();
        const int min_x = origin.x + 1;
        const int min_z = origin.z + 1;
        const int corner_x[] = {min_x, plot.get_Max_X() - 2};
        const int corner_z[] = {min_z, plot.get_Max_Z() - 2};
        std::vector<prefabs::FloorAccess> candidates = {};

        for (int x : corner_x) {
            for (int z : corner_z) {
                const int area_x = x == min_x ? x : x - ladder_clearance + 1;
                const int area_z = z == min_z ? z : z - ladder_clearance + 1;
                const Room area = {
                    area_x, area_z,
                    area_x + ladder_clearance, area_z + ladder_clearance
                };
                bool valid = !overlaps_entrance(area, *entrance);
                for (const FloorLayout& layout : layouts) {
                    valid = valid && contains_area(layout, area);
                }
                if (valid) {
                    candidates.push_back({0, area, x, z});
                }
            }
        }

        if (candidates.empty()) {
            throw std::logic_error("No shared corner has clear ladder landing");
        }

        const int choice = random_int(
            rng, 0, static_cast<int>(candidates.size()) - 1);

        return candidates[choice];
    }

    // Ensure space is clear before interiors are built.
    // Specifically things like flowers grass, etc will be removed.
    // Using fill instead of setBlock because some legacy blocks end up ommited
    void clear_room_interiors(
        const FloorLayout& layout, int floor_y, mcpp::MinecraftConnection& mc
    ) {
        PerformanceStage timer("floor.clear_room_interiors");
        for (const Room& room : layout.rooms) {
            const std::string command = "fill " +
                std::to_string(room.min_x) + " " +
                std::to_string(floor_y + 1) + " " +
                std::to_string(room.min_z) + " " +
                std::to_string(room.max_x - 1) + " " +
                std::to_string(floor_y + floor_height - 2) + " " +
                std::to_string(room.max_z - 1) + " minecraft:air replace";

            profiled_command(mc, command);
            performance_profile().add("furniture.rooms_cleared");
        }
    }

    void render_surface(
        const Plot& plot,
        int y,
        mcpp::MinecraftConnection& mc
    ) {
        const mcpp::Coordinate origin = plot.getOrigin();
        const mcpp::Coordinate start = {
            origin.x + 1,
            y,
            origin.z + 1};
        const mcpp::Coordinate end = {
            plot.get_Max_X() - 2,
            y,
            plot.get_Max_Z() - 2};

        profiled_mcpp("mcpp_set_blocks", [&]() {
            mc.setBlocks(start, end, mcpp::Blocks::SPRUCE_WOOD_PLANK);
        });
    }

    void render_ladder(
        const Plot& plot,
        const prefabs::FloorAccess& access,
        mcpp::MinecraftConnection& mc
    ) {
        PerformanceStage timer("connector.render");
        const int first_y = plot.getOrigin().y + 1;
        const int last_y = plot.getOrigin().y +
            (plot.getFloors() - 1) * floor_height + 1;
        const mcpp::Coordinate start = {
            access.shaft_x, first_y, access.shaft_z};
        const mcpp::Coordinate end = {
            access.shaft_x, last_y, access.shaft_z};

        profiled_mcpp("mcpp_set_blocks", [&]() {
            mc.setBlocks(start, end, mcpp::Blocks::AIR);
        });
        const std::string facing = access.shaft_z == plot.getOrigin().z + 1
            ? "south" : "north";
        const std::string command = "fill " +
            std::to_string(start.x) + " " + std::to_string(start.y) + " " +
            std::to_string(start.z) + " " + std::to_string(end.x) + " " +
            std::to_string(end.y) + " " + std::to_string(end.z) +
            " minecraft:ladder[facing=" + facing +
            ",waterlogged=false] replace";
        profiled_command(mc, command);
        performance_profile().add("vertical_connections", plot.getFloors() - 1);
    }

}

std::size_t build_house(
    Plot& plot, mcpp::MinecraftConnection& mc,
    std::mt19937& rng, bool debug,
    const std::vector<Plot>& all_plots, const VillageBounds& bounds
) {
    if (plot.getFloors() < 1) {
        throw std::invalid_argument("Cannot build an empty plot");
    }
    const auto& catalogue = furniture_catalogue();
    const mcpp::Coordinate origin = plot.getOrigin();
    const Room interior = {
        origin.x + 1, origin.z + 1,
        plot.get_Max_X() - 1, plot.get_Max_Z() - 1
    };
    std::vector<FloorLayout> layouts = {};
    for (int floor = 0; floor < plot.getFloors(); ++floor) {
        PerformanceStage timer("floor.plan");
        FloorLayout layout = subdivide_floor(interior, rng);
        allocate_internal_doors(layout, rng);
        if (floor == 0) {
            allocate_exterior_entrance(
                plot, layout, rng, all_plots, bounds, mc);
        }
        performance_profile().add("rooms_generated", layout.rooms.size());
        performance_profile().add("internal_walls", layout.walls.size());
        performance_profile().add("internal_doors", layout.doors.size());
        layouts.push_back(layout);
    }
    prefabs::FloorAccess access = {};
    if (plot.getFloors() > 1) {
        access = plan_ladder(plot, layouts, rng);
    }
    build_floor(plot, mc);

    render_exterior(plot, mc);

    for (int floor = 0; floor < plot.getFloors(); ++floor) {
        PerformanceStage timer("floor.render");
        const int floor_y = origin.y + floor * floor_height;

        // Limit the timer's scope-to-surface rendering.
        // Leaving this block destroys the timer and records the elapsed time.
        {
            PerformanceStage surface_timer("floor.surfaces");

            if (floor > 0) {
                render_surface(plot, floor_y, mc);
            }

            render_surface(plot, floor_y + floor_height - 1, mc);
        }

        clear_room_interiors(layouts[floor], floor_y, mc);
        render_interior(layouts[floor], floor_y, mc);
        performance_profile().add("floors_built");
    }

    if (access.reserved_area.has_value()) {
        render_ladder(plot, access, mc);
    }

    std::size_t rooms_furnished = 0;
    for (int floor = 0; floor < plot.getFloors(); ++floor) {
        access.floor_index = floor;
        debug_log(debug, "Furnishing floor " + std::to_string(floor + 1));
        const prefabs::FurnishingReport report = prefabs::furnish_planned_rooms(
            layouts[floor], plot, catalogue, rng, mc, debug, access);
        if (!report.failed_rooms.empty()) {
            throw std::runtime_error("Floor " + std::to_string(floor + 1) +
                " furnishing incomplete: " +
                std::to_string(report.failed_rooms.size()) + " of " +
                std::to_string(report.rooms_total) + " rooms unresolved");
        }
        rooms_furnished += report.furnished.size();
    }
    return rooms_furnished;
}

void build_buildings(
    std::vector<Plot>& plots, mcpp::MinecraftConnection& mc,
    std::mt19937& rng, bool debug, const VillageBounds& bounds
) {
    std::size_t rooms_furnished = 0;
    for (std::size_t i = 0; i < plots.size(); ++i) {
        rooms_furnished += build_house(
            plots[i], mc, rng, debug, plots, bounds);
        performance_profile().add("houses_built");
        progress_log("Houses completed: " +
            std::to_string(i + 1) + "/" + std::to_string(plots.size()) +
            " (" + std::to_string(rooms_furnished) +
            " room furnishings submitted)");
    }
}
