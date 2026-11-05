
# Minecraft Village Generator — Group 13

**Course:** COSC2804 — C++ Programming Studio  
**Assessment:** Assignment 3 — Minecraft Village Generator  
**Semester:** Semester 2, 2026  
**Repository:** [Group 13 GitHub Repository](https://github.com/rmit-computing-technologies/cpp-studio-cpp-villages-group-13)

---

## Team Members

| Task | Student | Student ID | GitHub Username |
|---|---|---|---|
| Task A — Building Generation | Joshua Appleton | s4246434 | `s4246434` |
| Task B — Pathfinding | Benjamin Longayroux | s4246540 | `Benjamin-Longayroux` |

## Project Overview

This C++17 program generates villages in Minecraft using the Minecraft C++ API (`mcpp`).

**Task A — Building Generation**

Finds suitable building plots and procedurally constructs houses using recursive subdivision, internal doors, multiple floors and modular furniture prefabs.

**Task B — Pathfinding**

Connects generated houses through waypoints and breadth-first-search (BFS) pathfinding.

The program uses a seeded random number generator to support reproducible village generation under consistent program and world conditions. A custom integer sampler is used to maintain consistent random selections across the tested C++ environments.

## Requirements

- G++ compiler with C++17 support
- Minecraft C++ API (`mcpp`)
- Course-provided Spigot installation
- GNU Make
- Updated ELCI plugin:
  [Download ELCI 1.12.1 — 9000f4a](https://github.com/Japples0/ELCI/releases/download/v1.12.1-proof-9000f4a/ELCI-1.12.1-9000f4a.jar)

The project uses the C++17 standard library and `mcpp` without additional third-party C++ libraries.

A Superflat Minecraft world is recommended for testing. The generator has been tested on modified Superflat worlds with different ground heights.

## Compiling

The project uses a Makefile to compile all required production source files.

From the repository root, run:

```bash
make
```

Alternatively, perform a clean rebuild using:

```bash
make clean && make
```

### Compiler Configuration

The Makefile uses the following compiler flags:

```makefile
CXX := g++
CXXFLAGS := -Wall -Werror -std=c++17 -O -g
LDLIBS := -lmcpp
```

| Flag | Purpose |
|---|---|
| `-Wall` | Enables common compiler warnings |
| `-Werror` | Treats compiler warnings as errors |
| `-std=c++17` | Enables C++17 language features |
| `-O` | Enables compiler optimisation |
| `-g` | Includes debugging information |
| `-lmcpp` | Links the Minecraft C++ API library |

### How the Makefile Works

The Makefile explicitly defines the 18 production `.cpp` files required for compilation. These include source files in the repository root and the `PREFABS/` directory.

Standalone testing programs containing their own `main()` functions are excluded from the production source list to prevent multiple entry-point definitions.

**1. Compiling source files**

Each production `.cpp` file is compiled individually into an object file (`.o`).

The Makefile preserves the original source directory structure inside `build/`, for example:

```text
build/
├── main.o
├── plots.o
├── room_subdivision.o
├── tb_bfs_alg.o
└── PREFABS/
    ├── prefab_reader.o
    ├── prefab_renderer.o
    ├── furniture_catalogue.o
    └── furniture_planner.o
```

The necessary directories are created automatically during compilation.

**2. Automatic dependency tracking**

The `-MMD -MP` compiler flags generate dependency files (`.d`) that record which header files each source file uses.

These are included by Make, allowing affected source files to be recompiled when their dependencies change.

**3. Incremental compilation**

Running `make` again only recompiles files whose source code or dependencies have changed.

This avoids unnecessary recompilation of the entire project during development.

**4. Linking**

Once compilation is complete, all production object files are linked together with `-lmcpp`.

The final executable is named:

```text
gen-village
```

It is generated in the repository root, separate from the intermediate files in `build/`.

**5. Cleaning**

Running:

```bash
make clean
```

Removes:

- The `build/` directory
- The generated `gen-village` executable
- Any leftover `.o` or `.d` files in the root or `PREFABS/` directory

This allows the project to be rebuilt from a clean state.

### Build Artefacts and Repository Organisation

Intermediate compilation files are kept inside `build/`, while the final executable remains in the repository root for convenient execution.

The `.gitignore` excludes generated build files, executable binaries, local testing directories, runtime logs, performance data and editor-specific settings.

This keeps the Git repository focused on source code and submission files. Generated test results remain local, so each user can produce their own results when running the program.

## Running the Program

Ensure the Minecraft server is running and accessible through `mcpp` before executing the generator.

Example:

```bash
./gen-village --loc=0,0 --village-size=200 --density=3 --seed=1
```

### Command-Line Options

| Option | Description | Default |
|---|---|---|
| `--loc=X,Z` | Village centre coordinates | Player position |
| `--village-size=N` | Village generation area size | 200 |
| `--density=N` | Village density (1–5) | 3 |
| `--seed=N` | Random generation seed | Current time |
| `--debug` | Enables additional diagnostic messages | Disabled |
| `--profile` | Enables performance profiling | Disabled |
| `--help` | Displays command-line usage information | — |

### Reproducibility

To reproduce a generation run, use the same village location, size, density and random seed.

Consistent Minecraft world conditions and the same program version are also required for comparable results.

### Performance Profiling

The optional `--profile` flag records execution statistics and timing information.

For example:

```bash
./gen-village --loc=100,100 --village-size=200 --density=3 --seed=1 --profile
```

Generated performance data is excluded from Git using `.gitignore`. This allows developers and markers to produce independent profiling results without reusing previously recorded test data.

## Development Contributions

### Task A — Joshua Appleton

Responsible for procedural house generation and supporting systems.

- Command-line interface and runtime argument validation
- Performance profiling using `std::chrono`
- Random number generation and cross-platform random sampling
- Plot data structures and plot placement
- Recursive room subdivision
- Interior doors and exterior entrance planning
- Multi-floor building construction
- Exterior and interior block rendering
- Prefab reading, selection, rendering and furniture planning
- Makefile and build organisation

**Primary source files:**

```text
plots.h / plots.cpp
plot_finder.h / plot_finder.cpp
plot_renderer.h / plot_renderer.cpp
room_subdivision.h / room_subdivision.cpp
entrance_planner.h / entrance_planner.cpp
build_buildings.h / build_buildings.cpp
exterior_renderer.h / exterior_renderer.cpp
room_renderer.h / room_renderer.cpp
utils.h / utils.cpp
PREFABS/
Makefile
```

### Task B — Benjamin Longayroux

Responsible for waypoint generation, house grouping, BFS pathfinding and path construction.

**Primary source files:**

```text
tb_bfs_alg.cpp
tb_house_grouping.cpp
tb_waypoint_gen.cpp
```

Supporting header files and development/testing code are included in the Task B implementation.

### Shared Contributions

`main.cpp` — Integration of Task A and Task B into the complete village generation program.

## Development Branches

Development was organised using separate feature branches before integration into the main program.

### Task A — Building Generation

- `ROOM-SUBDIVISIONS` — Recursive room subdivision development
- `PREFABS` — Prefab loading and rendering
- `FURNITURE` — Furniture planning and placement
- `Operation-XRAY` — Development and testing of profiling functionality
- `CODEBASE-CLEANUP` — Codebase cleanup and refinement

### Task B — Pathfinding

- `tb-plot-grouping` — Plot grouping development
- `TB-house-->-plot-translation` — House-to-plot translation development

### Integration and Finalisation

- `main` — Integrated project and final submission
- `Finalised` — Finalisation branch
- `feedback` — Earlier development branch

Feature development was progressively integrated into the main project. The final submitted version is maintained on `main`.

## Video Demonstration

[https://rmiteduau-my.sharepoint.com/:v:/r/personal/s4246434_student_rmit_edu_au/Documents/Documents/2026%20S2/C++%20Studio/Assignment%203/Village%20Assignment/VILLAGE%20VIDEO%20V4.mp4?d=w5b102357c316411cbb5502b68ff95aec&csf=1&web=1&e=nt9Xf8](https://rmiteduau-my.sharepoint.com/:v:/g/personal/s4246434_student_rmit_edu_au/IQBXIxBbFsMcQbtVAraP-VrsAa3nxu_wn5-jieehNI0ct7c?e=X8atmW)

## Final Report

[View the A3 Report](A3%20REPORT%20GROUP%2013.pdf)


## Known Limitations

- Performance profiling records program execution and API-call completion, rather than independently verifying every block placed in Minecraft.
- Certain modern Minecraft block types may not be recognised correctly during path obstacle detection.

Further discussion of design decisions, optimisation, testing and limitations is provided in the final assignment report.
