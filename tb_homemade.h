#ifndef tb_homemade
#define tb_homemade

class homemade {
private:
    int* x_coords;    
    int* z_coords;    
    int capacity;     
    int current_size; 

    void resize();

public:
    homemade();

    ~homemade();

    void push_back(int x, int z);

    int size() const;
    bool empty() const;

    int getX(int index) const;
    int getZ(int index) const;
};

#endif