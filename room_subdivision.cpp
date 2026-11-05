#include <stdexcept>
#include <algorithm>

#include "room_subdivision.h"
#include "utils.h"

// Keep subdivision helpers private to this implementation file.
namespace {

    constexpr int min_room_size = 4;
    constexpr int max_room_size = 8;

    void subdivide_recursive(
        const Room& room,
        std::mt19937& rng,
        FloorLayout& layout
    ) {
        // Each split consumes one additional block for the dividing wall.
        const bool can_split_x = room.width() > max_room_size;
        const bool can_split_z = room.depth() > max_room_size;

        if (!can_split_x && !can_split_z) {
            layout.rooms.push_back(room);
        } else {
            bool split_x = can_split_x;

            if (can_split_x && can_split_z) {
                split_x = (random_int(rng, 0, 1) == 0);
            }

            if (split_x) {
                // Left: [min_x, wall_x); right: [wall_x + 1, max_x).
                const int wall_x = random_int(rng,
                    room.min_x + min_room_size,
                    room.max_x - min_room_size - 1);

                layout.walls.push_back(
                    {true, wall_x, room.min_z, room.max_z});
                subdivide_recursive(
                    {room.min_x, room.min_z, wall_x, room.max_z}, rng, layout);
                subdivide_recursive(
                    {wall_x + 1, room.min_z, room.max_x, room.max_z},
                    rng, layout);
            } else {
                // Near: [min_z, wall_z); far: [wall_z + 1, max_z).
                const int wall_z = random_int(rng,
                    room.min_z + min_room_size,
                    room.max_z - min_room_size - 1);

                layout.walls.push_back({false, wall_z, room.min_x, room.max_x});
                subdivide_recursive(
                    {room.min_x, room.min_z, room.max_x, wall_z}, rng, layout);
                subdivide_recursive(
                    {room.min_x, wall_z + 1, room.max_x, room.max_z},
                    rng, layout);
            }
        }
    }

    bool find_shared_wall(
        const Room& first_room,
        const Room& second_room,
        InteriorWall& shared_wall
    ) {
        bool adjacent = false;

        const bool first_room_is_left =
            static_cast<long long>(first_room.max_x) + 1 ==
            second_room.min_x;

        const bool second_room_is_left =
            static_cast<long long>(second_room.max_x) + 1 ==
            first_room.min_x;

        // Check for rooms separated by a wall along X.
        if (first_room_is_left || second_room_is_left) {
            shared_wall.along_x = true;

            if (first_room_is_left) {
                shared_wall.coordinate = first_room.max_x;
            } else {
                shared_wall.coordinate = second_room.max_x;
            }

            shared_wall.start = std::max(
                first_room.min_z, second_room.min_z);

            shared_wall.end = std::min(
                first_room.max_z, second_room.max_z);

            adjacent = shared_wall.start < shared_wall.end;
        } else {
            const bool first_room_is_near =
                static_cast<long long>(first_room.max_z) + 1 ==
                second_room.min_z;

            const bool second_room_is_near =
                static_cast<long long>(second_room.max_z) + 1 ==
                first_room.min_z;

            // Check for rooms separated by a wall along Z.
            if (first_room_is_near || second_room_is_near) {
                shared_wall.along_x = false;

                if (first_room_is_near) {
                    shared_wall.coordinate = first_room.max_z;
                } else {
                    shared_wall.coordinate = second_room.max_z;
                }

                shared_wall.start = std::max(
                    first_room.min_x, second_room.min_x);

                shared_wall.end = std::min(
                    first_room.max_x, second_room.max_x);

                adjacent = shared_wall.start < shared_wall.end;
            }
        }

        return adjacent;
    }

    bool wall_exists(
        const FloorLayout& layout,
        const InteriorWall& shared_wall
    ) {
        bool wall_found = false;

        // Check whether a generated wall contains the shared boundary.
        for (const InteriorWall& wall : layout.walls) {
            if (wall.along_x == shared_wall.along_x &&
                wall.coordinate == shared_wall.coordinate &&
                wall.start <= shared_wall.start &&
                wall.end >= shared_wall.end) {
                wall_found = true;
            }
        }

        return wall_found;
    }

    InteriorDoor create_internal_door(
        const InteriorWall& shared_wall,
        int first_room_index,
        int second_room_index,
        std::mt19937& rng
    ) {
        int min_door_position = shared_wall.start;
        int max_door_position = shared_wall.end - 1;

        const int shared_length = shared_wall.end - shared_wall.start;

        // Preserve one block at each end whenever possible
        if (shared_length >= 3) {
            ++min_door_position;
            --max_door_position;
        }

        const int door_coordinate = random_int(
            rng, min_door_position, max_door_position);

        int door_x = 0;
        int door_z = 0;

        // Determine the door coordinates from the wall orientation.
        if (shared_wall.along_x) {
            door_x = shared_wall.coordinate;
            door_z = door_coordinate;
        } else {
            door_x = door_coordinate;
            door_z = shared_wall.coordinate;
        }

        // Store the doorway and its connected room indices.
        InteriorDoor door{
            door_x,
            door_z,
            shared_wall.along_x,
            first_room_index,
            second_room_index
        };

        return door;
    }
}

FloorLayout subdivide_floor(const Room& interior, std::mt19937& rng) {
    if (interior.width() < min_room_size || interior.depth() < min_room_size) {
        throw std::invalid_argument("Interior must be at least 4x4");
    }

    FloorLayout layout;
    subdivide_recursive(interior, rng, layout);
    return layout;
}

void allocate_internal_doors(FloorLayout& layout, std::mt19937& rng) {
    layout.doors.clear();

    for (std::size_t first_index = 0;
         first_index < layout.rooms.size();
         ++first_index) {

        const Room& first_room = layout.rooms[first_index];

        for (std::size_t second_index = first_index + 1;
             second_index < layout.rooms.size();
             ++second_index) {

            const Room& second_room = layout.rooms[second_index];

            InteriorWall shared_wall{false, 0, 0, 0};

            const bool adjacent = find_shared_wall(
                first_room, second_room, shared_wall);

            if (adjacent) {
                // Verify that the shared boundary is a valid partition.
                if (!wall_exists(layout, shared_wall)) {
                    throw std::logic_error(
                        "Adjacent rooms lack an internal wall");
                }

                // Create and store the internal doorway.
                InteriorDoor door = create_internal_door(
                    shared_wall,
                    static_cast<int>(first_index),
                    static_cast<int>(second_index),
                    rng);

                layout.doors.push_back(door);
            }
        }
    }
}