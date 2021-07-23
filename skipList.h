/* *********************************************
* 实现 Redis 中的跳表，Redis 中的 Zset 部分底层就是跳表；

********************************************** */

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "storeFile"

std::mutex mtx;
std::string delimiter = ":";

// 跳表节点设置
template<typename Key, typename Value>
class skipNode {
public:
    skipNode();
    ~skipNode();

    skipNode(Key key, Value value, int);
    Key getKey() const;
    Value getValue() const;
    
    skipNode<Key, Value>** forward;
    int level;

private:
    Key key;

public:
    Value value;
};

template<typename Key, typename Value>
skipNode<Key, Value>::skipNode(const Key key, const Value value, int level) {
    this->key = key;
    this->value = value;
    this->level = level;

    this->forward = new skipNode<Key,Value>* [level+1];
    memset(this->forward, 0, sizeof(skipNode<Key, Value>*) * (level+1)); // ???
}

template<typename Key, typename Value>
skipNode<Key, Value>::~skipNode() {
    delete[] forward;
};

template<typename Key, typename Value>
Key skipNode<Key, Value>::getKey() const {
    return key;
};

template<typename Key, typename Value>
Value skipNode<Key, Value>::getValue() const {
    return value;
};

// 跳表链表
template<typename Key, typename Value>
class skipList {
public:
    skipList(int);
    ~skipList();

    skipNode<Key, Value>* createNode(Key, Value , int);
    int getRandomLevel();

    int insert(Key key, Value value); 
    int del(Key key);
    int modify(Key key, Value value);
    int search(Key key);
    int rangeSearch(const Key beginKey, const Key endKey);

    void displayList();
    void writeFile();
    void loadFile();
    int size();


private:
    void readLine(const std::string& str,std::string* key,std::string* val); 
    bool isVaild(const std::string& str);

    int maxLevel;
    int curLevel;
    skipNode<Key, Value>* head;
    std::ofstream fileWrite;
    std::ifstream fileRead;

    int countNode;
};

template<typename Key, typename Value>
skipNode<Key, Value>* skipList<Key, Value>::createNode(const Key key, const Value value, int level) {
    skipNode<Key, Value>* node = new skipNode<Key, Value>(key, value, level);
    return node;
}

/* ***********************************************
插入键值对
0 -> 键值已存在
1 -> 插入成功
************************************************ */

template<typename Key, typename Value>
int skipList<Key, Value>::insert(const Key key, const Value value) {
    mtx.lock();

    skipNode<Key, Value>* cur = this->head;
    skipNode<Key, Value>* update[maxLevel+1];
    memset(update, 0, sizeof(skipNode<Key, Value> *) * (maxLevel+1));

    for(int i = maxLevel; i >= 0; --i) {
        while(cur->forward[i] != NULL && cur->forward[i]->getKey() < key) {
            cur = cur->forward[i];
        }
        update[i] = cur;
    }
    cur = cur->forward[0];
    // 插入的键值已存在
    if(cur != NULL && cur->getKey() == key) {
        std::cout << "Key: " << key << ", 已存在！" << std::endl;
        mtx.unlock();
        return 0;
    }
    // 插入的键值比后一个键值小
    if(cur == NULL || cur->getKey() != key) {
        int randomLevel = getRandomLevel();

        if(randomLevel > curLevel) {
            for(int i = curLevel+1; i < randomLevel+1; i++) {
                update[i] = head;
            }
            curLevel = randomLevel;
        }

        skipNode<Key, Value>* iNode = createNode(key, value, randomLevel);

        for(int i = 0; i <= randomLevel; ++i) {
            iNode->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = iNode;
        }

        std::cout << "键值对(" << key << ", " << value << ") 成功插入！" << std::endl;
        countNode++;
    }
    mtx.unlock();
    return 1;
}

/* ****************删除键值对******************** */

template<typename Key, typename Value>
int skipList<Key, Value>::del(const Key key) {
    mtx.lock();
    skipNode<Key, Value>* cur = this->head;
    skipNode<Key, Value>* update[maxLevel+1];
    memset(update, 0, sizeof(skipNode<Key, Value> *) * (maxLevel+1));

    for(int i = curLevel; i >= 0; --i) {
        while(cur->forward[i] != NULL && cur->forward[i]->getKey() < key) {
            cur = cur->forward[i];
        }
        update[i] = cur;
    }

    cur = cur->forward[0];
    Value val;
    // 删除链表中的节点
    if(cur != NULL && cur->getKey() == key) {
        for(int i = 0; i <= curLevel; ++i) {
            // 本层没有该节点，跳出循环，因为本层开始之后都不会出现这个键值
            if(update[i]->forward[i] != cur) {
                break;
            }
            update[i]->forward[i] = cur->forward[i];
        }

        val = cur->getValue();
        // 更新层数，删除没有节点的层
        while(curLevel > 0 && head->forward[curLevel] == 0) {
            curLevel--;
        }
        std::cout << "键值对(" << key << ", " << val << ") 成功删除！" << std::endl;
        countNode--;
    }
    mtx.unlock();
    return 1;
}

/* *************** 更新键值对 ********************** 
* 修改键值对应的值，键值不能修改；
* 如果要修改键值，只能删除键值对后重新添加；
****************************************************/
                    
template<typename Key, typename Value>
int skipList<Key, Value>::modify(const Key key, const Value val) {
    skipNode<Key, Value>* cur = head;
    Value oldVal;

    for(int i = curLevel; i >= 0; --i) {
        while(cur->forward[i] && cur->forward[i]->getKey() < key) {
            cur = cur->forward[i];
        }
    }
    cur = cur->forward[0];

    if(cur && cur->getKey() == key) {
        oldVal = cur->getValue();
        cur->value = val;
        std::cout << "键值对(" << key << ", " << oldVal << ") 更新为 (" 
        << "键值对(" << key << ", " << cur->getValue() << ")" << std::endl;
        return 1;
    } 
    std::cout << "键值 Key = " << key << "未找到！" << std::endl;
    return 0;

}

/* ***************** 查找键值对 *********************               
* 含义：0：未找到  1：已找到
************************************************ */
template<typename Key, typename Value>
int skipList<Key, Value>::search(const Key key) {
    skipNode<Key, Value>* cur = head;
    Value val;

    for(int i = curLevel; i >= 0; --i) {
        while(cur->forward[i] && cur->forward[i]->getKey() < key) {
            cur = cur->forward[i];
        }
    }
    cur = cur->forward[0];

    if(cur && cur->getKey() == key) {
        val = cur->getValue();
        std::cout << "键值对(" << key << ", " << val << ") 已找到！" << std::endl;
        return 1;
    } 
    std::cout << "键值 Key = " << key << "未找到！" << std::endl;
    return 0;
}

/* *************** 跳表范围查找 ************** */
template<typename Key, typename Value>
int skipList<Key, Value>::rangeSearch(const Key left, const Key right) {
    skipNode<Key, Value>* cur = head;

    for(int i = curLevel; i >= 0; --i) {
        while(cur->forward[i] && cur->forward[i]->getKey() < left) {
            cur = cur->forward[i];
        }  
    }
    cur = cur->forward[0];
    if(cur && cur->getKey() >= left) {
        std::cout << "范围查询 [" << left << ", " << right << "] : " << std::endl;
        while(cur && cur->getKey() <= right) {
            std::cout << "("<< cur->getKey() << ", " << cur->getValue() << ")\n";
            cur = cur->forward[0];
        }
        std::cout << std::endl;
        return 1;
    }
    return 0;
}

/* ***************构建跳表***************** */
template<typename Key, typename Value>
skipList<Key, Value>::skipList(int maxLev) {
    this->maxLevel = maxLev;
    this->curLevel = 0;
    this->countNode = 0;

    Key key;
    Value val;
    this->head = new skipNode<Key, Value>(key,val,maxLevel);
};

template<typename Key, typename Value>
skipList<Key, Value>::~skipList() {
    if(fileWrite.is_open()) {
        fileWrite.close();
    }
    if(fileRead.is_open()) {
        fileRead.close();
    }
    delete head;
}

template<typename Key, typename Value>
int skipList<Key, Value>::getRandomLevel() {
    int k = 1;
    while(rand() % 2) {
        k++;
    }
    k = (k < maxLevel) ? k : maxLevel;
    return k;
};

/* *************** 显示跳表 ***************** */
template<typename Key, typename Value>
void skipList<Key, Value>::displayList() {
    std::cout << "\n********** skipList **********" << "\n";
    for(int i = 0; i <= curLevel; ++i) {
        skipNode<Key, Value>* node = this->head->forward[i];
        std::cout << "Level " << i << ": ";
        while(node != NULL) {
            std::cout << "(" << node->getKey() << ":" << node->getValue() << ");";
            node = node->forward[i];
        }
        std::cout << std::endl;
    } 
}

/* *************** 写入文件 ***************** */
template<typename Key, typename Value>
void skipList<Key, Value>::writeFile() {
    fileWrite.open(STORE_FILE);
    skipNode<Key, Value>* cur = this->head->forward[0];

    while(cur) {
        fileWrite << cur->getKey() << ":" << cur->getValue() << "\n";
        std::cout << cur->getKey() << ":" << cur->getValue() << ";\n";
        cur = cur->forward[0];
    }
    fileWrite.flush();
    fileWrite.close();
}

/* *************** 加载文件 ***************** */
template<typename Key, typename Value>
void skipList<Key, Value>::loadFile() {
    fileRead.open(STORE_FILE);
    std::string line;
    std::string* key = new std::string();
    std::string* val = new std::string();

    while(getline(fileRead,line)) {
        readLine(line, key, val);
        if(key->empty() || val->empty()) {
            continue;
        }
        insert(*key, *val);
    }
    std::cout << "加载完成！" << std::endl;
    fileRead.close();
}

/* *************** 读取信息 ***************** */

template<typename Key, typename Value>
void skipList<Key, Value>::readLine(const std::string& str, std::string* key, std::string* val) {
    if(!isVaild(str)) {
        return;
    }
    *key = str.substr(0,str.find(delimiter));
    *val = str.substr(str.find(delimiter)+1, str.length());
}

template<typename Key, typename Value>
bool skipList<Key, Value>::isVaild(const std::string& str) {
    if(str.empty()) {
        return false;
    }
    if(str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

template<typename Key, typename Value>
int skipList<Key, Value>::size() {
    return countNode;
}