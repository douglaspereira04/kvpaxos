#ifndef _KVPAXOS_STORAGE_H_
#define _KVPAXOS_STORAGE_H_


#include <string>
#include <unordered_map>
#include <vector>

#include "concurrent_unordered_map.cpp"

#include "compresser/compresser.h"
#include "types/types.h"
//#include <cds/gc/hp.h>


#if defined(MICHAEL)

    #include <cds/container/michael_kvlist_hp.h>
    #include <cds/container/michael_map.h>
    typedef cds::container::MichaelHashMap<cds::gc::HP, cds::container::MichaelKVList<cds::gc::HP, int, std::string>> storage_t;
    
#elif defined(FELDMAN)
    #include <cds/container/feldman_hashmap_hp.h>
    typedef cds::container::FeldmanHashMap<cds::gc::HP, int, std::string> storage_t;
#elif defined(TBB)
    #include "tbb/concurrent_unordered_map.h"
    typedef tbb::concurrent_unordered_map<int, std::string> storage_t;
#else
    typedef kvstorage::concurrent_unordered_map<int, std::string> storage_t;
#endif



namespace kvstorage {

class Storage {
public:
    Storage() = default;

    std::string read(int key);
    void write(int key, const std::string& value);
    std::vector<std::string> scan(int start, int length);
    
private:
#if defined(MICHAEL)
    storage_t storage_ = storage_t(1048576,1);
#elif defined(FELDMAN)
    storage_t storage_ = storage_t(16,8);
#elif defined(TBB)
    storage_t storage_ = storage_t();
#else
    storage_t storage_ = storage_t();
#endif



};

};

#endif
