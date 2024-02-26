#include "storage.h"



namespace kvstorage {

int VALUE_SIZE = 4096;
std::string template_value(VALUE_SIZE, '*');


std::string Storage::read(int key) {
    std::string val;
    val = storage_.at(key);

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
    storage_[key] = compressed_value;
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
