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
    //  Standard quicksort algorithm
    //  Used to sort plots based on their proximity to the bottom left corner
    //  This is used so that the closest houses can be grouped

    if (low < high)
    {
    //  If the lowest index is lower than the higest, it still needs sorting
        
        Plot pivot = cs_plots[high];
    //  All of the plots are organised in proportion to the distance of
    //      the plot at the last index

        int i = low - 1;
    //  i is set to one less than the lowest index 
    //      so that it can be incremented at the start of the loop
    //  This index is used to track where the elements lower than the pivot will go
    

        int j = low;
    //  j is the index which will move through the array
    //      if the plot at this index, itll be moved back behind the pivot
        while (j <= high - 1)
        {
    // Goes until the highest set index

            if (cs_plots[j].dist < pivot.dist)
            {
    //  If the currently iterated plot is less than the pivot

            
                i++;
    //  The index which the j plot will be moved back to is moved along

                Plot temp = cs_plots[i];
                cs_plots[i] = cs_plots[j];
                cs_plots[j] = temp;
    //  j and i are then swapped
            }
            j++;
    //  The next plot is then iterated to for comparison
        }

        Plot temp = cs_plots[i + 1];
        cs_plots[i + 1] = cs_plots[high];
        cs_plots[high] = temp;
    //  The pivot is then put to the index after the i value
    //      this is done as this is the first index greater than the pivot
    //      as it remained unmoved 

        int pi = i + 1;
        //  the pi (pivot index) is then calculated

        quickSortPlots(cs_plots, low, pi - 1);
    //  Plots to the left of the pivot are then sorted in the same way

        quickSortPlots(cs_plots, pi + 1, high);
    //  Plots to the right of the pivot are then sorted in the same way
    }
    return;
}

bool isWaypointInsidePlot(int wx, int wz, const Plot &plot)
{
    //  This function returns whether the currently proposed
    //      waypoint coordinites are within the currently passed plot

    bool is_inside = false;
    //  By default is is not inside a plot
    //      will be updated is a plot its inside is found

    int origin_x = plot.getOrigin().x - 3;
    int origin_z = plot.getOrigin().z - 3;
    //  Bottom left corner of plot with buffer is found
    
    int max_x = plot.get_Max_X() + 2;
    int max_z = plot.get_Max_Z() + 2;
    //  Top right corner of plot with buffer is found

    if (wx >= origin_x && wx <= max_x && wz >= origin_z && wz <= max_z)
    {
    //  If the coords are within the bounds of the plot 

        is_inside = true;
    //  It is within the plot
    }

    return is_inside;
    //  Return whether it is
}

static bool waypointFits(int x, int z, int x1, int z1, int x2, int z2,
                         const bool *path_map, const Plot *plots, 
                         int plot_amount)
    //  Function that returns whether the proposed waypoint coords are valid
{
    bool fits = true;
    //  Default set as allowed unless a condition fails
    
    if (x < x1 || x > x2 || z < z1 || z > z2)
    {

        fits = false;
    //  Check if it is out of village bounds
    }

    if (fits)
    {
        const int depth = z2 - z1 + 1;
        if (path_map != nullptr && path_map[(x - x1) * depth + (z - z1)])
        {
            fits = false;
    //  Check that it is not on top of an obstacle
        }
    }

    if (fits)
    {
        int k = 0;
        while (k < plot_amount)
        {
    //  Loop through all of the plots

            if (isWaypointInsidePlot(x, z, plots[k]))
            {
    
                fits = false;
    //  Check that it is not inside of a plot
            }
            k++;
        }
    }
    return fits;
    // Return whether any of the condisitions were failed
}

std::vector<Waypoint> group_plots(Plot *cs_plots, int plot_amount, int x1, int z1,
                                  int x2, int z2, const bool *path_map, 
                                  const int surface_y)
{
    //  Function that calculates what plots should be grouped together to a waypoint

    std::vector<Waypoint> waypoints;
    // Intialise the vector of waypoints

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
    //  For all plots 
            Plot &p = cs_plots[i];

            int dx_val = ((p.get_Max_X() + p.getOrigin().x) / 2) - x2;
            int dz_val = ((p.get_Max_Z() + p.getOrigin().z) / 2) - z2;
    //  Calculate its distance from the bottom left corner 

            p.dist = (dx_val * dx_val) + (dz_val * dz_val);
    //  Set the plot's distance attribute 

            i++;
    //  Move to the next plot
        }

        quickSortPlots(cs_plots, 0, plot_amount - 1);
    //  Quick sort the plots 


    //  The following portion of code makes the plot groups
    //  If there are less than 3 remaining, 
    //      They are either added to a new waypoint 
    //      Or they are tacked on to the previous waypoints
    //  If there are more than 3 remaning,
    //      then 3 are taken and put in a group

        int index = 0;
        while (index < plot_amount)
        {
            int remaining = plot_amount - index;
    //  Calculate how many plots are left

            int take = 3;
    //  Set how many plots are going to be taken 

            if (remaining < 3 && remaining > 0 && waypoints.empty())
            {
    //  if there are less than three plots left and no waypoints have been created
    //      This means there are only 1 or 2 total plots

                take = remaining;
    //  Then the amount of plots to be grouped is how many there actually are

            }
            if (remaining < 3 && remaining > 0 && !waypoints.empty())
            {
    //  If there are less then 3 left and other waypoints exist 

                int j = index;
                while (j < plot_amount)
                {
    //  Then go through all the plots left and add it to the last waypoint made

                    waypoints.back().children.push_back(&cs_plots[j]);
    //  Add the current plot to the last waypoint made

                    index++;
                    j++;

                }
            }
            else
            {
    //  If there are more than 3 remaning

                Waypoint wp(0, 0);
    //  Intialise a waypoint 

                int j = 0;
                while (j < take)
                {
    //  For all of the waypoints to be taken 

                    Plot *p = &cs_plots[index + j];
                    wp.children.push_back(p);
                    j++;
    //  Add the next plot to the waypoint's children
                }
                waypoints.push_back(wp);
    //  Add this waypoint to the vector of waypoints

                index += take;
    //  Add the amount of plots just added to the counter
            }
        }

    //  The following section of code calculates the coordinate position for each waypoint
    //      It starts at the centre of all of the houses in the group,
    //      Then moves it around until it finds a valid spot 

        for (Waypoint &wp : waypoints)
        {
    //  Loops through all of the waypoints made

            int aX = 0;
            int aZ = 0;
    //  Intialise the average X (aX) and average Z (aZ) of the plots in the group
            for (Plot *p : wp.children)
            {
            
                aX += (p->getOrigin().x + p->get_Max_X()) / 2;
                aZ += (p->getOrigin().z + p->get_Max_Z()) / 2;

    //  For all of the plots in the waypoint's children, add the center position to the aX, aZ
            }
            int div = wp.children.size();
    //  Determine how much the running totals need to be divided by 
    //  to turn it into the average

            wp.x = aX / div;
            wp.z = aZ / div;
    //  Set the waypoint x/z to be the average value 
    //      which is total x & z divided by the amount of plots

            if (!waypointFits(wp.x, wp.z, x1, z1, x2, z2, path_map, cs_plots, 
                              plot_amount))
            {
    //  Still for every waypoint, 
    //      if tghe position at the center of the group is invalid

                const int centre_x = wp.x;
                const int centre_z = wp.z;
    //  Stores the original x/z coords

                int best_distance = -1;
    //  Intialises the amount of blocks the waypoint has shifted 

                int dx = 0;
                while (dx <= x2 - x1)
                {
                    int dz = 0;
                    while (dz <= z2 - z1)
                    {
    //  Moving through every block within the village

                        const int x = x1 + dx;
                        const int z = z1 + dz;
    //  Shift the x and z values up by wtv increment the loop is upto 

                        const int distance = std::abs(x - centre_x) + 
                                             std::abs(z - centre_z);
    //  Calculate how far the waypoint is from the start
                        
                        bool valid_candidate = true;
                        if (distance == 0 || 
                            (best_distance >= 0 && distance >= best_distance))
                        {
                            valid_candidate = false;
    //  If the distance is 0, or the current position is not better then the best found
                        }

                        if (valid_candidate)
    //  If the distance is valid
                        {
                            if (waypointFits(x, z, x1, z1, x2, z2, path_map, 
                                             cs_plots, plot_amount))
                            {
    //  Check if the waypoint is able to fit there 

                                best_distance = distance;
    //  Then the new best distance has been found 

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
    //  If the best distance remained -1, 
    //      Then no viable location was found
                    waypoints.clear();
                    success = false;
                }
            }
        }
    }

    if (success)
    {
    //  If all waypoints found a location
    //      Spawn the emerald block

        mcpp::MinecraftConnection mc;
        for (const Waypoint &wp : waypoints) {
            mcpp::Coordinate wpC1(wp.x, surface_y + 3, wp.z);
            profiled_mcpp("mcpp_set_block", [&]()
                          { return mc.setBlock(wpC1, mcpp::Blocks::EMERALD_BLOCK); });
        }
    }

    return waypoints;
    //  Return the vector of generated waypoints
}

void connectHouses(int x1, int z1, int x2, int z2, Waypoint wp, bool *path_map, 
                   Plot *cs_plots, int plot_amount, mcpp::BlockType block, 
                   bool debug, const int surface_y)
{
    //  Void function that iterates through all of the children plots
    //      and spawns the path connecting each child to the waypoint passed
    mcpp::MinecraftConnection mc;

    int child_count = wp.children.size();
    //  Find the amount of children

    bool can_connect = true;
    if (child_count == 0)
    {
        can_connect = false;
    }

    if (can_connect)
    {
    // If there are a nonzero amount of children

        debug_log(debug, "Connecting Houses...");
        int k = 0;
        while (k < child_count)
        {
            Plot *plot_ptr = wp.children[k];
    //  Iterate through every plot child
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
    //  Find the block infront of the door

                bfs_path(x1, z1, x2, z2, xS, zS, wp.x, wp.z, cs_plots, 
                         plot_amount, path_map, block, surface_y);
    //  Spawn the path connecting the block infront of the door to the waypoint
            }
            k++;
    //  Move to the next child
        }
    }

    return;
}