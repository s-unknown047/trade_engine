#include <bits/stdc++.h>

using namespace std;

int main() {
    vector <int> v;
    
    v.reserve(1000);

    v.push_back (1);
    v.push_back(3);

    for (int i = 0; i < v.size(); i++) cout << v[i] << endl;
    
    cout<<v.size()<<endl;
}