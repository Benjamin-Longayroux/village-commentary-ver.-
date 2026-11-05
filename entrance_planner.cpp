#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "entrance_planner.h"
#include "utils.h"

namespace {

    constexpr int entrance_corner_clearance = 2;
    constexpr int exterior_approach_length = 3;

    // Both landing positions must be inside the same room.
    bool has_clear_landing(
        const FloorLayout& layout, int door_x, int door_z,
        int inward_x, int inward_z
    ) {
        const int first_x = door_x + inward_x;
        const int first_z = door_z + inward_z;
        const int second_x = first_x + inward_x;
        const int second_z = first_z + inward_z;
        bool clear = false;

        for (const Room& room : layout.rooms) {
            if (first_x >= room.min_x && first_x < room.max_x &&
                first_z >= room.min_z && first_z < room.max_z &&
                second_x >= room.min_x && second_x < room.max_x &&
                second_z >= room.min_z && second_z < room.max_z) {
                clear = true;
            }
        }
        return clear;
    }

    void direction_offset(Direction facing, int& dx, int& dz) {
        dx = 0;
        dz = 0;
        if (facing == Direction::NORTH) {
            dz = -1;
        } else if (facing == Direction::SOUTH) {
            dz = 1;
        } else if (facing == Direction::EAST) {
            dx = 1;
        } else {
            dx = -1;
        }
    }

    Direction opposite_direction(Direction facing) {
        Direction opposite = Direction::NORTH;
        if (facing == Direction::NORTH) {
            opposite = Direction::SOUTH;
        } else if (facing == Direction::SOUTH) {
            opposite = Direction::NORTH;
        } else if (facing == Direction::EAST) {
            opposite = Direction::WEST;
        } else {
            opposite = Direction::EAST;
        }
        return opposite;
    }

    // Gather interior-safe candidates before testing the exterior.
    void add_candidate(
        const FloorLayout& layout, std::vector<Entrance>& candidates,
        int x, int y, int z, Direction facing
    ) {
        int outward_x = 0;
        int outward_z = 0;
        direction_offset(facing, outward_x, outward_z);
        if (has_clear_landing(
                layout, x, z, -outward_x, -outward_z)) {
            candidates.push_back({mcpp::Coordinate(x, y, z), facing});
        }
    }

    bool inside_village(
        std::int64_t x, std::int64_t z, const VillageBounds& bounds
    ) {
        const bool inside = x >= bounds.min_x &&
            x < bounds.max_x_exclusive && z >= bounds.min_z &&
            z < bounds.max_z_exclusive;
        return inside;
    }

    // The third approach cell must escape other plots' BFS exclusion margins.
    bool blocked_by_plot(
        std::int64_t x, std::int64_t z,
        const std::vector<Plot>& plots, const Plot& owner, int step
    ) {
        bool blocked = false;
        for (const Plot& other : plots) {
            if (&other != &owner) {
                const int margin = step == exterior_approach_length ? 2 : 0;
                const std::int64_t min_x =
                    static_cast<std::int64_t>(other.getOrigin().x) - margin;
                const std::int64_t min_z =
                    static_cast<std::int64_t>(other.getOrigin().z) - margin;
                const std::int64_t max_x =
                    static_cast<std::int64_t>(other.get_Max_X()) + margin;
                const std::int64_t max_z =
                    static_cast<std::int64_t>(other.get_Max_Z()) + margin;
                if (x >= min_x && x < max_x &&
                    z >= min_z && z < max_z) {
                    blocked = true;
                }
            }
        }
        return blocked;
    }

    bool passable_block(const mcpp::BlockType& block) {
        const bool passable = block.id == mcpp::Blocks::AIR.id ||
            block.id == mcpp::Blocks::TALL_GRASS.id ||
            block.id == mcpp::Blocks::DEAD_BUSH.id ||
            block.id == mcpp::Blocks::DANDELION.id ||
            block.id == mcpp::Blocks::POPPY.id;
        return passable;
    }

    // Count consecutive clear positions outside an entrance, at foot/head Y.
    int approach_clearance(
        const Entrance& entrance, const Plot& plot,
        const std::vector<Plot>& plots, const VillageBounds& bounds,
        mcpp::Chunk& outside, int sample_min_x, int sample_min_z
    ) {
        int outward_x = 0;
        int outward_z = 0;
        direction_offset(entrance.facing, outward_x, outward_z);
        int clear = 0;
        for (int step = 1;
             step <= exterior_approach_length && clear == step - 1;
             ++step) {
            const std::int64_t x =
                static_cast<std::int64_t>(entrance.position.x) +
                step * outward_x;
            const std::int64_t z =
                static_cast<std::int64_t>(entrance.position.z) +
                step * outward_z;
            bool valid = inside_village(x, z, bounds) &&
                !blocked_by_plot(x, z, plots, plot, step);
            if (valid) {
                const int block_x = static_cast<int>(x) - sample_min_x;
                const int block_z = static_cast<int>(z) - sample_min_z;
                for (int height = 0; valid && height < 2; ++height) {
                    valid = passable_block(
                        outside.get(block_x, height, block_z));
                }
            }
            if (valid) {
                clear = step;
            }
        }
        return clear;
    }

    // Prefer the opposite wall, then other walls, then the original wall.
    int fallback_group(Direction first, Direction candidate) {
        int group = 1;
        if (candidate == opposite_direction(first)) {
            group = 0;
        } else if (candidate == first) {
            group = 2;
        }
        return group;
    }
}

void allocate_exterior_entrance(
    Plot& plot, const FloorLayout& layout, std::mt19937& rng,
    const std::vector<Plot>& plots, const VillageBounds& bounds,
    mcpp::MinecraftConnection& mc
) {
    const mcpp::Coordinate origin = plot.getOrigin();
    const int min_x = origin.x;
    const int min_z = origin.z;
    const int max_x = plot.get_Max_X() - 1;
    const int max_z = plot.get_Max_Z() - 1;
    const int door_y = origin.y + 1;
    std::vector<Entrance> candidates = {};

    for (int x = min_x + entrance_corner_clearance;
         x <= max_x - entrance_corner_clearance; ++x) {
        add_candidate(layout, candidates, x, door_y, min_z,
                      Direction::NORTH);
        add_candidate(layout, candidates, x, door_y, max_z,
                      Direction::SOUTH);
    }
    for (int z = min_z + entrance_corner_clearance;
         z <= max_z - entrance_corner_clearance; ++z) {
        add_candidate(layout, candidates, max_x, door_y, z,
                      Direction::EAST);
        add_candidate(layout, candidates, min_x, door_y, z,
                      Direction::WEST);
    }
    if (candidates.empty()) {
        throw std::logic_error("No entrance has a clear interior landing");
    }

    // One bulk world read, rather than a request for each candidate block.
    const int sample_min_x = static_cast<int>(
        std::max(static_cast<std::int64_t>(bounds.min_x),
            static_cast<std::int64_t>(min_x) - exterior_approach_length));
    const int sample_min_z = static_cast<int>(
        std::max(static_cast<std::int64_t>(bounds.min_z),
            static_cast<std::int64_t>(min_z) - exterior_approach_length));
    const int sample_max_x = static_cast<int>(
        std::min(static_cast<std::int64_t>(bounds.max_x_exclusive) - 1,
            static_cast<std::int64_t>(max_x) + exterior_approach_length));
    const int sample_max_z = static_cast<int>(
        std::min(static_cast<std::int64_t>(bounds.max_z_exclusive) - 1,
            static_cast<std::int64_t>(max_z) + exterior_approach_length));
    const mcpp::Coordinate sample_start(
        sample_min_x, door_y, sample_min_z);
    const mcpp::Coordinate sample_end(
        sample_max_x, door_y + 1, sample_max_z);
    mcpp::Chunk outside = profiled_mcpp("mcpp_get_blocks", [&]() {
        return mc.getBlocks(sample_start, sample_end);
    });
    if (outside.x_len() != sample_max_x - sample_min_x + 1 ||
        outside.y_len() != 2 ||
        outside.z_len() != sample_max_z - sample_min_z + 1) {
        throw std::runtime_error("Entrance clearance read size mismatch");
    }

    std::vector<int> clearance = {};
    for (const Entrance& candidate : candidates) {
        clearance.push_back(approach_clearance(
            candidate, plot, plots, bounds, outside,
            sample_min_x, sample_min_z));
    }

    // Preserve one seeded initial choice; the fallback is deterministic.
    const int first = random_int(
        rng, 0, static_cast<int>(candidates.size()) - 1);
    int selected = clearance[first] == exterior_approach_length ? first : -1;
    for (int required = exterior_approach_length;
         required >= 1 && selected < 0; --required) {
        for (int group = 0; group < 3 && selected < 0; ++group) {
            for (std::size_t offset = 1;
                 offset < candidates.size() && selected < 0; ++offset) {
                const int index = static_cast<int>(
                    (static_cast<std::size_t>(first) + offset) %
                    candidates.size());
                if (clearance[index] >= required &&
                    fallback_group(candidates[first].facing,
                                   candidates[index].facing) == group) {
                    selected = index;
                }
            }
        }
    }
    if (selected < 0 && clearance[first] > 0) {
        selected = first;
    }
    if (selected < 0) {
        throw std::logic_error("No exterior entrance has a legal approach");
    }

    performance_profile().add("entrance.approach_cells_clear",
                              clearance[selected]);
    if (selected != first) {
        performance_profile().add("entrance.relocated");
    }
    if (clearance[selected] < exterior_approach_length) {
        performance_profile().add("entrance.short_approach");
    }
    plot.setEntrance(candidates[selected].position,
                     candidates[selected].facing);
}
