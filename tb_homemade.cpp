#include "tb_homemade.h"

homemade::homemade() {
    capacity = 10; 
    current_size = 0;
    x_coords = new int[capacity];
    z_coords = new int[capacity];
}

homemade::~homemade() {
    delete[] x_coords;
    delete[] z_coords;
    x_coords = nullptr;
    z_coords = nullptr;
}

void homemade::resize() {
    capacity *= 2; 
    int* new_x = new int[capacity];
    int* new_z = new int[capacity];

    for (int i = 0; i < current_size; ++i) {
        new_x[i] = x_coords[i];
        new_z[i] = z_coords[i];
    }
    delete[] x_coords;
    delete[] z_coords;
    x_coords = new_x;
    z_coords = new_z;
}

void homemade::push_back(int x, int z) {
    if (current_size >= capacity) {
        resize();
    }
    x_coords[current_size] = x;
    z_coords[current_size] = z;
    current_size++;
}

int homemade::size() const {
    return current_size;
}

bool homemade::empty() const {
    return current_size == 0;
}

int homemade::getX(int index) const {
    return x_coords[index];
}

int homemade::getZ(int index) const {
    return z_coords[index];
}