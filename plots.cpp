#include <stdexcept>
#include <cstdint>
#include <limits>

#include "plots.h"


Plot::Plot()
    : origin(0, 0, 0),
      width(0),
      depth(0),
      floors(0),
      entrance(std::nullopt),
      dist(-1) {}

Plot::Plot(
    mcpp::Coordinate origin,
    int plot_width,
    int plot_depth,
    int floor_count
)
    : origin(origin),
      width(plot_width),
      depth(plot_depth),
      floors(floor_count),
      entrance(std::nullopt) {

    constexpr int min_plot_size = 16;
    constexpr int max_plot_size = 32;
    constexpr int min_floors = 1;
    constexpr int max_floors = 3;

    // Validate plot dimensions (16-32 blocks).
    if (width < min_plot_size || width > max_plot_size ||
        depth < min_plot_size || depth > max_plot_size) {
        throw std::invalid_argument(
            "Plot dimensions must be between 16 and 32.");
    }

    if (floors < min_floors || floors > max_floors) {
        throw std::invalid_argument(
            "Building must contain between 1 and 3 floors.");
    }
    this->dist = -1;

    // Prevent coordinate overflow (Avoid flipping from max int -> min int)
    // int maximum 2,147,483,647 + 1 = -2,147,483,648
    std::int64_t max_x = static_cast<std::int64_t>(origin.x) + width;
    std::int64_t max_z = static_cast<std::int64_t>(origin.z) + depth;
    std::int64_t max_y = static_cast<std::int64_t>(origin.y) +
        static_cast<std::int64_t>(floors) * floor_height;

    if (max_x > std::numeric_limits<int>::max() ||
        max_z > std::numeric_limits<int>::max() ||
        max_y > std::numeric_limits<int>::max()) {
        throw std::out_of_range("Plot coordinates exceed integer limits.");
    }
}

mcpp::Coordinate Plot::getOrigin() const {
    return origin;
}

int Plot::getWidth() const {
    return width;
}

int Plot::getDepth() const {
    return depth;
}

// Maximum boundaries (exclusive)
int Plot::get_Max_X() const {
    return origin.x + width;
}

int Plot::get_Max_Z() const {
    return origin.z + depth;
}

int Plot::getFloors() const {
    return floors;
}

int Plot::getBuildingHeight() const {
    return floors * floor_height;
}

int Plot::get_Max_Y() const {
    return origin.y + getBuildingHeight();
}

// Returns the entrance coordinates and facing direction
std::optional<Entrance> Plot::getEntrance() const {
    return entrance;
}

// Stores the entrance position and its outward-facing direction
void Plot:: setEntrance(mcpp::Coordinate position, Direction facing) {
    entrance = Entrance{position, facing};
}
