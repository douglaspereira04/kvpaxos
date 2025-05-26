#include "storage.h"



namespace kvstorage {
using namespace std;

int VALUE_SIZE = 4096;
string template_value(VALUE_SIZE, '*');


int Storage::read(int key, char* &value) {
    string val;
    try {
        val = storage_.at(key);
    } catch(...) {
        value = nullptr;
        return -1;
    }
    string decompressed = decompress(val);
    int len = decompressed.length();
    value = new char[len+1];
    strcpy(value, decompressed.c_str());
    return len;
}

void Storage::write(int key, const char* chars, int len) {
    string value(chars, len);
    auto compressed_value = compress(value);
    storage_[key] = compressed_value;
}

void Storage::del(int key) {
    char null_str[] = "null";
    write(key, null_str, sizeof(null_str)-1);
}

};
