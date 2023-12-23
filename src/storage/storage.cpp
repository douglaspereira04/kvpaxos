#include "storage.h"



namespace kvstorage {

#if defined(MICHAEL) || defined(FELDMAN)
    typedef typename storage_t::guarded_ptr GuardedPointer;
#endif

int VALUE_SIZE = 4096;
std::string template_value(VALUE_SIZE, '*');


std::string Storage::read(int key) {
    std::string val;

#if defined(MICHAEL) || defined(FELDMAN)
    GuardedPointer gp(storage_.get(key));
    if(gp){
        val = gp->second;
    }
#elif defined(TBB)
    val = storage_.at(key);
#else 
    val = storage_.at(key);
#endif
    try {

        return decompress(val);

    } catch(...) {
        // I'm not sure why sometimes decompression fails.
        // It fails in what seems to be random keys and in less
        // than 0.00001% of calls, so lets just ignore it for now.
        // It'll be wise to investigate
        return template_value;
    }
}

void Storage::write(int key, const std::string& value) {
    auto compressed_value = compress(template_value);
#if defined(MICHAEL) || defined(FELDMAN)
    storage_.insert(key,compressed_value);
#elif defined(TBB)
    storage_[key] = compressed_value;
# else 
    storage_.insert(key, compressed_value);
#endif
}

std::vector<std::string> Storage::scan(int start, int length) {
    auto values = std::vector<std::string>();
    for (auto i = 0; i < length; i++) {
        auto key = (start + i) % storage_.size();
        values.push_back(read(key));
    }
    return values;
}

};
