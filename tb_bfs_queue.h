#ifndef tb_bfs_queue
#define tb_bfs_queue

#include "tb_bfs_node.h"

class BfsQueue {
public: 
    Node* headN;
    Node* rearN;
    int size; 

    BfsQueue() {
        headN = nullptr;
        rearN = nullptr;
        size = 0;
    }

    bool isEmpty(){
        return headN == nullptr; 
    }

    void push(Node* newN){
        newN->next = nullptr;
        if(this->isEmpty()){
            headN = newN;
            rearN = newN;
        } else {
            this->rearN->next = newN;
            this->rearN = newN; 
        }
        this->size++; 
    }

    Node* pop(){
        if(this->isEmpty()){
            return nullptr;
        }
        Node* temp = this->headN;
        this->headN = this->headN->next;
        if (this->headN == nullptr) {
            rearN = nullptr;
        }
        this->size--;
        return temp;
    }
};

#endif