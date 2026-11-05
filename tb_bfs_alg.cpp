#include <mcpp/mcpp.h>
#include <iostream>

#include "plots.h"
#include "utils.h"
#include "tb_waypoint.h"
#include "tb_homemade.h"
#include "tb_bfs_alg.h"
#include "tb_bfs_queue.h"

namespace {
std::size_t failed_bfs_attempts = 0;
}  // namespace

std::size_t bfs_failed_attempts() {
    return failed_bfs_attempts;
}

bool* path_viability_check(int x1, int z1, int x2, int z2, 
                           const int surface_y) {
    mcpp::MinecraftConnection mc;

    int width = (x2 - x1) + 1;
    int depth = (z2 - z1) + 1;
    //  Calc the dimensions of the village
    //      Techincally since its a square only one of these is needed

    bool* obs = new bool[width * depth];
    //  Create an array of obstacles to be found
    //      True --> obstacle at the block
    //      False --> No obstacle

    mcpp::Coordinate corner_1(x1, surface_y, z1);
    mcpp::Coordinate corner_2(x2, surface_y + 1, z2);
    //  Define the coordinates of the village corners
    
    mcpp::Chunk map = mc.getBlocks(corner_1, corner_2);
    //  Get the chunk of blocks

    for (int x = x1; x <= x2; ++x) {
        for (int z = z1; z <= z2; ++z) {
    //  Iterate through every x & z
            
            int index = (x - x1) * depth + (z - z1);
    //  Get that block's index in the obstacle array

            int local_x = x - x1;
            int local_z = z - z1;
    //  Calculate the relative coordinates

            bool is_ground_valid = 
                (map.get(local_x, 0, local_z) == mcpp::Blocks::GRASS.id || 
                 map.get(local_x, 0, local_z) == mcpp::Blocks::DIRT.id);

    //  Check if the ground is a valid block

            bool is_air_above = 
                (map.get(local_x, 1, local_z) == mcpp::Blocks::AIR.id);
    //  Make sure there is only air above

            if (is_ground_valid && is_air_above) {
                obs[index] = false; 
    //  If both are true, there is no obstacle 

            } else {
                obs[index] = true;
    //  Otherwise there is an obstacle
            }
        }
    }
    
    bool* result_ptr = obs;
    return result_ptr; 
    //  Return the obstacle array
}

void bfs_path(int x1, int z1, int x2, int z2, int xS,
              int zS, int xT, int zT, Plot *cs_plots, int plot_amount,
              bool* path_map, mcpp::BlockType block, const int surface_y)
{
    PerformanceStage bfs_timer("paths.bfs_total");
    mcpp::MinecraftConnection mc;
    performance_profile().add("bfs_calls");
    std::uint64_t nodes_expanded = 0;
    std::uint64_t nodes_enqueued = 1;

    int outer_vis_size = x2 - x1 + 1;
    int inner_vis_size = z2 - z1 + 1;
    //  This is the length of the village in the x && z direction
    //  This is used for array generations.

    bool endpoints_valid = true;
    if (xS < x1 || xS > x2 || zS < z1 || zS > z2 ||
        xT < x1 || xT > x2 || zT < z1 || zT > z2) {
        ++failed_bfs_attempts;
        performance_profile().add("bfs_failed_routes");
        std::cerr << "Warning: Path endpoint is outside the village.\n";
        endpoints_valid = false;
    }

    if (endpoints_valid) {
        bool *visited = new bool[outer_vis_size * inner_vis_size];
    //  This array is used to keep track of what blocks have been visited
    //      the index will be set to true if visited,

        for (int i = 0; i < outer_vis_size * inner_vis_size; ++i)
        {
            visited[i] = false;
        }
    //  All blocks in visited are set to false (unvisited) as no blocks have been checked.
        
        int xD[] = {0, 0, -1, 1, 1, 1, -1, -1};
        int zD[] = {-1, 1, 0, 0, 1, -1, 1, -1};
    //  These arrays are used for expansion from a Node / block to its neighbours
    //      The numbers are kept in pairs across both arrays for the direction
    //  e.g. xD[0] means go 0 blocks in the Z, zD[0] means go -1 blocks in the Z
    //      Hence remove the pairs where both arrays are 1/-1 to remove diagonals. 


        BfsQueue queue;
    //  Queue item is created, 
    //      explained on the txt, but is just a linked list of Nodes. 

        Node *startN = new Node(xS, zS, nullptr, nullptr);
    //  Create the beginning Node at (xS, zS) with no parent and no successor.

        queue.push(startN);
    //  Add it to the queue so it is first in line,

        visited[(xS - x1) * inner_vis_size + (zS - z1)] = true;
    //  Sets the starting block to visited, but this is ultimately undone later
    //      So this is redundant and my mistake, mb. 

        Node *targetN = nullptr;
    //  Sets the target node as nothing because the target block hasnt been found. 

        int size = outer_vis_size;
        int inner_size = inner_vis_size;
    //  These ints are used for loop iteration
    //      They are just renamed versions of the array size ints,
    //      This was done for my own reading comprehension. 
    

        for (int i = 0; i < plot_amount; ++i)
    //  This iterates through all of the plots created,
    //      it then sets all occupied blocks to visited
    //      this is done because the algorithm ignores visited blocks.  

        {
            Plot h = cs_plots[i];
    //  Obtain the current house (h) being considered. 
            

            int hX1 = h.getOrigin().x;
            int hZ1 = h.getOrigin().z;
            int hX2 = h.get_Max_X() - 1;
            int hZ2 = h.get_Max_Z() - 1;
    //  This gets the corners of the house
    //      hX1, hZ1 being bottom left corner
    //      hX2, hZ2 being the top right corner

            hX1 -= 2;
            hZ1 -= 2;
            hX2 += 2;
            hZ2 += 2;
    //  The corners are then expanded upon by 2 blocks
    //      This creates the 2 block buffer
    //      Change the int to change the size of the buffer

            for (int x = hX1; x <= hX2; ++x)
            {
                for (int z = hZ1; z <= hZ2; ++z)
                {
    //  Nested loop that goes through all the blocks along the X within the house
    //      Then all the blocks along the Z, inline with the current X value, within the two corners
                    if ((x - x1 >= 0 && z - z1 >= 0) && 
                        (x - x1 < size && z - z1 < inner_size))
                    {
    //  Make sure the Plot X/Z and the buffer zone is within village bounds
    //      This ensures when visited is indexed, it wont be an error

                        visited[(x - x1) * inner_vis_size + (z - z1)] = true;
    //  Mark the current block as visited, to remove it from consideration.
                    }
                }
            }
        }

        if (xT - x1 >= 0 && xT - x1 < size && 
            zT - z1 >= 0 && zT - z1 < inner_size)
        {
            visited[(xT - x1) * inner_vis_size + (zT - z1)] = false;
    //  Makse sure the target block is unvisited
    //      Hence why the line earlier is redundant
        }
        if (xS - x1 >= 0 && xS - x1 < size && 
            zS - z1 >= 0 && zS - z1 < inner_size)
        {
        visited[(xS - x1) * inner_vis_size + (zS - z1)] = false;
    //  Makse sure the starting block is unvisited
        }

        int w[] = {xS, zS, xT, zT};
    //  This is an array of the starting coords and target coords to be iterated through
        
        for (int i = 0; i <= 2; i += 2)
    //  Iterates by 2 because the w array is indexed in pairs (xS, zS) && (xT, zT)
        {
            for (int d = 0; d < 8; ++d)
    //  This then iterates through all the neighbouring blocks
    //      to mark them as unvisited.
    //      This makes sure the path can expand from the start.
            {
                int xB = w[i] + xD[d];
    //  Gets the X coord to be expanded from, using the direction array from earlier
                int zB = w[i + 1] + zD[d];
    //  Gets the Z coord to be expanded from, using the direction array from earlier
    
                if ((xB >= x1 && xB <= x2) && (zB >= z1 && zB <= z2))
                {
    //  Make sure the X is in bounds
                    if (xB - x1 >= 0 && xB - x1 < size && 
                        zB - z1 >= 0 && zB - z1 < inner_size)
                    {
    //  Make sure the Z is in bounds
                        visited[(xB - x1) * inner_vis_size + (zB - z1)] = false;
    //  Mark the neighbouring block as unvisited
                    }
                }
            }
        }

        {
            PerformanceStage search_timer("paths.bfs_search");
            while (!queue.isEmpty())
    //  While theres Nodes in the queue
    //      There will be no Nodes left in the queue when all
    //      viable Nodes have been already checked
            {
                Node *curr = queue.pop();
    //  Gets the Node from the front of the line
    //      And removes it from the line

                ++nodes_expanded;
                if (curr->x == xT && curr->z == zT)
    //  If the x/z coords of the Node currently considered is the target
                {
                    targetN = curr;
    //  Then the target Node has been found
    //  And nothing has been added to the Queue
    //      This may lead to residual Nodes continuing to being checked
    //      After the target as been found.
    //      A break should go here, or some kind of loop end.
                }
                else
    //  If the target isnt found
                {
                    for (int d = 0; d < 8; ++d)
    //  Expand in all directions to the neighbours
    //      Change 8 to be the length of the direction array if you change the direction arrays
                    {
                        int xN = curr->x + xD[d];
    //  xN (x neighbour) = the current Node's X + wtv offset its on

                        int zN = curr->z + zD[d];
    //  zN (z neighbour) = the current Node's Z + wtv offset its on
    
                        if ((xN >= x1 && xN <= x2) && (zN >= z1 && zN <= z2))
                        {
    //  If the x/z of the neighbour is in bounds, then check further

                            int vIndex = (xN - x1) * inner_vis_size + 
                                         (zN - z1);
    //  Calculating its vIndex (visited index) to index the visited array

                            if (!visited[vIndex] && !path_map[vIndex])
    //  Check that the block is unvisited and not an obstacle
                            {
                                visited[vIndex] = true;
    //  Mark it as visited
                                Node *neighbour = new Node(xN, zN, curr, nullptr);
    //  Create the Node for this block as it is valid, setting the parent 
                                queue.push(neighbour);
    //  Add it to the queue to be expanded upon later when its turn comes 
                                ++nodes_enqueued;
                            }
                        }
                    }
                }
            }
        }  
        // Standard BFS algorithm adding unvisited neighbouring nodes.

        int *result_array = nullptr;
    //  The an array of the path coordinates.
    //  The 0th index is the amnt of Nodes.
    //  Every odd index is the x-val
    //  Every even index >0 is the z-val
    //      This can be factored out now 
    //      as it was mostly used to avoid path intersection


        if (targetN != nullptr)
    //  If the target Node exists  
        {
            int max_cells = outer_vis_size * inner_vis_size;
    //  The maximum possible amount of nodes that couldve been created

            Node **Mpath = new Node *[max_cells];
    //  An array of the Nodes on path,
    //      This could also be factored out

            int mpath_size = 0;
    //  The size of the array for iteration


    //  The next portion of the code starts at the target Node and goes through
    //      Through the chain of parent Nodes until the beginning is found    

            Node *backtrack = targetN;
    //  backtrack is the Node currently selected in the chain
    //      It starts with the target then goes to its parent Node

            while (backtrack != nullptr)
    //  Goes until there are no parent Nodes left, 
    //      therefore the start of the chain / path
            {
                Mpath[mpath_size++] = backtrack;
    //  Add the current Node to the array of Nodes

                backtrack = backtrack->parent;
    // Backtrack becomes the next parent
            }

            result_array = new int[1 + (mpath_size * 2)];
    //  Create the result array to be the amount of x/z coords for each Node
    //      +1 for the amnt of Nodes at the 0 index

            result_array[0] = mpath_size;
    //  Set the 0 index to be the amount of Nodes

            int index = 1;
    //  Create an index to be counting forward when adding to the result array

            {
                PerformanceStage render_timer("paths.bfs_render");
                for (int i = mpath_size - 1; i >= 0; --i)
    //  Iterate backwards through the Node array,
    //      This is done because the path was added backwards,
    //      Starting from the target and going back to the start
                {
                    Node *N = Mpath[i];
    //  Get the Node from the array of Nodes

                    result_array[index++] = N->x;
                    result_array[index++] = N->z;
    //  Save the x/z coords of the Node to the index starting from the start

                    mcpp::Coordinate Mcoord(N->x, surface_y, N->z);
    //  Define the coordinate which the block will be placed at

                    profiled_mcpp("mcpp_set_block", [&]() { 
                        return mc.setBlock(Mcoord, block); 
    //  Place the block
                    });
                }
            }  
            for (int i = 0; i < mpath_size; ++i) {
                delete Mpath[i];
    //  Delete all the Nodes created
    //      SHOOT THIS IS WHERE THE MEMORY LEAK IS 
    //      ONLY THE NODES USED IN THE PATH ARE CREATED, 
    //      NOT ALL NODES CREATED ARE DELETED
    //      I suppose a second array of ALL Nodes would need to be kept and deleted

            }
            delete[] Mpath;
    //  Delete the Node array
        }
        else
        {
            performance_profile().add("bfs_failed_routes");
            ++failed_bfs_attempts;
            std::cerr << "Warning: No path found from (" << xS << ", " << zS
                      << ") to (" << xT << ", " << zT << ").\n";
            result_array = new int[1];
            result_array[0] = 0;
        }

        performance_profile().add("bfs_nodes_expanded", nodes_expanded);
        performance_profile().add("bfs_nodes_enqueued", nodes_enqueued);
        if (targetN != nullptr)
        {
            performance_profile().add("bfs_successful_routes");
            performance_profile().add("path_blocks_placed", result_array[0]);
        }

        delete[] visited;
        delete[] result_array;
    //  Delete the other arrays
    }

    return;
}