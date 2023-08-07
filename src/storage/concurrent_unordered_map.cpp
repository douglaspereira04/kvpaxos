#ifndef _KVPAXOS_CONCURRENT_UNORDERED_MAP_H_
#define _KVPAXOS_CONCURRENT_UNORDERED_MAP_H_


#include <mutex>
#include <unordered_map>



namespace kvstorage {

template<typename KeyType, typename ValueType>
class concurrent_unordered_map : public std::unordered_map<KeyType, ValueType>{
public:
    concurrent_unordered_map() : std::unordered_map<KeyType, ValueType>(){}


    size_t size() { 
        mtx.lock();
        auto ret = std::unordered_map<KeyType, ValueType>::size(); 
        mtx.unlock();
        return ret;
    }

    void
    insert(KeyType key, ValueType value){
        mtx.lock();
        std::unordered_map<KeyType, ValueType>::operator[](key) = value; 
        mtx.unlock();
    }

    ValueType
    at(KeyType key){
        mtx.lock();
        auto ret = std::unordered_map<KeyType, ValueType>::operator[](key); 
        mtx.unlock();
        return ret;
    }

private:
    std::mutex mtx;
};

};

#endif
