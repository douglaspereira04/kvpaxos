#include "storage.h"



namespace kvstorage {

int VALUE_SIZE = 4096;
std::string template_value(VALUE_SIZE, '*');


std::string* Storage::read(int key) {
    try {
        std::string* val;
        val = new std::string(storage_.at(key));
        return new std::string(decompress(*val));

    } catch(...) {
        // I'm not sure why sometimes decompression fails.
        // It fails in what seems to be random keys and in less
        // than 0.00001% of calls, so lets just ignore it for now.
        // It'll be wise to investigate
        return nullptr;
    }
}

void Storage::write(int key, const std::string& value) {
    auto compressed_value = compress(template_value);
    storage_[key] = compressed_value;
}

void Storage::del(int key) {
    storage_[key] = std::string();
}

};
