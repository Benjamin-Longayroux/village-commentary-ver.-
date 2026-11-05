
CXX := g++
CXXFLAGS := -Wall -Werror -std=c++17 -O -g
LDLIBS := -lmcpp

# Explicit production sources.
# Test programs with their own main() are excluded.
SOURCES := \
    main.cpp \
    build_buildings.cpp \
    entrance_planner.cpp \
    exterior_renderer.cpp \
    plot_finder.cpp \
    plot_renderer.cpp \
    plots.cpp \
    room_renderer.cpp \
    room_subdivision.cpp \
    tb_bfs_alg.cpp \
    tb_house_grouping.cpp \
    tb_homemade.cpp \
    tb_waypoint_gen.cpp \
    utils.cpp \
    PREFABS/prefab_reader.cpp \
    PREFABS/furniture_catalogue.cpp \
    PREFABS/prefab_renderer.cpp \
    PREFABS/furniture_planner.cpp

# Preserve source directory structure inside build/.
OBJECTS := $(patsubst %.cpp,build/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean

all: gen-village

# Link every production object into the final executable.
gen-village: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

# Compile individual source files.
# Create nested output directories when necessary.
build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Remove all generated compilation artifacts.
clean:
	rm -rf build gen-village
	rm -f ./*.o ./*.d
	rm -f PREFABS/*.o PREFABS/*.d

# Automatically track header dependencies.
-include $(DEPS)
