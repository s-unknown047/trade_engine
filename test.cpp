#include <bits/stdc++.h>
using namespace std;
#include <atomic>
#include <thread>

void solve (atomic<int>& count) {
    for (int i = 0; i < 100000; i++) {
        count++;
    }
}
int main () {
    atomic<int> count = 0;
    std::thread t1(solve, ref(count));
    std::thread t2(solve, ref(count));
    t1.join();
    t2.join();
    cout<< "count" <<endl;
    cout << count;

    return 0;
}