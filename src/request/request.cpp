#include "request.hpp"


namespace workload {
    Request make_request(int &type_buffer, int &key_buffer, int &arg_buffer) {
        auto type = static_cast<request_type>(type_buffer);
        auto key = key_buffer;
        auto arg = std::to_string(arg_buffer);

        return Request(type, key, arg);
    }

    Request import_cs_request(std::ifstream &file)
    {    
        std::string line;
        int type, key, arg;
        std::getline(file, line);
        sscanf(line.c_str(), "%d,%d,%d", &type,&key,&arg);
        return make_request(
            type,
            key,
            arg
        );
    }
}