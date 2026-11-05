#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <vector>
#include <iostream>

#include "plots.h"

class Waypoint {
public: 
    int x; 
    int z; 
    std::vector<Plot*> children;
    bool linked; 
    int dist; 

    Waypoint(int x, int z) {
        this->x = x;
        this->z = z; 
        this->children = {};
        this-> linked = false; 
        this-> dist = 0; 
    }

    void print() const {
        std::cout << "[DEBUG] Waypoint at (" << x << ", " << z << ") with "
                  << children.size() << " child plots:" << std::endl;
        
        for (size_t i = 0; i < children.size(); ++i) {
            if (children[i] != nullptr) {
                mcpp::Coordinate origin = children[i]->getOrigin();
                std::cout << "[DEBUG]   -> Child " << i << ": Plot origin ("
                          << origin.x << ", " << origin.y << ", " << origin.z << ")" 
                          << " [Size: " 
                          << children[i]->getWidth() << "x" 
                          << children[i]->getDepth() << "]"
                          << std::endl;
            }
        }
    }
};

#endif