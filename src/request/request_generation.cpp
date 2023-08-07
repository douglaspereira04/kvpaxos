#include "request_generation.h"
#include <iostream>
#include "scrambled_zipfian_int_distribution.cpp"
#include <unordered_map>

namespace workload {

Request make_request(char* type_buffer, char* key_buffer, char* arg_buffer) {
    auto type = static_cast<request_type>(std::stoi(type_buffer));
    auto key = std::stoi(key_buffer);
    auto arg = std::string(arg_buffer);

    return Request(type, key, arg);
}

std::vector<Request> import_cs_requests(const std::string& file_path)
{
    std::ifstream infile(file_path);
    std::vector<Request> requests;
    std::string line;
    char type_buffer[2];
    char key_buffer[11];
    char arg_buffer[129];
    auto* reading_buffer = type_buffer;
    auto buffer_index = 0;
    while (std::getline(infile, line)) {
        for (auto& character: line) {
            if (character == ',') {
                reading_buffer[buffer_index] = '\0';
                if (reading_buffer == type_buffer) {
                    reading_buffer = key_buffer;
                } else if (reading_buffer == key_buffer) {
                    reading_buffer = arg_buffer;
                } else {
                    reading_buffer = type_buffer;
                    Request request = make_request(
                        type_buffer,
                        key_buffer,
                        arg_buffer
                    );
                    requests.emplace_back(request);
                }
                buffer_index = 0;
            } else {
                reading_buffer[buffer_index] = character;
                buffer_index++;
            }
        }
    }
    return requests;
}

std::vector<Request> import_requests(const std::string& file_path,
    const std::string& field)
    {
    const auto file = toml::parse(file_path);
    auto str_requests = toml::find<std::vector<std::vector<std::string>>>(
        file, field
    );

    auto requests = std::vector<Request>();
    for (auto& request : str_requests) {
        auto type = static_cast<request_type>(std::stoi(request[0]));
        auto key = std::stoi(request[1]);
        auto arg = request[2];

        requests.push_back(Request(type, key, arg));
    }

    return requests;
}

std::vector<Request> random_single_data_requests(
    int n_requests, rfunc::RandFunction& data_rand, rfunc::DoubleRandFunction& type_rand, double read_proportion
) {
    auto requests = std::vector<Request>();
    for (auto i = 0; i < n_requests; i++) {
        request_type type = request_type::READ;
        auto rand = type_rand();
        if(rand > read_proportion){
            type = request_type::WRITE;
        }
        auto key = data_rand();
        auto request = Request(type, key, "");
        requests.push_back(request);
    }
    return requests;
}

/*
std::vector<Request> generate_fixed_data_requests(
    int n_variables, int requests_per_variable
) {
    auto requests = std::vector<Request>();
    for (auto i = 0; i < n_variables; i++) {
        for (auto j = 0; j < requests_per_variable; j++) {
            auto request = Request();
            request.insert(i);
            requests.push_back(request);
        }
    }

    shuffle_requests(requests);
    return requests;
}*/

std::vector<Request> random_multi_data_requests(
    int n_requests,
    int n_variables,
    rfunc::RandFunction& data_rand,
    rfunc::RandFunction& size_rand
) {
    auto requests = std::vector<Request>();
    request_type type = request_type::SCAN;
    
    for (auto i = 0; i < n_requests; i++) {
        auto key = data_rand();
        auto size = std::to_string(size_rand());
        auto request = Request(type, key, size);
        requests.push_back(request);
    }
    return requests;
}

std::vector<workload::Request> generate_single_data_requests(
    const toml_config& config
) {
    const auto single_data_distributions = toml::find<std::vector<std::string>>(
        config, "workload", "requests", "single_data", "distribution_pattern"
    );
    const auto read_proportion = toml::find<std::vector<double>>(
        config, "workload", "requests", "single_data", "read_proportion"
    );

    const auto insert_proportion = toml::find<double>(
        config, "workload", "write_proportion"
    );

    const auto n_initial_keys = toml::find<int>(
        config, "workload", "n_initial_keys"
    );

    const auto n_requests = toml::find<std::vector<int>>(
        config, "workload", "requests", "single_data", "n_requests"
    );
    const auto n_variables = toml::find<int>(
        config, "workload", "n_variables"
    );

    auto requests = std::vector<workload::Request>();
    auto binomial_counter = 0;
    for (auto i = 0; i < n_requests.size(); i++) {
        auto current_distribution_ = single_data_distributions[i];
        auto current_distribution = rfunc::string_to_distribution.at(
            current_distribution_
        );

        auto new_requests = std::vector<workload::Request>();
        /*if (current_distribution == rfunc::FIXED) {
            const auto requests_per_data = floor(n_variables);

            new_requests = workload::generate_fixed_data_requests(
                n_variables, requests_per_data
            );
        } else */if (current_distribution == rfunc::UNIFORM) {
            auto data_rand = rfunc::uniform_distribution_rand(
                0, n_variables-1
            );

            auto type_rand = rfunc::uniform_double_distribution_rand(
                0.0, 1.0
            );

            new_requests = workload::random_single_data_requests(
                n_requests[i], data_rand, type_rand, read_proportion[i]
            );
        } else if (current_distribution == rfunc::ZIPFIAN) {
            int expectednewkeys = (int) ((n_requests[i]) * insert_proportion * 2.0);
            auto data_rand = rfunc::scrambled_zipfian_distribution(0, n_initial_keys + expectednewkeys);

            auto type_rand = rfunc::uniform_double_distribution_rand(
                0.0, 1.0
            );

            new_requests = workload::random_single_data_requests(
                n_requests[i], data_rand, type_rand, read_proportion[i]
            );
        } else if (current_distribution == rfunc::BINOMIAL) {
            const auto success_probability = toml::find<std::vector<double>>(
                config, "workload", "requests", "single_data", "success_probability"
            );
            auto data_rand = rfunc::binomial_distribution(
                n_variables-1, success_probability[binomial_counter]
            );


            auto type_rand = rfunc::uniform_double_distribution_rand(
                0.0, 1.0
            );


            new_requests = workload::random_single_data_requests(
                n_requests[i], data_rand, type_rand, read_proportion[i]
            );
            binomial_counter++;
        }

        requests.insert(requests.end(), new_requests.begin(), new_requests.end());
    }

    return requests;
}

std::vector<workload::Request> generate_multi_data_requests(
    const toml_config& config
) {
    const auto n_requests = toml::find<std::vector<int>>(
        config, "workload", "requests", "multi_data", "n_requests"
    );

    const auto min_involved_data = toml::find<std::vector<int>>(
        config, "workload", "requests", "multi_data",
        "min_involved_data"
    );


    const auto insert_proportion = toml::find<std::vector<double>>(
        config, "workload", "requests", "multi_data",
        "write_proportion"
    );

    const auto max_involved_data = toml::find<std::vector<int>>(
        config, "workload", "requests", "multi_data",
        "max_involved_data"
    );

    const auto size_distribution_ = toml::find<std::vector<std::string>>(
        config, "workload", "requests", "multi_data", "size_distribution_pattern"
    );
    const auto data_distribution_ = toml::find<std::vector<std::string>>(
        config, "workload", "requests", "multi_data", "data_distribution_pattern"
    );
    const auto n_variables = toml::find<int>(
        config, "workload", "n_variables"
    );

    auto size_binomial_counter = 0;
    auto data_binomial_counter = 0;
    auto requests = std::vector<workload::Request>();
    for (auto i = 0; i < n_requests.size(); i++) {
        auto new_requests = std::vector<workload::Request>();

        auto size_distribution = rfunc::string_to_distribution.at(
            size_distribution_[i]
        );
        rfunc::RandFunction size_rand;
        if (size_distribution == rfunc::UNIFORM) {
            size_rand = rfunc::uniform_distribution_rand(
                min_involved_data[i], max_involved_data[i]
            );
        } /*else if (size_distribution == rfunc::FIXED) {
            size_rand = rfunc::fixed_distribution(max_involved_data[i]);
        } */else if (size_distribution == rfunc::BINOMIAL) {
            const auto success_probability = toml::find<std::vector<double>>(
                config, "workload", "requests", "multi_data",
                "size_success_probability"
            );
            size_rand = rfunc::ranged_binomial_distribution(
                min_involved_data[i], max_involved_data[i],
                success_probability[size_binomial_counter]
            );
            size_binomial_counter++;
        }

        auto data_distribution = rfunc::string_to_distribution.at(
            data_distribution_[i]
        );
        rfunc::RandFunction data_rand;
        if (data_distribution == rfunc::UNIFORM) {
            data_rand = rfunc::uniform_distribution_rand(0, n_variables-1);
        } else if (data_distribution == rfunc::ZIPFIAN) {
            
            int expectednewkeys = (int) ((n_requests[i]) * insert_proportion[i] * 2.0);

            auto data_rand = rfunc::scrambled_zipfian_distribution(0, expectednewkeys);
        } else if (data_distribution == rfunc::BINOMIAL) {
            const auto success_probability = toml::find<std::vector<double>>(
                config, "workload", "requests", "multi_data",
                "data_success_probability"
            );
            data_rand = rfunc::binomial_distribution(
                n_variables-1, success_probability[data_binomial_counter]
            );
            data_binomial_counter++;
        }

        new_requests = workload::random_multi_data_requests(
            n_requests[i],
            n_variables,
            data_rand,
            size_rand
        );
        requests.insert(requests.end(), new_requests.begin(), new_requests.end());
    }

    return requests;
}


void shuffle_requests(std::vector<Request>& requests) {
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(requests), std::end(requests), rng);
}

std::vector<Request> merge_requests(
    std::vector<Request> single_data_requests,
    std::vector<Request> multi_data_requests,
    double single_data_pick_probability
) {
    auto shuffled_requests = std::vector<Request>();
    auto rand = rfunc::uniform_double_distribution_rand(0.0, 1.0);
    std::reverse(single_data_requests.begin(), single_data_requests.end());
    std::reverse(multi_data_requests.begin(), multi_data_requests.end());

    while (!single_data_requests.empty() and !multi_data_requests.empty()) {
        auto random_value = rand();

        if (single_data_pick_probability > random_value) {
            shuffled_requests.push_back(single_data_requests.back());
            single_data_requests.pop_back();
        } else {
            shuffled_requests.push_back(multi_data_requests.back());
            multi_data_requests.pop_back();
        }

    }

    while (!single_data_requests.empty()) {
        shuffled_requests.push_back(single_data_requests.back());
        single_data_requests.pop_back();
    }
    while (!multi_data_requests.empty()) {
        shuffled_requests.push_back(multi_data_requests.back());
        multi_data_requests.pop_back();
    }

    return shuffled_requests;
}

void export_requests(std::vector<Request> requests, std::string output_path){
    std::ofstream ofs(output_path, std::ofstream::out);
    
    for (auto request : requests) {
        ofs << static_cast<int>(request.type()) << "," << request.key() << "," << request.args() << "," << std::endl;
    }

    ofs.close();
}


request_type next_operation(
    std::vector<std::pair<request_type,double>> values, 
    rfunc::DoubleRandFunction *generator
) {
    double sum = 0;
    
    for (size_t i = 0; i < values.size(); i++) {
       sum += values[i].second;
    }

    double val = (*generator)();

    for (size_t i = 0; i < values.size(); i++) {
        double vw = values[i].second / sum;
        if (val < vw) {
            return values[i].first;
        }

        val -= vw;
    }

    throw std::invalid_argument("Something went wrong");

}

std::vector<workload::Request> generate_requests(
    const toml_config& config
) {
    std::vector<workload::Request> requests;
    std::vector<std::pair<request_type, double>> operation_proportions;

    const auto n_records = toml::find<int>(
        config, "workload", "n_records"
    );
    
    const auto n_operations = toml::find<int>(
        config, "workload", "n_operations"
    );

    const auto data_distribution_str = toml::find<std::string>(
        config, "workload", "data_distribution"
    );

    const auto read_proportion = toml::find<double>(
        config, "workload", "read_proportion"
    );
    operation_proportions.push_back(std::make_pair(request_type::READ,read_proportion));
    
    const auto scan_proportion = toml::find<double>(
        config, "workload", "scan_proportion"
    );

    const auto insert_proportion = toml::find<double>(
        config, "workload", "insert_proportion"
    );
    operation_proportions.push_back(std::make_pair(request_type::WRITE,insert_proportion));
    

    auto data_distribution = rfunc::string_to_distribution.at(
        data_distribution_str
    );

    rfunc::RandFunction data_generator;
    if (data_distribution == rfunc::UNIFORM) {
        data_generator = rfunc::uniform_distribution_rand(
            0, n_records
        );
    } else if (data_distribution == rfunc::ZIPFIAN) {
        int expectednewkeys = (int) ((n_operations) * insert_proportion * 2.0);
        data_generator = rfunc::scrambled_zipfian_distribution(0, n_records);
    }

    rfunc::RandFunction scan_length_generator;
    if(scan_proportion > 0){
        operation_proportions.push_back(std::make_pair(request_type::SCAN,scan_proportion));
    
        const auto scan_length_distribution_str = toml::find<std::string>(
            config, "workload", "scan_length_distribution"
        );
        
        const auto min_scan_length = toml::find<int>(
            config, "workload", "min_scan_length"
        );

        const auto max_scan_length = toml::find<int>(
            config, "workload", "max_scan_length"
        );
        auto scan_length_distribution = rfunc::string_to_distribution.at(
            scan_length_distribution_str
        );

        if (scan_length_distribution == rfunc::UNIFORM) {
            scan_length_generator = rfunc::uniform_distribution_rand(
                min_scan_length, max_scan_length
            );
        } else if (scan_length_distribution == rfunc::ZIPFIAN) {
            int expectednewkeys = (int) ((n_operations) * insert_proportion * 2.0);
            scan_length_generator = rfunc::scrambled_zipfian_distribution(0, n_records);
        }

    }

    rfunc::DoubleRandFunction operation_generator = rfunc::uniform_double_distribution_rand(
        0.0, 1.0
    );
    
    for (auto i = 0; i < n_operations; i++) {
        request_type type = next_operation(operation_proportions, &operation_generator);
        
        auto key = data_generator();
        std::string size = "";
        if(type == request_type::SCAN){
            size = std::to_string(scan_length_generator());
        }
        auto request = Request(type, key, size);
        requests.push_back(request);
    }

    return requests;
}

std::vector<Request> create_requests(
    std::string config_path
) {
    const auto config = toml::parse(config_path);

    const auto is_one_distribution = toml::find<bool>(
        config, "workload", "single_distribution"
    );

    std::vector<workload::Request> requests;

    if(is_one_distribution){
        requests = generate_requests(config);
    }else{
        /*
            O programa utilizava instâncias diferentes de geradores de números para requisições
            "Single Data" e "Multiple Data", o que pode não ser o desejado ao utilizar a distribuição 
            "Scrambled Zipfian".
        */

        /*auto single_data_requests = generate_single_data_requests(config);
        auto multi_data_requests = generate_multi_data_requests(config);

        const auto single_data_pick_probability = toml::find<double>(
            config, "workload", "requests", "single_data_pick_probability"
        );

        requests = workload::merge_requests(
            single_data_requests,
            multi_data_requests,
            single_data_pick_probability
        );*/
    }

    return requests;
}


}
