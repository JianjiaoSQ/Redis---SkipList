#include<iostream>
#include "skipList.h"

int main() {
    skipList<std::string,std::string> skiplist(6);

    /*
    skiplist.insert(1,"bob");
    skiplist.insert(2,"aoa");
    skiplist.insert(8,"cococ");
    skiplist.insert(7,"fdff");
    skiplist.insert(4,"fsfjsh");
    skiplist.insert(0,"dkjajd");

    skiplist.displayList();
    skiplist.del(6);
    skiplist.del(0);
    skiplist.displayList();
    skiplist.writeFile();

*/
    skiplist.loadFile();
    skiplist.displayList();
    skiplist.modify("zhanghui","0001X");
    skiplist.displayList();

    skiplist.rangeSearch("A","}");
    return 0;
}