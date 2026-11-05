#ifndef tb_house_grouping
#define tb_house_grouping

#include <vector>
#include <iostream>

#include "tb_waypoint.h"
#include "plots.h"

std::vector<Waypoint> group_plots(Plot *cs_plots, int plot_amount, int x1, int z1,
                                  int x2, int z2, const bool *path_map, const int surface_y);
void connectHouses(int x1, int z1, int x2, int z2, Waypoint wp, bool* path_map, Plot* cs_plots, int plot_amount, mcpp::BlockType block, bool debug, const int surface_y);



#endif