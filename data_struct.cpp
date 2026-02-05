#include <iostream>
#define MAX_SIZE 1000
struct Node {
    int data;
    int nextIndex;
    int prevIndex;

};

class data_struct {
  private:
  int head_index;
  int tail_index;
  int prev_index;
  int size;
  public: 
  data_struct () {
    head_index = -1;
    tail_index = -1;
    prev_index = -1; 
    size = 0;
  }
  void insert(int data, Node* arr){
    if ((tail_index+1)%MAX_SIZE == head_index) {
        std::cerr << "Error: List is full!" << std::endl;
        return;
    }

    Node newNode;

    if (head_index == -1) {
        head_index =  0;
        tail_index  = 1;
        newNode = {data, prev_index, tail_index};
        prev_index = head_index;    
        arr[head_index] = newNode;
    } else {

        int current_index = tail_index;
        tail_index = (tail_index + 1) % MAX_SIZE;
        Node newNode = {data,prev_index, tail_index};
        arr[current_index] = newNode;
        prev_index = current_index;
    }

    size++;
  }

  int deleteIndex(int index,int* arr) {
  }
};


int main() {

    Node arr[MAX_SIZE];


}