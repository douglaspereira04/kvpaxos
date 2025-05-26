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


#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sstream>
#include <signal.h>
#include <mutex>
#include <netinet/tcp.h>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <random>
#include <assert.h>
#include <boost/lockfree/spsc_queue.hpp>
#include "types/types.h"
#include "utils/utils.h"
#include "graph/graph.hpp"
#include "request/request.hpp"

#include "scheduler/scheduler.hpp"
using namespace workload;

typedef kvpaxos::Scheduler<int, true, TRACK_LENGTH, Q_SIZE, interval_type::MICROSECONDS, MAX_SUCESSIVE_IMBALANCE> Scheduler;


typedef boost::lockfree::spsc_queue<Request*, boost::lockfree::capacity<SCHEDULE_QUEUE_SIZE>> scheduling_queue_t;


static int verbose = 0;
static int SLEEP = 1000;
static bool RUNNING = true;

static const int N_REQUESTS = 1;
static const int N_PARTITIONS = 2;
static const int N_INITIAL_KEYS = 3;
static const int REPARTITION_INTERVAL = 4;
static const int REPARTITION_METHOD = 5;
static const int REQUESTS_PATH = 6;
static const int REQUEST_RATE = 7;
static const int REQUEST_RATE_SEED = 8;
static const int QUEUE_HEAD_DISTANCE = 9;
static const int BALANCE_THRESHOLD = 10;

static char* *params;

static int arrived = 0;

static long request_rate;
static long request_rate_seed;


void
metrics_loop(int sleep_duration, Scheduler* scheduler)
{
	size_t n_requests = atol(params[N_REQUESTS]);
	size_t n_initial_keys = atol(params[N_INITIAL_KEYS]);
	cout << "Executed,Arrivals,Graph Vertices,Graph Edges";
	int n_partitions =  atoi(params[N_PARTITIONS]);
	for (int i = 0; i < n_partitions; i++)
	{
		cout << ", In Queue " << i;
	}
	cout << endl;
	size_t executed_requests = 0;
	while (RUNNING && executed_requests < (n_requests + n_initial_keys)) {
		this_thread::sleep_for(chrono::milliseconds(sleep_duration));
		executed_requests = scheduler->n_executed_requests();
		cout << executed_requests << ",";

		if constexpr(utils::ENABLE_INFO){
			cout << arrived << ",";

			cout << scheduler->graph_vertices() << ",";
			cout << scheduler->graph_edges() << ",";

			vector<size_t> in_queue = scheduler->in_queue_amount();
			for (int i = 0; i < n_partitions; i++)
			{
				cout << in_queue[i] << ",";
			}
		}

		cout << endl;
	}
}

static Scheduler*
initialize_scheduler(ifstream &requests_file)
{
	auto n_partitions = atoi(params[N_PARTITIONS]);
	auto repartition_interval = atoi(params[REPARTITION_INTERVAL]);
	string repartition_method_s = params[REPARTITION_METHOD];

	auto repartition_method = model::string_to_cut_method.at(
		repartition_method_s
	);

	float q_head_distance = atoi(params[QUEUE_HEAD_DISTANCE]);
	float balance_threshold = atof(params[BALANCE_THRESHOLD]);

	auto* scheduler = new Scheduler(
		repartition_interval, n_partitions,
		repartition_method,
		q_head_distance,
		balance_threshold
	);

	scheduler->run();

	auto n_initial_keys = atoi(params[N_INITIAL_KEYS]);
	if (n_initial_keys > 0) {
		for (int i = 0; i < n_initial_keys; i++)
		{
			Request *request;
			read_request(request, requests_file);
			scheduler->submit(request);
		}
		
		while(scheduler->n_executed_requests() < n_initial_keys){
			this_thread::sleep_for(chrono::milliseconds(100));
		}
	}
	return scheduler;
}

void
workload_loop(ifstream &requests_file, Scheduler *scheduler)
{
	size_t n_requests = atol(params[N_REQUESTS]);
	mt19937 generator(request_rate_seed);
	poisson_distribution<long> interval_distribution(1);
	if(request_rate>0){
		interval_distribution = poisson_distribution<long>(1.0E9/request_rate);

		auto begin = utils::now();
		while (requests_file.peek() != EOF) {
			Request *request;
			read_request(request, requests_file);
			scheduler->submit(request);

			if constexpr(utils::ENABLE_INFO){
				arrived++;
			}
			auto duration = chrono::nanoseconds(interval_distribution(generator));
			auto now = utils::now();
			while(now < begin + duration){now = utils::now();}
			begin = now;
		}
	} else {
		while (requests_file.peek() != EOF) {
			Request *request;
			read_request(request, requests_file);
			scheduler->submit(request);

			if constexpr(utils::ENABLE_INFO){
				arrived++;
			}
		}
	}
	Request *end_request = new Request(END);
	scheduler->submit(end_request);
}


static void
run()
{
	
	size_t n_requests = atol(params[N_REQUESTS]);
	request_rate = atol(params[REQUEST_RATE]);
	request_rate_seed = atol(params[REQUEST_RATE_SEED]);
	string requests_path = params[REQUESTS_PATH];
	ifstream requests_file(requests_path);

	auto* scheduler = initialize_scheduler(ref(requests_file));
	
	auto throughput_thread = thread(
		metrics_loop, SLEEP, scheduler
	);
	cpu_set_t throughput_cpu_set;
	utils::set_affinity(0,throughput_thread, throughput_cpu_set);
	
	auto start_execution_timestamp = utils::now();
	auto workload_thread = thread(workload_loop, ref(requests_file), scheduler);
	cpu_set_t workload_cpu_set;
	utils::set_affinity(1,workload_thread, workload_cpu_set);

	scheduler->join();
	workload_thread.join();
	throughput_thread.join();
	requests_file.close();

	auto end_scheduling = scheduler->schedule_end();
	auto end_execution_timestamp = utils::now();


	auto makespan = end_execution_timestamp - start_execution_timestamp;

    ofstream ofs("details.csv", ofstream::out);
	ofs << "Scheduling End," << (end_scheduling - start_execution_timestamp).count()/pow(10,9) << "\n";
	ofs << "Makespan," << makespan.count()/pow(10,9) << "\n";
	ofs << "Error Count," << scheduler->error_count() << "\n";
	if constexpr(utils::ENABLE_INFO){
		auto& repartition_times = scheduler->repartition_timestamps();
		ofs << "Repartition Request, Graph Copy Duration, Repartition Begin, Repartition End, Reconstruction Duration, Apply Time" << endl;
		
		auto copy_time_it = scheduler->graph_copy_duration().begin();
		auto repartition_end_it = scheduler->repartition_end_timestamps().begin();
		auto repartition_request_it = scheduler->repartition_request_timestamp().begin();
		auto repartition_apply_it = scheduler->repartition_apply_timestamp().begin();
		auto reconstruction_it = scheduler->reconstruction_duration().begin();
		for (auto& repartition_time : repartition_times) {
			double end_time = -1;
			double copy_time = -1;
			double repartition_request_time = -1;
			double repartition_apply_time = -1;
			double reconstruction_duration = -1;
			double repartition_begin_time = (repartition_time - start_execution_timestamp).count()/pow(10,9);

			if(repartition_request_it != scheduler->repartition_request_timestamp().end()){
				repartition_request_time = (*repartition_request_it - start_execution_timestamp).count()/pow(10,9);
			}
			repartition_request_it++;

			if(repartition_apply_it != scheduler->repartition_apply_timestamp().end()){
				repartition_apply_time = (*repartition_apply_it - start_execution_timestamp).count()/pow(10,9);
			}
			repartition_apply_it++;

			if(copy_time_it != scheduler->graph_copy_duration().end()){
				copy_time = (*copy_time_it).count()/pow(10,9);
			}
			copy_time_it++;

			if(repartition_end_it != scheduler->repartition_end_timestamps().end()){
				end_time = (*repartition_end_it - start_execution_timestamp).count()/pow(10,9);
			}
			repartition_end_it++;

			if(reconstruction_it != scheduler->reconstruction_duration().end()){
				reconstruction_duration = (*reconstruction_it).count()/pow(10,9);
			}
			reconstruction_it++;

			ofs << repartition_request_time << ","<< copy_time << "," << repartition_begin_time << "," << end_time << ","<< reconstruction_duration << ","<< repartition_apply_time;

			ofs << endl;

		}
	}

	ofs << endl;
	ofs.flush();
    ofs.close();
}

static void
usage(string prog)
{
	cout << "Usage: " << prog << " config\n";
}


int
main(int argc, char const *argv[])
{
	if (argc < 2) {
		usage(string(argv[0]));
		exit(1);
	}


	params = const_cast<char**>(argv);
	run();
	
}
