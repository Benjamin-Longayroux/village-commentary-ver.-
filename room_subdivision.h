#ifndef ROOM_SUBDIVISION_H
#define ROOM_SUBDIVISION_H

#include <random>
#include <vector>
#include <cstddef>


// A one-block-thick partition wall. 'start' and 'end' are half-open.
// along_x: wall is at X = coordinate, running along Z.
// !along_x: wall is at Z = coordinate, running along X.
struct InteriorWall {
    bool along_x;
    int coordinate;
    int start;
    int end;
};

// An internal door connects two rooms through a dividing wall.
// along_x uses the same convention as InteriorWall
struct InteriorDoor {
    int x;
    int z;

    // Preserve wall orientation
    bool along_x;

    // Rooms connected by the doorway
    int room_a;
    int room_b;
};

// X and Z bounds are half open: [min_x, max_x), [min_z, max_z)
struct Room {
    int min_x;
    int min_z;
    int max_x;
    int max_z;

    int width() const {
        return max_x - min_x;
    }
    int depth() const {
        return max_z - min_z;
    }
};



struct FloorLayout {
    std::vector<Room> rooms;
    std::vector<InteriorWall> walls;
    std::vector<InteriorDoor> doors;
};



// Interior excludes the outer walls of the building
// Guarantees every leaf has 4 to 8 clear blocks in the X and Z direction
FloorLayout subdivide_floor(const Room& interior, std::mt19937& rng);


// Allocates one internal door for each pair of adjacent rooms.
// Must be called after recursive subdivision has completed.
void allocate_internal_doors(FloorLayout& layout, std::mt19937& rng);

#endif