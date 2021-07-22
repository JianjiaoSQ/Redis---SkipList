#include<iostream>
#include "skipList.h"

#define FILE_PATH "storeFile"

int main() {
    skipList<int,std::string> skiplist(6);

    skiplist.insert(1,"bob");
    skiplist.insert(2,"aoa");
    skiplist.insert(8,"cococ");
    skiplist.insert(7,"fdff");
    skiplist.insert(4,"fsfjsh");
    skiplist.insert(0,"dkjajd");

    skiplist.displayList();

    return 0;
}