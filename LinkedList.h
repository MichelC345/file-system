#ifndef LINKEDLIST_H
#define LINKEDLIST_H
#include <cstdint>
#include <iostream>

class Node{
    public:
        uint32_t data;
        Node* next;
        Node(uint32_t value);
};


class LinkedList{
    private:
      Node* head;
    public:
        LinkedList();
        void insertAtEnd(uint32_t value);
        void deleteByValue(uint32_t value);
        void display();
        ~LinkedList();
};

#endif