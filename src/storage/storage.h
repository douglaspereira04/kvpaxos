#ifndef _KVPAXOS_STORAGE_H_
#define _KVPAXOS_STORAGE_H_


#include <string>
#include <vector>

namespace kvstorage {

class Storage {
public:
    Storage() {};
    void init();

    int read(std::string &key, std::string &value);
    void write(std::string &key, const std::string &value);
    void del(std::string &key);
    std::vector<std::string> scan(std::string &key, size_t len);

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
