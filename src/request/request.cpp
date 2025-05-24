#include "request.hpp"


namespace workload {
    using namespace std;

    void read_request(Request* &request, ifstream &file)
    {    
        string chars;
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
            getline(file, chars);
            request = new Request(type, key, chars);
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