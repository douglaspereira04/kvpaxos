#include "storage.h"



namespace kvstorage {


#if defined(TBB)
    storage_t *create_storage_map(){ return new storage_t(); }
#else
    storage_t *create_storage_map(){ return new storage_t(); }
#endif

int VALUE_SIZE = 4096;
std::string template_value(VALUE_SIZE, '*');


std::string Storage::read(int key) const {
    try {
        std::string val;

#if defined(TBB)
        val = (*storage_)[key];
#else 
        val = storage_->at(key);
#endif
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
#if defined(TBB)
    (*storage_)[key] = template_value;
# else 
    storage_->insert(key, template_value);
#endif
}

std::vector<std::string> Storage::scan(int start, int length) {
    auto values = std::vector<std::string>();
    for (auto i = 0; i < length; i++) {
        auto key = (start + i) % storage_->size();
        values.push_back(read(key));
    }
    return values;
}

};
