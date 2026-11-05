// tb_house_grouping.cpp

#include <cstdlib>
#include <iostream>
#include <mcpp/mcpp.h>

#include "tb_waypoint.h"
#include "tb_homemade.h"
#include "tb_bfs_alg.h"
#include "plots.h"
#include "utils.h"

void quickSortPlots(Plot *cs_plots, int low, int high)
{
    if (low < high)
    {
        Plot pivot = cs_plots[high];
        int i = low - 1;

        int j = low;
        while (j <= high - 1)
        {
            if (cs_plots[j].dist < pivot.dist)
            {
                i++;
                Plot temp = cs_plots[i];
                cs_plots[i] = cs_plots[j];
                cs_plots[j] = temp;
            }
            j++;
        }
        Plot temp = cs_plots[i + 1];
        cs_plots[i + 1] = cs_plots[high];
        cs_plots[high] = temp;
        int pi = i + 1;

        quickSortPlots(cs_plots, low, pi - 1);
        quickSortPlots(cs_plots, pi + 1, high);
    }
    return;
}

bool isWaypointInsidePlot(int wx, int wz, const Plot &plot)
{
    bool is_inside = false;
    int origin_x = plot.getOrigin().x - 3;
    int origin_z = plot.getOrigin().z - 3;
    int max_x = plot.get_Max_X() + 2;
    int max_z = plot.get_Max_Z() + 2;

    if (wx >= origin_x && wx <= max_x && wz >= origin_z && wz <= max_z)
    {
        is_inside = true;
    }

    return is_inside;
}

static bool waypointFits(int x, int z, int x1, int z1, int x2, int z2,
                         const bool *path_map, const Plot *plots, 
                         int plot_amount)
{
    bool fits = true;
    if (x < x1 || x > x2 || z < z1 || z > z2)
    {
        fits = false;
    }

    if (fits)
    {
        const int depth = z2 - z1 + 1;
        if (path_map != nullptr && path_map[(x - x1) * depth + (z - z1)])
        {
            fits = false;
        }
    }

    if (fits)
    {
        int k = 0;
        while (k < plot_amount)
        {
            if (isWaypointInsidePlot(x, z, plots[k]))
            {
                fits = false;
            }
            k++;
        }
    }
    return fits;
}

std::vector<Waypoint> group_plots(Plot *cs_plots, int plot_amount, int x1, int z1,
                                  int x2, int z2, const bool *path_map, 
                                  const int surface_y)
{
    std::vector<Waypoint> waypoints;
    bool success = true;

    if (plot_amount == 0)
    {
        success = false;
    }

    if (success)
    {
        int i = 0;
        while (i < plot_amount)
        {
            Plot &p = cs_plots[i];
            int dx_val = ((p.get_Max_X() + p.getOrigin().x) / 2) - x2;
            int dz_val = ((p.get_Max_Z() + p.getOrigin().z) / 2) - z2;
            p.dist = (dx_val * dx_val) + (dz_val * dz_val);
            i++;
        }

        quickSortPlots(cs_plots, 0, plot_amount - 1);

        int index = 0;
        while (index < plot_amount)
        {
            int remaining = plot_amount - index;
            int take = 3;
            if (remaining < 3 && remaining > 0 && waypoints.empty())
            {
                take = remaining;
            }
            if (remaining < 3 && remaining > 0 && !waypoints.empty())
            {
                int j = index;
                while (j < plot_amount)
                {
                    waypoints.back().children.push_back(&cs_plots[j]);
                    index++;
                    j++;
                }
            }
            else
            {
                Waypoint wp(0, 0);
                int j = 0;
                while (j < take)
                {
                    Plot *p = &cs_plots[index + j];
                    wp.children.push_back(p);
                    j++;
                }
                waypoints.push_back(wp);
                index += take;
            }
        }

        for (Waypoint &wp : waypoints)
        {
            int aX = 0;
            int aZ = 0;
            for (Plot *p : wp.children)
            {
                aX += (p->getOrigin().x + p->get_Max_X()) / 2;
                aZ += (p->getOrigin().z + p->get_Max_Z()) / 2;
            }
            int div = wp.children.size();
            wp.x = aX / div;
            wp.z = aZ / div;

            if (!waypointFits(wp.x, wp.z, x1, z1, x2, z2, path_map, cs_plots, 
                              plot_amount))
            {
                // If the group centre is blocked, use the nearest valid cell.
                const int centre_x = wp.x;
                const int centre_z = wp.z;
                int best_distance = -1;
                int dx = 0;
                while (dx <= x2 - x1)
                {
                    int dz = 0;
                    while (dz <= z2 - z1)
                    {
                        const int x = x1 + dx;
                        const int z = z1 + dz;
                        const int distance = std::abs(x - centre_x) + 
                                             std::abs(z - centre_z);
                        
                        bool valid_candidate = true;
                        if (distance == 0 || 
                            (best_distance >= 0 && distance >= best_distance))
                        {
                            valid_candidate = false;
                        }

                        if (valid_candidate)
                        {
                            if (waypointFits(x, z, x1, z1, x2, z2, path_map, 
                                             cs_plots, plot_amount))
                            {
                                best_distance = distance;
                                wp.x = x;
                                wp.z = z;
                            }
                        }
                        dz++;
                    }
                    dx++;
                }
                if (best_distance < 0)
                {
                    waypoints.clear();
                    success = false;
                }
            }
        }
    }

    if (success)
    {
        mcpp::MinecraftConnection mc;
        for (const Waypoint &wp : waypoints) {
            mcpp::Coordinate wpC1(wp.x, surface_y + 3, wp.z);
            profiled_mcpp("mcpp_set_block", [&]()
                          { return mc.setBlock(wpC1, mcpp::Blocks::EMERALD_BLOCK); });
        }
    }

    return waypoints;
}

void connectHouses(int x1, int z1, int x2, int z2, Waypoint wp, bool *path_map, 
                   Plot *cs_plots, int plot_amount, mcpp::BlockType block, 
                   bool debug, const int surface_y)
{
    mcpp::MinecraftConnection mc;

    int child_count = wp.children.size();
    bool can_connect = true;
    if (child_count == 0)
    {
        can_connect = false;
    }

    if (can_connect)
    {
        debug_log(debug, "Connecting Houses...");
        int k = 0;
        while (k < child_count)
        {
            Plot *plot_ptr = wp.children[k];
            std::optional<Entrance> entry = plot_ptr->getEntrance();
            if (entry.has_value())
            {
                int xS = entry->position.x;
                int zS = entry->position.z;

                if (entry->facing == Direction::NORTH)
                {
                    zS -= 1;
                }
                else if (entry->facing == Direction::SOUTH)
                {
                    zS += 1;
                }
                else if (entry->facing == Direction::EAST)
                {
                    xS += 1;
                }
                else if (entry->facing == Direction::WEST)
                {
                    xS -= 1;
                }

                bfs_path(x1, z1, x2, z2, xS, zS, wp.x, wp.z, cs_plots, 
                         plot_amount, path_map, block, surface_y);
            }
            k++;
        }
    }

    return;
}