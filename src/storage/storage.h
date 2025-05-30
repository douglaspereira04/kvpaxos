#ifndef _KVPAXOS_STORAGE_H_
#define _KVPAXOS_STORAGE_H_


#include <string>

namespace kvstorage {

class Storage {
public:
    Storage() {};

    int read(int key, std::string* &value);
    void write(int key, std::string *value);
    void del(int key);

};
};

#endif
