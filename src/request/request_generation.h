#ifndef WORKLOAD_REQUEST_GENERATOR_H
#define WORKLOAD_REQUEST_GENERATOR_H

#include <algorithm>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>
#include <unordered_set>
#include <vector>

#include <toml11/toml.hpp>
#include "random.h"

namespace workload {

typedef toml::basic_value<toml::discard_comments, std::unordered_map> toml_config;

void create_requests(std::string config_path);

request_type next_operation(
    std::vector<std::pair<request_type,double>> values, 
    rfunc::DoubleRandFunction *generator
);

}


#endif
