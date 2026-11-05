#include <mcpp/mcpp.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <optional>

#include "plot_finder.h"
#include "utils.h"

namespace {

constexpr int plot_separation = 2;

}

std::optional<int> find_ground_height(
    const Plot& candidate,
    mcpp::MinecraftConnection& mc
) {
    PerformanceStage timer("plots.ground_height_validation");
    std::optional<int> ground_height = std::nullopt;
    mcpp::Coordinate origin = candidate.getOrigin();
    mcpp::Coordinate2D start(origin.x, origin.z);
    mcpp::Coordinate2D end(
        candidate.get_Max_X() - 1,
        candidate.get_Max_Z() - 1
    );

    // Retrieve terrain heights before requesting surface blocks.

    mcpp::HeightMap heights = profiled_mcpp("mcpp_get_heights", [&]()
        { return mc.getHeights(start, end); });

    bool valid_terrain = heights.x_len() == candidate.getWidth() &&
        heights.z_len() == candidate.getDepth();

    int reference_height = 0;

    if (valid_terrain) {
        reference_height = heights.get(0, 0);

        // Stop inspecting heights as soon as uneven terrain is found.
        for (int x = 0; valid_terrain && x < heights.x_len(); x++) {
            for (int z = 0; valid_terrain && z < heights.z_len(); z++) {
                valid_terrain = heights.get(x, z) == reference_height;
            }
        }
    }

    // Invalid dimensions or uneven terrain must not trigger this read.
    if (valid_terrain) {
        mcpp::Coordinate block_start(origin.x, reference_height, origin.z);
        mcpp::Coordinate block_end(
            candidate.get_Max_X() - 1,
            reference_height,
            candidate.get_Max_Z() - 1
        );

        mcpp::Chunk surface = profiled_mcpp("mcpp_get_blocks", [&]()
            { return mc.getBlocks(block_start, block_end); });

        valid_terrain = surface.x_len() == candidate.getWidth() &&
            surface.y_len() == 1 &&
            surface.z_len() == candidate.getDepth();

        for (int x = 0; valid_terrain && x < surface.x_len(); x++) {
            for (int z = 0; valid_terrain && z < surface.z_len(); z++) {
                mcpp::BlockType block = surface.get(x, 0, z);
                valid_terrain = block.id == mcpp::Blocks::GRASS.id;
            }
        }

        if (valid_terrain) {
            ground_height = reference_height + 1;
        }
    }

    return ground_height;
}

bool check_collision(
    const Plot& candidate,
    const std::vector<Plot>& existing_plots
) {
    // Use wider arithmetic when expanding exclusive plot boundaries.
    std::int64_t candidate_min_x = candidate.getOrigin().x;
    std::int64_t candidate_min_z = candidate.getOrigin().z;
    std::int64_t candidate_max_x = candidate.get_Max_X();
    std::int64_t candidate_max_z = candidate.get_Max_Z();
    bool collision = false;

    PerformanceStage timer("plots.collision_validation");
    std::uint64_t comparisons = 0;
    for (std::size_t index = 0;
         !collision && index < existing_plots.size(); index++) {
        ++comparisons;
        const Plot& existing = existing_plots[index];
        std::int64_t existing_min_x = existing.getOrigin().x;
        std::int64_t existing_min_z = existing.getOrigin().z;
        std::int64_t existing_max_x = existing.get_Max_X();
        std::int64_t existing_max_z = existing.get_Max_Z();

        bool separated =
            existing_max_x + plot_separation <= candidate_min_x ||
            existing_max_z + plot_separation <= candidate_min_z ||
            candidate_max_x + plot_separation <= existing_min_x ||
            candidate_max_z + plot_separation <= existing_min_z;
        collision = !separated;
    }

    performance_profile().add("plot_collision_comparisons", comparisons);
    return collision;
}

bool check_obstacles(
    const Plot& candidate,
    mcpp::MinecraftConnection& mc,
    int ground_y
) {
    PerformanceStage timer("plots.obstacle_validation");

    std::int64_t min_x =
        static_cast<std::int64_t>(candidate.getOrigin().x) - plot_separation;
    std::int64_t min_z =
        static_cast<std::int64_t>(candidate.getOrigin().z) - plot_separation;

    // Convert exclusive bounds to inclusive Minecraft endpoints.
    std::int64_t max_x =
        static_cast<std::int64_t>(candidate.get_Max_X()) +
        (plot_separation - 1);
    std::int64_t max_z =
        static_cast<std::int64_t>(candidate.get_Max_Z()) +
        (plot_separation - 1);

    // Include the ground surface and full building height.
    std::int64_t min_y = static_cast<std::int64_t>(ground_y) - 1;
    std::int64_t max_y = static_cast<std::int64_t>(ground_y) +
        candidate.getBuildingHeight() - 1;

    auto valid_coordinate = [](std::int64_t value) {
        return value >= std::numeric_limits<int>::min() &&
               value <= std::numeric_limits<int>::max();
    };

    bool obstruction = !valid_coordinate(min_x) ||
        !valid_coordinate(min_z) || !valid_coordinate(max_x) ||
        !valid_coordinate(max_z) || !valid_coordinate(min_y) ||
        !valid_coordinate(max_y);

    // Reject invalid bounds before making a world request.
    if (!obstruction) {
        mcpp::Coordinate start(
            static_cast<int>(min_x),
            static_cast<int>(min_y),
            static_cast<int>(min_z)
        );
        mcpp::Coordinate end(
            static_cast<int>(max_x),
            static_cast<int>(max_y),
            static_cast<int>(max_z)
        );

        mcpp::Chunk volume = profiled_mcpp("mcpp_get_blocks", [&]()
            { return mc.getBlocks(start, end); });

        obstruction =
            volume.x_len() != candidate.getWidth() + 2 * plot_separation ||
            volume.y_len() != candidate.getBuildingHeight() + 1 ||
            volume.z_len() != candidate.getDepth() + 2 * plot_separation;

        for (int x = 0; !obstruction && x < volume.x_len(); x++) {
            for (int y = 0; !obstruction && y < volume.y_len(); y++) {
                for (int z = 0; !obstruction && z < volume.z_len(); z++) {
                    mcpp::BlockType block = volume.get(x, y, z);
                    bool allowed_block = block.id == mcpp::Blocks::AIR.id;

                    // Ground materials are allowed only at surface level.
                    if (!allowed_block && y == 0) {
                        allowed_block =
                            block.id == mcpp::Blocks::GRASS.id ||
                            block.id == mcpp::Blocks::DIRT.id ||
                            block.id == mcpp::Blocks::GRAVEL.id ||
                            block.id == mcpp::Blocks::SAND.id ||
                            block.id == mcpp::Blocks::GRASS_PATH.id;
                    }

                    if (!allowed_block) {
                        allowed_block =
                            block.id == mcpp::Blocks::TALL_GRASS.id ||
                            block.id == mcpp::Blocks::DEAD_BUSH.id ||
                            block.id == mcpp::Blocks::DANDELION.id ||
                            block.id == mcpp::Blocks::POPPY.id ||
                            block.id == mcpp::Blocks::SUNFLOWER.id;
                    }

                    obstruction = !allowed_block;
                }
            }
        }
    }

    return obstruction;
}

std::vector<Plot> find_plots(
    int loc_x,
    int loc_z,
    int village_size,
    std::mt19937& rng,
    bool debug,
    int density
) {
    constexpr int min_plot_size = 16;
    constexpr int plot_size_limit = 32;
    constexpr int village_size_limit = 10000;
    constexpr int min_floors = 1;
    constexpr int max_floors = 3;
    constexpr int village_blocks_per_plot = 50;
    constexpr int attempts_per_plot = 50;
    std::vector<Plot> plots;

    if (village_size < min_plot_size) {
        debug_log(debug, "Village too small to contain a valid plot.");
    } else if (village_size >= village_size_limit) {
        debug_log(debug, "Village size exceeds supported maximum (9999).");
    } else {
        int max_plot_size = std::min(plot_size_limit, village_size);

        // Calculate village boundaries using 64-bit arithmetic.
        std::int64_t min_x64 =
            static_cast<std::int64_t>(loc_x) - village_size / 2;
        std::int64_t min_z64 =
            static_cast<std::int64_t>(loc_z) - village_size / 2;
        std::int64_t max_x64 = min_x64 + village_size;
        std::int64_t max_z64 = min_z64 + village_size;

        if (min_x64 < std::numeric_limits<int>::min() ||
            min_z64 < std::numeric_limits<int>::min() ||
            max_x64 > std::numeric_limits<int>::max() ||
            max_z64 > std::numeric_limits<int>::max()) {
            throw std::out_of_range(
                "Village coordinates exceed integer limits!");
        }

        // Safe to convert to int after validation.
        int min_x = static_cast<int>(min_x64);
        int min_z = static_cast<int>(min_z64);
        int max_x = static_cast<int>(max_x64);
        int max_z = static_cast<int>(max_z64);

        mcpp::MinecraftConnection mc;

        int required_plots =
            std::max(1, village_size / village_blocks_per_plot);
        int target_plot_count = required_plots * density;
        int attempt = 0;
        int max_attempts = target_plot_count * attempts_per_plot;

        debug_log(debug, "Village Boundaries");
        debug_log(debug, "  X: [" +
            std::to_string(min_x) + ", " +
            std::to_string(max_x) + ")");
        debug_log(debug, "  Z: [" +
            std::to_string(min_z) + ", " +
            std::to_string(max_z) + ")");
        debug_log(debug, "Minimum required plots: " +
            std::to_string(required_plots));
        debug_log(debug, "Target plots: " +
            std::to_string(target_plot_count));

        // Draw a candidate, then validate collisions, terrain and obstacles.
        while (plots.size() < static_cast<std::size_t>(target_plot_count) &&
               attempt < max_attempts) {
            attempt += 1;
            performance_profile().add("plot_candidates");

            int size_x = random_int(rng, min_plot_size, max_plot_size);
            int size_z = random_int(rng, min_plot_size, max_plot_size);
            int plot_x = random_int(rng, min_x, max_x - size_x);
            int plot_z = random_int(rng, min_z, max_z - size_z);
            int floors = random_int(rng, min_floors, max_floors);
            mcpp::Coordinate position(plot_x, 0, plot_z);
            Plot candidate(position, size_x, size_z, floors);
            debug_log(debug, "Candidate " + std::to_string(attempt) +
                ": origin (" + std::to_string(plot_x) + ", " +
                std::to_string(plot_z) + "), size " +
                std::to_string(size_x) + "x" + std::to_string(size_z) +
                ", floors " + std::to_string(floors));

            if (check_collision(candidate, plots)) {
                performance_profile().add("plot_rejected_collision");
                debug_log(debug, "Plot Rejected: Collision detected.");
            } else {
                std::optional<int> ground_y =
                    find_ground_height(candidate, mc);

                if (ground_y.has_value()) {
                    if (!check_obstacles(candidate, mc, *ground_y)) {
                        mcpp::Coordinate ground_position(
                            plot_x, *ground_y-1, plot_z);
                        plots.emplace_back(
                            ground_position, size_x, size_z, floors);
                        performance_profile().add("plot_accepted");
                        const Plot& generated = plots.back();

                        debug_log(debug, "Plot accepted: origin (" +
                            std::to_string(plot_x) + ", " +
                            std::to_string(generated.getOrigin().y) + ", " +
                            std::to_string(plot_z) + "), size " +
                            std::to_string(size_x) + "x" +
                            std::to_string(size_z));
                        debug_log(debug, "Exclusive maximum X: " +
                            std::to_string(generated.get_Max_X()));
                        debug_log(debug, "Exclusive maximum Z: " +
                            std::to_string(generated.get_Max_Z()));
                        debug_log(debug, "Floors: " +
                            std::to_string(generated.getFloors()));
                        debug_log(debug, "Building Height: " +
                            std::to_string(generated.getBuildingHeight()));
                        debug_log(debug, "Exclusive maximum Y: " +
                            std::to_string(generated.get_Max_Y()));
                    } else {
                        performance_profile().add("plot_rejected_obstacle");
                        debug_log(debug,
                            "Plot rejected: obstruction detected.");
                    }
                } else {
                    performance_profile().add("plot_rejected_terrain");
                    debug_log(debug, "Plot Rejected: Invalid terrain.");
                }
            }
        }

        std::cout << "Plots found: " << plots.size() << "/"
                  << target_plot_count << " target (minimum required: "
                  << required_plots << ")"
                  << std::endl;

        if (plots.size() < static_cast<std::size_t>(required_plots)) {
            std::cerr << "Error: Found fewer than the minimum "
                      << required_plots << " plots; continuing with "
                      << plots.size()
                      << std::endl;
        }
        else if (plots.size() < static_cast<std::size_t>(target_plot_count)){
            std::cerr << "Warning: Target plot density was not reached;"
                      << "continuing with "
                      << plots.size()
                      << "plots." << std::endl;
        }
    }

    return plots;
}