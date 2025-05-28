#include "request.hpp"


namespace workload {

    void read_request(Request* &request, std::ifstream &file)
    {    
        std::string chars;
        int key;
        getline(file, chars, ',');
        RequestType type = static_cast<RequestType>(atoi(chars.c_str()));
        if (type == READ){
            getline(file, chars);
            key = atoi(chars.c_str());
            request = new Request(type, key);
        } else if(type == WRITE) {
            getline(file, chars, ',');
            key = atoi(chars.c_str());
            std::string *value = new std::string();
            getline(file, *value);
            request = new Request(type, key, value);
        } else if(type == SCAN) {
            getline(file, chars, ',');
            key = atoi(chars.c_str());
            getline(file, chars);
            size_t len = atol(chars.c_str());
            request = new Request(type, key, len);
        } else {
            request = new Request();
        }
    }
}