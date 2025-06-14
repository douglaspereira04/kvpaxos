#include <chrono>
#include <iostream>
#include <thread>
#include <random>
#include "types.h"
#include "utils.h"
#include <fstream>
#include "rocks_db_storage.h"
#include <queue>

static int verbose = 0;
static int SLEEP = 1000;
static bool RUNNING = true;

static const int N_OPERATIONS = 1;
static const int N_INITIAL_KEYS = 2;
static const int OPERATIONS_PATH = 3;
static const int OPERATIONS_RATE = 4;
static const int OPERATIONS_RATE_SEED = 5;
static const int SNAPSHOT_SCAN = 6;

static char* *params;

static int arrived = 0;

static long ops_rate;
static long ops_rate_seed;

static const int VALUE_SIZE = 1024;
static const std::string template_value(VALUE_SIZE, '*');

struct operation_data_t {
	types::RequestType type;
	std::string key;
	size_t len;
	std::string value;
};

static size_t executed = 0;

typedef kvstorage::RocksDBStorage storage_t;


storage_t *storage;

void print_read(std::string &key, std::string &value, std::ofstream &output_file){
	if constexpr(utils::ENABLE_ANSWER){
		output_file << "read( " << key << " ): " << value << "\n";
	}
}

void print_write(std::string &key, const std::string &value, std::ofstream &output_file){
	if constexpr(utils::ENABLE_ANSWER){
		output_file << "write( " << key << ", " << value << " )\n";
	}
}

void print_scan(std::string &key, size_t &len, const std::string* values, std::ofstream &output_file){
	if constexpr(utils::ENABLE_ANSWER){
		output_file << "scan( " << key << ", "<< len << " ): [";
		for (size_t i = 0; i < len; i++)
		{
			output_file << "\"" << values[i] << "\",";
		}
		output_file << "]\n";
	}
}

void print_del(std::string &key, std::ofstream &output_file){
	if constexpr(utils::ENABLE_ANSWER){
		output_file << "del( " << key << " )\n";
	}
}


void metrics_loop(int sleep_duration) {
	size_t n_ops = atol(params[N_OPERATIONS]);
	size_t n_initial_keys = atol(params[N_INITIAL_KEYS]);
	std::cout << "Executed,Arrivals,VM,RSS,Used Disk\n";
	while (RUNNING && executed < (n_ops + n_initial_keys)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
		std::cout << executed << ",";

		std::cout << arrived << ",";
		double vm, rss;
		utils::process_mem_usage(vm, rss);
		std::cout << vm << ",";
		std::cout << rss << ",";
		std::cout << utils::used_disk(".");

		std::cout << "\n";
	}
	std::cout << std::flush;
}


void operate(operation_data_t &operation_data, std::ofstream &output_file){
	types::RequestType type = operation_data.type;
	std::string key = operation_data.key;
	size_t len = operation_data.len;
	std::string value = operation_data.value;

	switch (type)
	{
	case types::READ:
	{
		std::string value;
		storage->read(key, value);
		print_read(key, value, output_file);
		break;
	}
	case types::WRITE:
	{
		if (utils::ENABLE_ANSWER){
			storage->write(key, value);
		} else {
			storage->write(key, template_value);
		}
		print_write(key, value, output_file);
		break;
	}
	case types::SCAN:
	{
		std::vector<std::string> values;
		if (params[SNAPSHOT_SCAN]){
			values = storage->snapshot_scan(key, len);
		} else {
			values = storage->scan(key, len);
		}
		print_scan(key, len, values.data(), output_file);
		break;
	}
	case types::DEL:
	{
		storage->del(key);
		print_del(key, output_file);
		break;
	}
	default:
	{
		std::cout << "ERROR" << std::endl;
		break;
	}
	}
	executed++;
}

void initialize_kvstore(std::queue<operation_data_t> &operation_queue, std::ofstream &output_file) {

	storage  = new storage_t();
	storage->init();
	auto n_initial_keys = atoi(params[N_INITIAL_KEYS]);

	if (n_initial_keys > 0) {
		for (int i = 0; i < n_initial_keys; i++)
		{
			operation_data_t operation_data = std::move(operation_queue.front());
			operation_queue.pop();
			operate(operation_data, output_file);
		}
	}
}

void
workload_loop(std::queue<operation_data_t> &operation_queue, std::ofstream& output_file)
{
	size_t n_ops = atol(params[N_OPERATIONS]);
	std::mt19937 generator(ops_rate_seed);
	std::poisson_distribution<long> interval_distribution(1);
	if(ops_rate>0){
		interval_distribution = std::poisson_distribution<long>(1.0E9/ops_rate);

		auto begin = utils::now();
		for (int i = 0; i < n_ops && operation_queue.size() > 0; i++) {
			operation_data_t operation_data = std::move(operation_queue.front());
			operation_queue.pop();
			operate(operation_data, output_file);
			arrived++;
			auto duration = std::chrono::nanoseconds(interval_distribution(generator));
			auto now = utils::now();
			while(now < begin + duration){now = utils::now();}
			begin = now;
		}
	} else {
		for (int i = 0; i < n_ops && operation_queue.size() > 0; i++) {
			operation_data_t operation_data = std::move(operation_queue.front());
			operation_queue.pop();
			operate(operation_data, output_file);
			arrived++;
		}
	}
}


static void run() {
	std::ofstream output_file("operations_output");
	auto n_initial_keys = atoi(params[N_INITIAL_KEYS]);
	size_t n_ops = atol(params[N_OPERATIONS]);
	ops_rate = atol(params[OPERATIONS_RATE]);
	ops_rate_seed = atol(params[OPERATIONS_RATE_SEED]);
	std::string operations_path = params[OPERATIONS_PATH];
	std::ifstream operations_file(operations_path);
	std::queue<operation_data_t> operation_queue;
	for (int i = 0; i < (n_ops + n_initial_keys) && operations_file.peek() != EOF; i++) {
		operation_data_t operation_data;
		utils::read_operation(operation_data.type, operation_data.key, operation_data.len, operation_data.value, operations_file);
		operation_queue.push(std::move(operation_data));
	}
	operations_file.close();

	initialize_kvstore(ref(operation_queue), output_file);
	
	std::thread throughput_thread = std::thread(
		metrics_loop, SLEEP
	);
	
	auto start_execution_timestamp = utils::now();
	workload_loop(ref(operation_queue), output_file);

	throughput_thread.join();

	auto end_execution_timestamp = utils::now();


	auto makespan = end_execution_timestamp - start_execution_timestamp;

    std::ofstream ofs("details.csv");
	ofs << "Makespan," << makespan.count()/pow(10,9) << "\n";
	ofs.flush();
    ofs.close();
	
}

int
main(int argc, char const *argv[])
{
	if (argc < 2) {
		exit(1);
	}


	params = const_cast<char**>(argv);
	run();
	
}
