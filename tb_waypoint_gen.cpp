#include <vector>
#include <mcpp/mcpp.h>
#include <iostream>

#include "utils.h"
#include "tb_waypoint_gen.h"
#include "tb_bfs_alg.h"

void quickSortWaypoints(std::vector<Waypoint>& waypoints, int low, int high) {
    bool can_sort = true;
    if (low >= high) {
        can_sort = false;
    }

    if (can_sort) {
        Waypoint pivot = waypoints[high];
        int i = low - 1;

        int j = low;
        while (j <= high - 1) {
            bool condition = (waypoints[j].dist < pivot.dist);
            if (condition) {
                i++;
                Waypoint temp = waypoints[i];
                waypoints[i] = waypoints[j];
                waypoints[j] = temp;
            }
            j++;
        }
        Waypoint temp = waypoints[i + 1];
        waypoints[i + 1] = waypoints[high];
        waypoints[high] = temp;
        int pi = i + 1;

        quickSortWaypoints(waypoints, low, pi - 1);
        quickSortWaypoints(waypoints, pi + 1, high);
    }

    return;
}

void connectWaypoints(int x1, int z1, int x2, int z2, 
                      std::vector<Waypoint>& waypoints, bool* path_map, 
                      Plot* cs_plots, int plot_amount, mcpp::BlockType block, 
                      bool debug, const int surface_y) 
{
    int size = static_cast<int>(waypoints.size());
    bool can_connect = true;
    if (size == 0) {
        can_connect = false;
    }

    if (can_connect) {
        int i = 0;
        while (i < size) {
            Waypoint& wp = waypoints[i];
            int dx_val = wp.x - x1;
            int dz_val = wp.z - z1; 
            wp.dist = (dx_val * dx_val) + (dz_val * dz_val);
            i++;
        }
        
        int high_index = static_cast<int>(waypoints.size()) - 1;
        quickSortWaypoints(waypoints, 0, high_index);

        debug_log(debug, "Connecting Waypoints...");
        
        int index_i = 0;
        while (index_i < size) {
            while (!waypoints.at(index_i).linked) {
                int index_j = index_i;
                while (index_j < size) {
                    bfs_path(
                        x1, z1, x2, z2, 
                        waypoints.at(index_i).x, 
                        waypoints.at(index_i).z, 
                        waypoints.at(index_j).x, 
                        waypoints.at(index_j).z, 
                        cs_plots, plot_amount, path_map, block, surface_y
                    );
                    index_j++;
                }
                waypoints.at(index_i).linked = true;
            }
            index_i++;
        }
    }

    return;
}

void waypointPlatforms(const std::vector<Waypoint>& waypoints, 
                       const int surface_y) 
{
    // Void function that places all of the waypoint structure except the emerald block
    //      Loops through to do this at all waypoints

    mcpp::MinecraftConnection mc;

    int wp_index = 0;
    int total_waypoints = static_cast<int>(waypoints.size());
    while (wp_index < total_waypoints) {
    //  Standard loop to go through all of the waypoints in vector

        const Waypoint& wp = waypoints[wp_index];
    //  Get the current waypoint 

        int x_offset = -1;
        while (x_offset <= 1) {
            int z_offset = -1;
            while (z_offset <= 1) {
    //  Create two loops to expand in the x/z direction from the waypoint
    //      This is to spawn the spruce blocks on all neighbouring blocks


                mcpp::Coordinate wp_coord(wp.x + x_offset, surface_y, 
                                          wp.z + z_offset);
    //  Define the coordinate for the current spruceblock to be placed

                profiled_mcpp("mcpp_set_block", [&]() { 
                    return mc.setBlock(wp_coord, 
                                       mcpp::Blocks::SPRUCE_WOOD_PLANK); 
                });
    // Place the spruce block

                z_offset++;
    //  Move in the z direction
            }
            x_offset++;
    //  Move in the x direction
        }
        
        mcpp::Coordinate center_coord(wp.x, surface_y, wp.z);
        profiled_mcpp("mcpp_set_block", [&]() { 
            return mc.setBlock(center_coord, 
                               mcpp::Blocks::REDSTONE_LAMP_ACTIVE); 
        });
    //  Place the redstone lamp in the centre,
    //      This is the exact coord of the waypoint 
        
        mcpp::Coordinate redstone_coord(wp.x, surface_y - 1, wp.z);
        profiled_mcpp("mcpp_set_block", [&]() { 
            return mc.setBlock(redstone_coord, 
                               mcpp::Blocks::REDSTONE_BLOCK); 
        });
    //  Place the redstone block under the lamp to power it

        wp_index++;
    //  Move to the next waypoint
    }

    return;
}