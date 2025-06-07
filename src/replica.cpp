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
#include <fstream>
#include <thread>
#include <random>

#include "types.h"
#include "utils.h"
#include "request.hpp"
#include "partition.hpp"

#include <rocksdb/db.h>

using namespace workload;

typedef kvpaxos::Partition<int, Q_SIZE> partition_t;

static int verbose = 0;
static int SLEEP = 1000;
static bool RUNNING = true;

static const int N_REQUESTS = 1;
static const int N_INITIAL_KEYS = 2;
static const int REQUESTS_PATH = 3;
static const int REQUEST_RATE = 4;
static const int REQUEST_RATE_SEED = 5;

static char* *params;

static int arrived = 0;

static long request_rate;
static long request_rate_seed;


void
metrics_loop(int sleep_duration, partition_t* partition)
{
	size_t n_requests = atol(params[N_REQUESTS]);
	size_t n_initial_keys = atol(params[N_INITIAL_KEYS]);
	std::cout << "Executed,Arrivals,VM,RSS,Used Disk,In Queue\n";
	size_t executed_requests = 0;
	while (RUNNING && executed_requests < (n_requests + n_initial_keys)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
		executed_requests = partition->n_executed_requests();
		std::cout << executed_requests << ",";

		if constexpr(utils::ENABLE_INFO){
			std::cout << arrived << ",";
			double vm, rss;
			utils::process_mem_usage(vm, rss);
			std::cout << vm << ",";
			std::cout << rss << ",";
			std::cout << utils::used_disk(".");
			std::cout << partition->request_queue_size() << ",";
		}

		std::cout << "\n";
	}
	std::cout << std::flush;
}

static partition_t*
initialize_partition(std::ifstream &requests_file)
{
	partition_t* partition = new partition_t(0);

	partition->start_worker_thread();

	auto n_initial_keys = atoi(params[N_INITIAL_KEYS]);
	if (n_initial_keys > 0) {
		for (int i = 0; i < n_initial_keys; i++)
		{
			Request *request;
			read_request(request, requests_file);
			partition->push_request(request);
		}
		
		while(partition->n_executed_requests() < n_initial_keys){
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	return partition;
}

void
workload_loop(std::ifstream &requests_file, partition_t *partition)
{
	size_t n_requests = atol(params[N_REQUESTS]);
	std::mt19937 generator(request_rate_seed);
	std::poisson_distribution<long> interval_distribution(1);
	if(request_rate>0){
		interval_distribution = std::poisson_distribution<long>(1.0E9/request_rate);

		auto begin = utils::now();
		for (int i = 0; i < n_requests && requests_file.peek() != EOF; i++) {
			Request *request;
			read_request(request, requests_file);
			partition->push_request(request);

			if constexpr(utils::ENABLE_INFO){
				arrived++;
			}
			auto duration = std::chrono::nanoseconds(interval_distribution(generator));
			auto now = utils::now();
			while(now < begin + duration){now = utils::now();}
			begin = now;
		}
	} else {
		for (int i = 0; i < n_requests && requests_file.peek() != EOF; i++) {
			Request *request;
			read_request(request, requests_file);
            partition->push_request(request);

			if constexpr(utils::ENABLE_INFO){
				arrived++;
			}
		}
	}
	Request *end_request = new Request(END);
	partition->push_request(end_request);
}


static void
run()
{
	
	size_t n_requests = atol(params[N_REQUESTS]);
	request_rate = atol(params[REQUEST_RATE]);
	request_rate_seed = atol(params[REQUEST_RATE_SEED]);
	std::string requests_path = params[REQUESTS_PATH];
	std::ifstream requests_file(requests_path);

	partition_t* partition = initialize_partition(ref(requests_file));
	
	auto throughput_thread = std::thread(
		metrics_loop, SLEEP, partition
	);
	cpu_set_t throughput_cpu_set;
	utils::set_affinity(0,throughput_thread, throughput_cpu_set);
	
	auto start_execution_timestamp = utils::now();
	auto workload_thread = std::thread(workload_loop, ref(requests_file), partition);
	cpu_set_t workload_cpu_set;
	utils::set_affinity(1,workload_thread, workload_cpu_set);
	
	workload_thread.join();
	throughput_thread.join();
	partition->join();
	requests_file.close();

	auto end_execution_timestamp = utils::now();


	auto makespan = end_execution_timestamp - start_execution_timestamp;

    std::ofstream ofs("details.csv");
	ofs << "Makespan," << makespan.count()/pow(10,9) << "\n";
	ofs << "Error Count," << partition->error_count() << "\n";

	ofs << "\n";
	ofs.flush();
    ofs.close();
	delete partition;
	
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
