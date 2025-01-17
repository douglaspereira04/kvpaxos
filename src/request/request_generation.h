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
#include "request.hpp"

namespace workload {

typedef toml::basic_value<toml::discard_comments, std::unordered_map> toml_config;

/*
Those generations were made for a simpler execution that doesn't differentiate
request's commands, so it's no longer compatible. The code is commented since
there may be a need to adapt it to the newer requests soon
*/

std::vector<Request> create_requests(std::string config_path);
std::vector<workload::Request> generate_single_data_requests(
    const toml_config& config
);
std::vector<workload::Request> generate_multi_data_requests(
    const toml_config& config
);
std::vector<Request> random_single_data_requests(
    int n_requests,
    rfunc::RandFunction& data_rand,
     rfunc::RandFunction& type_rand, 
     double read_proportion
);
/*
std::vector<Request> generate_fixed_data_requests(
    int n_variables, int requests_per_variable
);*/
std::vector<Request> random_multi_data_requests(
    int n_requests,
    int n_variables,
    rfunc::RandFunction& data_rand,
    rfunc::RandFunction& size_rand
);
void shuffle_requests(std::vector<Request>& requests);
std::vector<Request> merge_requests(
    std::vector<Request> single_data_requests,
    std::vector<Request> multi_data_requests,
    int single_data_pick_probability
);

request_type next_operation(
    std::vector<std::pair<request_type,double>> values, 
    rfunc::DoubleRandFunction *generator
);

std::vector<workload::Request> generate_requests(
    const toml_config& config
);

Request import_cs_request(std::ifstream &file);

void export_requests(std::vector<Request> requests, std::string output_path);
}


#endif
