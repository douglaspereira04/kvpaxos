#ifndef _KVPAXOS_STORAGE_H_
#define _KVPAXOS_STORAGE_H_


#include <string>

namespace kvstorage {

template<typename T>
class Storage {
public:
    Storage() {};
    void init();

    int read(T &key, std::string &value);
    void write(T &key, const std::string &value);
    void del(T &key);

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
