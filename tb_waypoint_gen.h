#ifndef TB_WAYPOINT_GEN_H
#define TB_WAYPOINT_GEN_H

#include <vector>
#include <mcpp/mcpp.h>

#include "plots.h"
#include "tb_waypoint.h"
#include "tb_homemade.h"

void quickSortWaypoints(std::vector<Waypoint>& waypoints, int low, int high);

void connectWaypoints(
    int x1, int z1, int x2, int z2, 
    std::vector<Waypoint>& waypoints, 
    bool* path_map, 
    Plot* cs_plots, 
    int plot_amount, 
    mcpp::BlockType block,
    bool debug,
    const int surface_y
);

void waypointPlatforms(
    const std::vector<Waypoint>& waypoints, 
    const int surface_y);

#endif