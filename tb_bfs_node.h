#ifndef tb_bfs_node
#define tb_bfs_node

class Node {
    public: 
        int x; 
        int z;
        Node* parent;
        Node* next; 
    
        Node(int x, int z, Node* parent = nullptr, Node* next ){
            this->x = x;
            this->z = z; 
            this->parent = parent;
            this->next = next; 
        }
    };

#endif