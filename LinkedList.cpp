#include <iostream>

struct Node {
    int data;
    Node* next;

    Node (int data) : data(data), next(nullptr) {}  
};

class LinkedList {
    private:                    
       Node* head;

    public:
        LinkedList() : head(nullptr) {}

    void insert(int data) {
        Node* newNode = new Node(data);
        if (!head) {
            head = newNode;
        } else {
            Node* temp = head;
            while (temp->next) {
                temp = temp->next;
            }
            temp->next = newNode;
        }
    }   
};