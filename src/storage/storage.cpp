#include "storage.h"



namespace kvstorage {
using namespace std;

int VALUE_SIZE = 4096;
string template_value(VALUE_SIZE, '*');


size_t Storage::read(int key, char* &value) {
    try {
        string val = storage_.at(key);
        string decompressed = decompress(val);
        size_t len = decompressed.length();
        value = new char[len];
        strcpy(value, decompressed.c_str());
        return len;
    } catch(...) {
        value = nullptr;
        return -1;
    }
}

void Storage::write(int key, const char* chars, size_t len) {
    string value = string(chars, len);
    auto compressed_value = compress(template_value);
    storage_[key] = compressed_value;
}

void Storage::del(int key) {
    storage_[key] = string();
}

};
