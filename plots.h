#ifndef PLOTS_H
#define PLOTS_H

#include <mcpp/mcpp.h>
#include <optional>

inline constexpr int floor_height = 6;

// Direction the entrance faces (facing outward from the house central origin)
enum class Direction {
    NORTH,
    SOUTH,
    EAST,
    WEST
};

// Stores the location and orientation of a house entrance
struct Entrance {
    mcpp::Coordinate position;
    Direction facing;
};

class Plot {
private:
    mcpp::Coordinate origin;

    int width;
    int depth;
    int floors;

    std::optional<Entrance> entrance;

public:
    Plot();

    Plot(
        mcpp::Coordinate origin, int plot_width, int plot_depth, int floor_count
    );
    int dist;

    // Getters
    mcpp::Coordinate getOrigin() const;
    int getWidth() const;
    int getDepth() const;
    int getFloors() const;
    int getBuildingHeight() const;

    // First Y coordinate outside the plot
    int get_Max_Y() const;
    // First X coordinate outside the plot
    int get_Max_X() const;
    // First Z coordinate outside the plot
    int get_Max_Z() const;

    // The entrance remains empty until building placement sets it.
    void setEntrance(mcpp::Coordinate position, Direction facing);

    std::optional<Entrance> getEntrance() const;
};

#endif