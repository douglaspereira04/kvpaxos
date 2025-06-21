#include "tkrzw_storage.h"

namespace kvstorage {
template<typename T>
void TKRZWStorage<T>::init(){
    std::string path = 
        std::string("/tmp/repart_kv_storage/") +
        id +
        std::string("/");
    std::filesystem::create_directories(path);
    path += std::to_string(db_counter.fetch_add(1, std::memory_order_relaxed));
    tkrzw::Status status;

    __storage = new tkrzw::HashDBM();
    size_t i = 0;
    do {
        status = __storage->Open(path, true);
    } while(!status.IsOK() && 10 > i++);
    assert(status.IsOK());
}

template<typename T>
std::atomic_int TKRZWStorage<T>::db_counter = 0;

template<typename T>
std::string TKRZWStorage<T>::id = std::to_string(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count()
);
}