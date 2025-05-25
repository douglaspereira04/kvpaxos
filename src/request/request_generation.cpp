#include "request_generation.h"
#include <iostream>
#include "scrambled_zipfian_int_distribution.cpp"
#include "acknowledged_counter.cpp"
#include "skewed_latest_int_distribution.cpp"
#include <unordered_map>
#include <stdio.h>
#include <unistd.h>

namespace workload {




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

void generate_export_requests(
    const toml_config& config
) {
    std::vector<std::pair<request_type, double>> operation_proportions;
    long long n_requests = 0;
    auto export_path = toml::find<std::string>(
        config, "output", "requests", "export_path"
    );

    const auto key_seed = toml::find<long>(
        config, "workload", "key_seed"
    );

    const auto operation_seed = toml::find<long>(
        config, "workload", "operation_seed"
    );

    const auto n_records = toml::find<int>(
        config, "workload", "n_records"
    );
    acknowledged_counter<long> *insertkeysequence = new acknowledged_counter<long>(n_records);
    
    const auto n_operations = toml::find<int>(
        config, "workload", "n_operations"
    );
    n_requests = n_operations;

    const auto data_distribution_str = toml::find<std::string>(
        config, "workload", "data_distribution"
    );

    const auto read_proportion = toml::find<double>(
        config, "workload", "read_proportion"
    );
    if(read_proportion>0){
        operation_proportions.push_back(std::make_pair(request_type::READ,read_proportion));
    }

    const auto scan_proportion = toml::find<double>(
        config, "workload", "scan_proportion"
    );

    const auto update_proportion = toml::find<double>(
        config, "workload", "update_proportion"
    );
    if(update_proportion>0){
        operation_proportions.push_back(std::make_pair(request_type::UPDATE,update_proportion));
    }

    const auto insert_proportion = toml::find<double>(
        config, "workload", "insert_proportion"
    );
    if(insert_proportion>0){
        operation_proportions.push_back(std::make_pair(request_type::WRITE,insert_proportion));
    }

    auto data_distribution = rfunc::string_to_distribution.at(
        data_distribution_str
    );

    rfunc::RandFunction data_generator;
    if (data_distribution == rfunc::UNIFORM) {
        data_generator = rfunc::uniform_distribution_rand(
            0, n_records, key_seed
        );
    } else if (data_distribution == rfunc::ZIPFIAN) {
        int expectednewkeys = (int) ((n_operations) * insert_proportion * 2.0);
        data_generator = rfunc::scrambled_zipfian_distribution(0, n_records + expectednewkeys, key_seed);
    }  else if (data_distribution == rfunc::LATEST) {
        auto zip = new zipfian_int_distribution<long>(0, insertkeysequence->last_value());
        data_generator = rfunc::skewed_latest_distribution(insertkeysequence, zip, key_seed);
        //leaking
    }

    rfunc::RandFunction scan_length_generator;
    if(scan_proportion > 0){

        const auto scan_seed = toml::find<long>(
            config, "workload", "scan_seed"
        );

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
                min_scan_length, max_scan_length, scan_seed
            );
        } else if (scan_length_distribution == rfunc::ZIPFIAN) {
            int expectednewkeys = (int) ((n_operations) * insert_proportion * 2.0);
            scan_length_generator = rfunc::scrambled_zipfian_distribution(0, n_records, scan_seed);
        }

    }

    rfunc::DoubleRandFunction operation_generator = rfunc::uniform_double_distribution_rand(
        0.0, 1.0, operation_seed
    );


    std::ofstream ofs(export_path, std::ofstream::out);
    for (size_t i = 0; i < n_records; i++)
    {
        ofs << static_cast<int>(WRITE) << "," << i << "," << request.args() << "," << std::endl;
    }
    
    for (auto i = 0; i < n_operations; i++) {
        request_type type = next_operation(operation_proportions, &operation_generator);
        std::string value = "";
        int key, size;
        if(type == request_type::READ || type == request_type::UPDATE){
            do{
                key = data_generator();
            } while(key >= insertkeysequence->last_value());
            if(type == request_type::UPDATE){
                type = request_type::WRITE;
            }
        }else if(type == request_type::SCAN){
            size = scan_length_generator();
            n_requests += (size-1);
            do{
                key = data_generator();
            } while(key+size >= insertkeysequence->last_value());
        } else if(type == request_type::WRITE){
            key = insertkeysequence->next();
            insertkeysequence->acknowledge(key);
        }

        if (type == READ) {
            ofs << type << "," << key << std::endl;
        } else if (type == WRITE) {
            ofs << type << "," << key << "," << value << std::endl;
        } else if (type == SCAN) {
            ofs << type << "," << key << "," << size << std::endl;
        }
       
    }
    std::cout << "n_requests: " << n_requests << std::endl; 

    ofs.close();
}

void create_requests(
    std::string config_path
) {
    const auto config = toml::parse(config_path);

    const bool is_one_distribution = toml::find<bool>(
        config, "workload", "single_distribution"
    );

    if(is_one_distribution){
        generate_export_requests(config);
    }else{
    }
}


}
