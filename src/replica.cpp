/*
 * Copyright (c) 2014-2015, University of Lugano
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the names of it
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <chrono>
#include <iostream>
#include <thread>
#include <random>
#include "types.h"
#include "utils.h"
#include "operation.hpp"
#include <fstream>

#include "kvstore.hpp"
using namespace workload;

#if defined(REPARTITIONING)
	const bool ENABLE_REPARTITION = true;
#else
    const bool ENABLE_REPARTITION = false;
#endif

typedef kvpaxos::KVStore<int, ENABLE_REPARTITION, TRACK_LENGTH, Q_SIZE, types::OPERATIONS> KVStore;


static int verbose = 0;
static int SLEEP = 1000;
static bool RUNNING = true;

static const int N_OPERATIONS = 1;
static const int N_PARTITIONS = 2;
static const int N_INITIAL_KEYS = 3;
static const int REPARTITION_INTERVAL = 4;
static const int REPARTITION_METHOD = 5;
static const int OPERATIONS_PATH = 6;
static const int OPERATIONS_RATE = 7;
static const int OPERATIONS_RATE_SEED = 8;
static const int QUEUE_HEAD_DISTANCE = 9;

static char* *params;

static int arrived = 0;

static long ops_rate;
static long ops_rate_seed;

static const int VALUE_SIZE = 1024;
static const std::string template_value(VALUE_SIZE, '*');


void
metrics_loop(int sleep_duration, KVStore* kvstore)
{
	size_t n_ops = atol(params[N_OPERATIONS]);
	size_t n_initial_keys = atol(params[N_INITIAL_KEYS]);
	std::cout << "Executed,Arrivals,VM,RSS,Used Disk,Graph Vertices,Graph Edges";
	int n_partitions =  atoi(params[N_PARTITIONS]);
	for (int i = 0; i < n_partitions; i++)
	{
		std::cout << ", In Queue " << i;
	}
	std::cout << "\n";
	size_t executed = 0;
	while (RUNNING && executed < (n_ops + n_initial_keys)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
		executed = kvstore->n_executed_operations();
		std::cout << executed << ",";

		if constexpr(utils::ENABLE_INFO){
			std::cout << arrived << ",";
			double vm, rss;
			utils::process_mem_usage(vm, rss);
			std::cout << vm << ",";
			std::cout << rss << ",";
			std::cout << utils::used_disk(".");

			std::cout << kvstore->graph_vertices() << ",";
			std::cout << kvstore->graph_edges() << ",";

			std::vector<size_t> in_queue = kvstore->in_queue_amount();
			for (int i = 0; i < n_partitions; i++)
			{
				std::cout << in_queue[i] << ",";
			}
		}

		std::cout << "\n";
	}
	std::cout << std::flush;
}

void do_nothing_with_kv(int key, std::string *value){}
void do_nothing_with_k(int key){}

void operation_from_file(KVStore *kvstore, std::ifstream &operations_file){
	types::RequestType type;
	int key;
	size_t len;
	std::string value;
	utils::read_operation(type, key, len, value, operations_file);

	switch (type)
	{
	case types::READ:
		kvstore->get(key, do_nothing_with_kv);
		break;
	case types::WRITE:
		if (utils::ENABLE_ANSWER){
			kvstore->set(key, value, do_nothing_with_kv);
		} else {
			kvstore->set(key, template_value, do_nothing_with_kv);
		}
		break;
	case types::SCAN:
		kvstore->scan(key, len, do_nothing_with_kv);
		break;
	case types::DEL:
		kvstore->del(key, do_nothing_with_k);
		break;
	default:
		std::cout << "ERROR" << std::endl;
		break;
	}
}

static KVStore*
initialize_kvstore(std::ifstream &operations_file)
{
	auto n_partitions = atoi(params[N_PARTITIONS]);
	auto repartition_interval = atoi(params[REPARTITION_INTERVAL]);
	std::string repartition_method_s = params[REPARTITION_METHOD];

	auto repartition_method = model::string_to_cut_method.at(
		repartition_method_s
	);

	float q_head_distance = atoi(params[QUEUE_HEAD_DISTANCE]);

	KVStore* kvstore = new KVStore(
		repartition_interval, n_partitions,
		repartition_method,
		q_head_distance
	);

	kvstore->run();

	auto n_initial_keys = atoi(params[N_INITIAL_KEYS]);
	if (n_initial_keys > 0) {
		for (int i = 0; i < n_initial_keys; i++)
		{
			operation_from_file(kvstore, operations_file);
		}
		
		bool wait = true;
		while(wait){
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			size_t executed = kvstore->n_executed_operations();
			if (ENABLE_REPARTITION){
				size_t processed = kvstore->n_processed_operations();
				wait = executed < n_initial_keys || processed < n_initial_keys;
			} else {
				wait = executed < n_initial_keys;
			}
		}
	}
	return kvstore;
}

void
workload_loop(std::ifstream &operations_file, KVStore *kvstore)
{
	size_t n_ops = atol(params[N_OPERATIONS]);
	std::mt19937 generator(ops_rate_seed);
	std::poisson_distribution<long> interval_distribution(1);
	if(ops_rate>0){
		interval_distribution = std::poisson_distribution<long>(1.0E9/ops_rate);

		auto begin = utils::now();
		for (int i = 0; i < n_ops && operations_file.peek() != EOF; i++) {
			operation_from_file(kvstore, operations_file);

			if constexpr(utils::ENABLE_INFO){
				arrived++;
			}
			auto duration = std::chrono::nanoseconds(interval_distribution(generator));
			auto now = utils::now();
			while(now < begin + duration){now = utils::now();}
			begin = now;
		}
	} else {
		for (int i = 0; i < n_ops && operations_file.peek() != EOF; i++) {
			operation_from_file(kvstore, operations_file);
			if constexpr(utils::ENABLE_INFO){
				arrived++;
			}
		}
	}
	kvstore->stop();
}


static void
run()
{
	
	size_t n_ops = atol(params[N_OPERATIONS]);
	ops_rate = atol(params[OPERATIONS_RATE]);
	ops_rate_seed = atol(params[OPERATIONS_RATE_SEED]);
	std::string operations_path = params[OPERATIONS_PATH];
	std::ifstream operations_file(operations_path);

	KVStore* kvstore = initialize_kvstore(ref(operations_file));
	
	std::thread throughput_thread = std::thread(
		metrics_loop, SLEEP, kvstore
	);
	cpu_set_t throughput_cpu_set;
	utils::set_affinity(0,throughput_thread, throughput_cpu_set);
	
	auto start_execution_timestamp = utils::now();
	auto workload_thread = std::thread(workload_loop, ref(operations_file), kvstore);
	cpu_set_t workload_cpu_set;
	utils::set_affinity(1,workload_thread, workload_cpu_set);
	workload_thread.join();
	throughput_thread.join();
	kvstore->join();
	operations_file.close();

	auto end_scheduling = kvstore->schedule_end();
	auto end_execution_timestamp = utils::now();


	auto makespan = end_execution_timestamp - start_execution_timestamp;

    std::ofstream ofs("details.csv");
	ofs << "Scheduling End," << (end_scheduling - start_execution_timestamp).count()/pow(10,9) << "\n";
	ofs << "Makespan," << makespan.count()/pow(10,9) << "\n";
	ofs << "Error Count," << kvstore->error_count() << "\n";
	if constexpr(utils::ENABLE_INFO){
		auto& repartition_times = kvstore->repartition_timestamps();
		ofs << "Repartition Request, Graph Copy Duration, Repartition Begin, Repartition End, Reconstruction Duration, Apply Time\n";
		
		auto copy_time_it = kvstore->graph_copy_duration().begin();
		auto repartition_end_it = kvstore->repartition_end_timestamps().begin();
		auto repartition_request_it = kvstore->repartition_request_timestamp().begin();
		auto repartition_apply_it = kvstore->repartition_apply_timestamp().begin();
		auto reconstruction_it = kvstore->reconstruction_duration().begin();
		for (auto& repartition_time : repartition_times) {
			double end_time = -1;
			double copy_time = -1;
			double repartition_request_time = -1;
			double repartition_apply_time = -1;
			double reconstruction_duration = -1;
			double repartition_begin_time = (repartition_time - start_execution_timestamp).count()/pow(10,9);

			if(repartition_request_it != kvstore->repartition_request_timestamp().end()){
				repartition_request_time = (*repartition_request_it - start_execution_timestamp).count()/pow(10,9);
			}
			repartition_request_it++;

			if(repartition_apply_it != kvstore->repartition_apply_timestamp().end()){
				repartition_apply_time = (*repartition_apply_it - start_execution_timestamp).count()/pow(10,9);
			}
			repartition_apply_it++;

			if(copy_time_it != kvstore->graph_copy_duration().end()){
				copy_time = (*copy_time_it).count()/pow(10,9);
			}
			copy_time_it++;

			if(repartition_end_it != kvstore->repartition_end_timestamps().end()){
				end_time = (*repartition_end_it - start_execution_timestamp).count()/pow(10,9);
			}
			repartition_end_it++;

			if(reconstruction_it != kvstore->reconstruction_duration().end()){
				reconstruction_duration = (*reconstruction_it).count()/pow(10,9);
			}
			reconstruction_it++;

			ofs << repartition_request_time << ","<< copy_time << "," << repartition_begin_time << "," << end_time << ","<< reconstruction_duration << ","<< repartition_apply_time;

			ofs << "\n";

		}
	}

	ofs << "\n";
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
