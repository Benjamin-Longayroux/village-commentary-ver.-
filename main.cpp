// #### IMPORTANT ####
// Build using the Makefile so PREFABS/*.cpp is included.
#include <mcpp/mcpp.h>
#include <vector>
#include <iostream>
#include <ctime>
#include <random>
#include <string>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <system_error>

// Task A
#include "plots.h"
#include "plot_finder.h"
#include "plot_renderer.h"
#include "build_buildings.h"
#include "utils.h"

// Task B
#include "tb_bfs_alg.h"
#include "tb_waypoint.h"
#include "tb_house_grouping.h"
#include "tb_waypoint_gen.h"
#include "tb_homemade.h"


int main(const int argc, const char *const *const argv)
{
    // /* FLAGS USAGE: ./gen-village [OPTIONS] (Range)
    //     --loc=x,z           (world limit)
    //     --village-size=int  (15->100000)
    //     --seed=int          (int64_t)
    //     --density=int       (1-5)
    //     --debug
    //     --profile
    //     --help
    // */

    int loc_x = 0;
    int loc_z = 0;
    bool loc_provided = false;
    constexpr int default_village_size = 200;
    constexpr int min_density = 1;
    constexpr int max_density = 5;
    int village_size = default_village_size;
    int density = 3;
    int seed = static_cast<int>(std::time(nullptr));
    bool debug = false;
    bool profile = false;

    const std::string location_prefix = "--loc=";
    const std::string village_size_prefix = "--village-size=";
    const std::string density_prefix = "--density=";
    const std::string seed_prefix = "--seed=";
    const std::string help_option = "--help";
    const std::string debug_option = "--debug";
    const std::string profile_option = "--profile";

    int exit_code = 0;
    bool process_arguments = true;

    for (int argument_index = 1;
         process_arguments && argument_index < argc; argument_index++)
    {
        // The executable name occupies argv[0], arguments start argv[1].
        std::string argument = argv[argument_index];

        if (argument.rfind(location_prefix, 0) == 0)
        {
            try
            {
                std::string coordinates =
                    argument.substr(location_prefix.size());
                std::size_t comma = coordinates.find(',');

                // Require exactly one comma between the coordinates.
                if (comma == std::string::npos ||
                    coordinates.find(',', comma + 1) != std::string::npos)
                {

                    throw std::invalid_argument("Expected X,Z coordinates");
                }

                // Parse both values before updating the village location.
                std::string x_text = coordinates.substr(0, comma);
                std::string z_text = coordinates.substr(comma + 1);

                // Parse_integer lives in utils.h
                int parsed_x = parse_integer(x_text);
                int parsed_z = parse_integer(z_text);

                loc_x = parsed_x;
                loc_z = parsed_z;
                loc_provided = true;
            }
            catch (const std::exception &error)
            {
                std::cerr << "Error: Invalid --loc argument: "
                          << error.what() << std::endl;
                exit_code = 1;
                process_arguments = false;
            }
        }
        else if (argument.rfind(village_size_prefix, 0) == 0)
        {
            try
            {
                village_size = parse_integer(
                    argument.substr(village_size_prefix.size()));

                if (village_size <= 0)
                {
                    throw std::invalid_argument(
                        "Village size must be positive");
                }
            }
            catch (const std::exception &error)
            {
                std::cerr << "Error: Invalid --village-size argument: "
                          << error.what() << std::endl;
                exit_code = 1;
                process_arguments = false;
            }
        }
        else if (argument.rfind(density_prefix, 0) == 0)
        {
            try
            {
                density = parse_integer(
                    argument.substr(density_prefix.size()));

                if (density < min_density || density > max_density)
                {
                    throw std::invalid_argument(
                        "Density must be between 1 and 5");
                }
            }
            catch (const std::exception &error)
            {
                std::cerr << "Error: Invalid --density argument: "
                          << error.what() << std::endl;
                exit_code = 1;
                process_arguments = false;
            }
        }
        else if (argument.rfind(seed_prefix, 0) == 0)
        {
            try
            {
                // Seed can be negative or positive! Error if non-numeric.
                seed = parse_integer(argument.substr(seed_prefix.size()));
            }
            catch (const std::exception &error)
            {
                std::cerr << "Error: Invalid --seed argument: "
                          << error.what() << std::endl;
                exit_code = 1;
                process_arguments = false;
            }
        }
        else if (argument == help_option)
        {
            help();
            process_arguments = false;
        }
        else if (argument == debug_option)
        {
            debug = true;
        }
        else if (argument == profile_option)
        {
            profile = true;
        }
        else
        {
            // Reject unknown options before processing further arguments.
            std::cerr << "Error: Unknown argument: " << argument << std::endl;
            exit_code = 1;
            process_arguments = false;
        }
    }

    if (process_arguments)
    {
        try
        {
            mcpp::MinecraftConnection mc;

            print_student_details();

            // Use the player position when --loc was not supplied.
            if (!loc_provided)
            {
                try
                {
                    mcpp::Coordinate player_position = mc.getPlayerPosition();

                    loc_x = player_position.x;
                    loc_z = player_position.z;
                }
                catch (const std::exception &error)
                {
                    std::cerr << "Error: Unable to retrieve player position: "
                              << error.what() << std::endl;
                    exit_code = 1;
                }
            }

            if (exit_code == 0)
            {
                if (profile) {
                    performance_profile().begin(seed, village_size, density,
                                                loc_x, loc_z);
                }
                const auto generation_start = PerformanceProfile::Clock::now();
                std::size_t plot_count = 0;
                std::size_t waypoint_count = 0;
                std::mt19937 rng(seed);

                std::cout << "Village: centre (" << loc_x << ", " << loc_z
                          << "), size " << village_size
                          << ", density " << density
                          << ", seed " << seed << '\n';

                progress_log("1/6 Finding valid building plots...");

                std::vector<Plot> plots;
                {
                    PerformanceStage timing("find_plots");
                    plots = find_plots(loc_x, loc_z, village_size, rng,
                                       debug, density);
                }
                plot_count = plots.size();

                if (plots.empty())
                {
                    std::cerr << "Error: No valid plots available "
                              << "for construction "
                              << "(check village size, terrain, "
                                 "and obstacles).\n";
                    exit_code = 1;
                }
                else
                {
                    if (debug)
                    {
                        debug_log(
                            debug, "Rendering diagnostic plot boundaries");
                        PerformanceStage timing("debug_plot_boundaries");
                        render_plot_boundaries(plots, mc);
                    }

                    progress_log("2/6 Constructing and furnishing " +
                                 std::to_string(plot_count) + " houses...");

                    {
                        PerformanceStage timing("build_buildings");
                        try {
                            // Use the same half-open region as find_plots().
                            const int min_x = loc_x - village_size / 2;
                            const int min_z = loc_z - village_size / 2;
                            const VillageBounds bounds = {
                                min_x, min_z,
                                min_x + village_size, min_z + village_size
                            };
                            build_buildings(plots, mc, rng, debug, bounds);
                        } catch (const std::exception& error) {
                            std::cerr << "Error building/furnishing houses: "
                                      << error.what() << '\n';
                            exit_code = 1;
                        }
                    }

                    if (debug)
                    {
                        for (const Plot &plot : plots)
                        {
                            std::optional<Entrance> entrance =
                                plot.getEntrance();

                            if (entrance.has_value())
                            {
                                const char* facing = "unknown";
                                if (entrance->facing == Direction::NORTH) {
                                    facing = "north";
                                } else if (entrance->facing ==
                                           Direction::SOUTH) {
                                    facing = "south";
                                } else if (entrance->facing ==
                                           Direction::EAST) {
                                    facing = "east";
                                } else if (entrance->facing ==
                                           Direction::WEST) {
                                    facing = "west";
                                }
                                debug_log(debug, "Entrance: (" +
                                    std::to_string(entrance->position.x) +
                                    ", " +
                                    std::to_string(entrance->position.y) +
                                    ", " +
                                    std::to_string(entrance->position.z) +
                                    "), facing " + facing);
                            }
                        }
                    }
                }

                // Task B only runs when plot generation succeeded.
                if (exit_code == 0) {
                    // Task B
                    progress_log("3/6 Selecting waypoint locations...");
                    int x1 = loc_x - village_size / 2;
                    int z1 = loc_z - village_size / 2;
                    int x2 = loc_x + village_size / 2;
                    int z2 = loc_z + village_size / 2;

                    const int surface_y = plots.front().getOrigin().y;
                    for (const Plot &plot : plots) {
                        if (plot.getOrigin().y != surface_y) {
                            throw std::runtime_error(
                                "Paths require plots at the "
                                "same surface height.");
                        }
                    }

                    bool* path_map = path_viability_check(
                        x1, z1, x2, z2, surface_y);
                    int plot_amount = plots.size();
                    Plot* cs_plots = plots.data();
                    std::vector<Waypoint> waypoints;
                    {
                        PerformanceStage timing("group_plots");
                        waypoints = group_plots(
                            cs_plots, plot_amount, x1, z1, x2, z2,
                            path_map, surface_y);
                    }
                    if (waypoints.empty()) {
                        delete[] path_map;
                        throw std::runtime_error(
                            "No valid waypoint location "
                            "on the village surface.");
                    }
                    waypoint_count = waypoints.size();
                    std::cout << "Waypoints selected: "
                              << waypoint_count << '\n';


                    progress_log("4/6 Connecting houses to waypoints...");
                    {
                        PerformanceStage timing("connect_houses");
                        for (Waypoint wp : waypoints)
                        {
                            if (debug) {
                                wp.print();
                            }
                            connectHouses(
                                x1, z1, x2, z2, wp, path_map,
                                cs_plots, plot_amount,
                                mcpp::Blocks::GRASS_PATH, debug, surface_y);
                        }
                    }
                    progress_log("5/6 Connecting waypoints...");
                    {
                        PerformanceStage timing("connect_waypoints");
                        connectWaypoints(
                            x1, z1, x2, z2, waypoints, path_map,
                            cs_plots, plot_amount,
                            mcpp::Blocks::COBBLESTONE, debug, surface_y);
                    }
                    progress_log("6/6 Building waypoint markers...");
                    {
                        PerformanceStage timing("waypoint_platforms");
                        waypointPlatforms(waypoints, surface_y);
                    }

                    delete[] path_map;

                    const std::size_t failed_paths = bfs_failed_attempts();

                    progress_log("Generation stages finished: " +
                        std::to_string(plot_count) + " houses, " +
                        std::to_string(waypoint_count) + " waypoints.");

                    if (failed_paths != 0) {
                        std::cerr << "Warning: " << failed_paths
                                  << " pathfinding attempts failed. "
                                  << "Check village connectivity.\n";
                    }

                    if (profile) {
                        const double total_ms =
                            std::chrono::duration<double, std::milli>(
                                PerformanceProfile::Clock::now() -
                                generation_start).count();
                        performance_profile().record_time(
                            "total_generation", total_ms);
                        performance_profile().print_summary();
                        std::error_code directory_error;
                        std::filesystem::create_directories(
                            "testing", directory_error);
                        if (directory_error)
                        {
                            std::cerr << "Warning: Unable to create testing/: "
                                      << directory_error.message() << '\n';
                        }
                        else if (!performance_profile().append_csv(
                                     "testing/performance.csv", plot_count,
                                     waypoint_count, exit_code))
                        {
                            std::cerr << "Warning: Unable to write "
                                      << "testing/performance.csv\n";
                        }
                    }
                }
            }
        }
        catch (const std::exception &error)
        {
            std::cerr << "Error: " << error.what() << std::endl;
            exit_code = 1;
        }
    }
    return exit_code;
}
