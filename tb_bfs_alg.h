#ifndef tb_bfs_alg
#define tb_bfs_alg

#include <mcpp/mcpp.h>
#include <cstddef>

#include "tb_bfs_node.h"
#include "plots.h"
#include "tb_waypoint.h"
#include "tb_homemade.h"

std::size_t bfs_failed_attempts();



void bfs_path(int x1, int z1, int x2, int z2, int xS,
    int zS, int xT, int zT, Plot *cs_plots, int plot_amount,
    bool* path_map, mcpp::BlockType block, const int surface_y);

bool* path_viability_check(int x1, int z1, int x2, int z2, const int surface_y);

#endif
