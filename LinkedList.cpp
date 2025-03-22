#include <bits/stdc++.h>
using namespace std;
#include "LinkedList.h"

Node::Node(uint32_t value){
    data = value;
    next = nullptr;
}

LinkedList::LinkedList(){
    head = nullptr;
}

void LinkedList::insertAtEnd(uint32_t value){
    Node* newNode = new Node(value);
    if(head == nullptr){
        head = newNode;
    }
    else{
        Node* temp = head;
        while (temp->next != nullptr){
        temp = temp->next;
        }
        temp->next = newNode;
    }
}

void LinkedList::deleteByValue(uint32_t value){
    if(head == nullptr){
        return;
    }
    if(head->data == value){
        Node* temp = head;
        head = head->next;
        delete temp;
        return;
    }
    Node* temp = head;
    while(temp->next && temp->next->data != value){
        temp = temp->next; 
    }
    if(temp->next){
        Node* nodeToDelete = temp->next;
        temp->next = temp->next->next;
        delete nodeToDelete;
    }
}

void LinkedList::display(){
    Node* temp = head;
    while(temp != nullptr){
        cout << temp->data << "->";
        temp = temp->next;
    }
    cout << "NULL" <<endl;
}

LinkedList::~LinkedList() {
    Node* temp;
    while (head) {
        temp = head;
        head = head->next;
        delete temp;
    }
}