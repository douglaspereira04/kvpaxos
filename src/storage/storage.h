#ifndef _KVPAXOS_STORAGE_H_
#define _KVPAXOS_STORAGE_H_


#include <string>

namespace kvstorage {

class Storage {
public:
    Storage() {};
    void init();

    int read(int key, std::string &value);
    void write(int key, const std::string &value);
    void del(int key);

    inline const size_t &level() const {
        return __level;
    }

    inline void level(size_t level_) {
        __level = level_;
    }

private:
    size_t __level = 0;
};
};

#endif
