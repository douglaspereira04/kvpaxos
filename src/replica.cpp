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

#include <cds/init.h>
#include <cds/gc/hp.h>

#include "request/skewed_latest_int_distribution.cpp"
#include "request/request_generation.h"
#include "types/types.h"
#include "graph/graph.hpp"


#if defined(FREE)
	#include "scheduler/free_scheduler.hpp"
	#define Scheduler FreeScheduler
#elif defined(CARELESS)
	#include "scheduler/careless_scheduler.hpp"
	#define Scheduler CarelessScheduler
#elif defined(NON_STOP)
	#include "scheduler/non_stop_scheduler.hpp"
	#define Scheduler NonStopScheduler
#elif defined(NON_STOP_JUMP)
	#include "scheduler/non_stop_jump_scheduler.hpp"
	#define Scheduler NonStopJumpScheduler
#else
	#include "scheduler/scheduler.hpp"
#endif


using toml_config = toml::basic_value<
	toml::discard_comments, std::unordered_map, std::vector
>;

static int verbose = 0;
static int SLEEP = 1000;
static bool RUNNING = true;

int N_PARTITIONS = 2;
int N_INITIAL_KEYS = 3;
int REPARTITION_INTERVAL = 4;
int REPARTITION_METHOD = 5;
int REQUESTS_PATH = 6;

char* *params;

void
metrics_loop(int sleep_duration, int n_requests, kvpaxos::Scheduler<int>* scheduler)
{
	auto already_counted_throughput = 0;
	auto counter = 0;
	std::cout << "Sec,Throughput,Graph Vertices,Graph Edges\n";
	while (RUNNING) {
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
		auto executed_requests = scheduler->n_executed_requests();
		auto graph_vertices = scheduler->graph_vertices();
		auto graph_edges = scheduler->graph_edges();
		auto throughput = executed_requests - already_counted_throughput;
		std::cout << counter << ",";
		std::cout << throughput << ",";
		std::cout << graph_vertices << ",";
		std::cout << graph_edges << "\n";
		already_counted_throughput += throughput;
		counter++;

		if (executed_requests >= n_requests) {
			break;
		}
	}
}

static kvpaxos::Scheduler<int>*
initialize_scheduler(
	int n_requests,
	const toml_config& config)
{
	auto n_partitions = atoi(params[N_PARTITIONS]);
	auto repartition_interval = atoi(params[REPARTITION_INTERVAL]);
	std::string repartition_method_s = params[REPARTITION_METHOD];

	auto repartition_method = model::string_to_cut_method.at(
		repartition_method_s
	);

	auto* scheduler = new kvpaxos::Scheduler<int>(
		n_requests, repartition_interval, n_partitions,
		repartition_method
	);
	auto n_initial_keys = atoi(params[N_INITIAL_KEYS]);

	for (auto i = 0; i <= n_initial_keys; i++) {
		struct client_message client_message;
		client_message.key = i;
		scheduler->process_populate_request(client_message);
	}
	scheduler->wait_populate();

	scheduler->run();
	return scheduler;
}
static int client_message_id = 0;
struct client_message
to_client_message(
	workload::Request& request)
{
	struct client_message client_message;
	client_message.sin_port = htons(0);
	client_message.id = client_message_id;
	client_message.type = request.type();
	client_message.key = request.key();
	for (auto i = 0; i < request.args().size(); i++) {
		client_message.args[i] = request.args()[i];
	}
	client_message.args[request.args().size()] = 0;
	client_message.size = request.args().size();
	client_message_id++;
	return client_message;
}

std::vector<struct client_message>
to_client_messages(
	std::vector<workload::Request>& requests)
{
	std::vector<struct client_message> client_messages;
	auto counter = 0;
	for (auto i = 0; i < requests.size(); i++) {
		auto& request = requests[i];
		struct client_message client_message;
		client_message.sin_port = htons(0);
		client_message.id = i;
		client_message.type = request.type();
		client_message.key = request.key();
		for (auto i = 0; i < request.args().size(); i++) {
			client_message.args[i] = request.args()[i];
		}
		client_message.args[request.args().size()] = 0;
		client_message.size = request.args().size();

		client_messages.emplace_back(client_message);
	}

	return client_messages;
}

void
execute_requests(
	kvpaxos::Scheduler<int>& scheduler,
	std::vector<client_message> *messages)
{	
	
	for(auto message = messages->begin(); message != messages->end(); message++){
		scheduler.schedule_and_answer(*message);
	}


}

std::unordered_map<int, time_point>
join_maps(std::vector<std::unordered_map<int, time_point>> maps) {
	std::unordered_map<int, time_point> joined_map;
	for (auto& map: maps) {
		joined_map.insert(map.begin(), map.end());
	}
	return joined_map;
}

static void
run(const toml_config& config)
{
	std::string requests_path = params[REQUESTS_PATH];
	
	
	std::ifstream requests_file(requests_path);
	std::vector<client_message> *messages = new std::vector<client_message>();
	while (requests_file.peek() != EOF) {
		workload::Request request= workload::import_cs_request(requests_file);
		client_message message = to_client_message(request);
		messages->push_back(message);
	}
	requests_file.close();
	int n_requests = messages->size();

	auto* scheduler = initialize_scheduler(n_requests, config);
	auto throughput_thread = std::thread(
		metrics_loop, SLEEP, n_requests, scheduler
	);
	
	auto start_execution_timestamp = std::chrono::system_clock::now();
	execute_requests(*scheduler, messages);

	auto end_scheduling = std::chrono::system_clock::now();

	throughput_thread.join();
	auto end_execution_timestamp = std::chrono::system_clock::now();

	delete messages;

	auto makespan = end_execution_timestamp - start_execution_timestamp;


    std::ofstream ofs("details.csv", std::ofstream::out);
	ofs << "Scheduling End," << (end_scheduling - start_execution_timestamp).count() << "\n";
	ofs << "Makespan," << makespan.count() << "\n";
	

	auto& repartition_times = scheduler->repartition_timestamps();
	ofs << "Scheduler Repartition Time,Repartition Begin,Repartition End,Copy Time,Reconstruction Time,Begin Graph Queue Size,Graph Updates,Begin Total Queues Size,End Total Queues Size,Queues Begin-End Sizes\n";
	
	auto copy_time_it = scheduler->graph_copy_duration().begin();
	auto reconstruction_it = scheduler->reconstruction_durations().begin();
	auto repartition_end_it = scheduler->repartition_end_timestamps().begin();
	auto q_begin_it = scheduler->q_size_repartition_begin().begin();
	auto q_end_it = scheduler->q_size_repartition_end().begin();
	auto repartition_notify_it = scheduler->repartition_notify_timestamp().begin();
	auto graph_q_size_it = scheduler->graph_queue_sizes().begin();
	auto graph_updates_it = scheduler->graph_updates().begin();
	for (auto& repartition_time : repartition_times) {
		double end_time = -1;
		double copy_time = -1;
		double reconstruction_time = -1;
		double repartition_notify_time = -1;
		size_t graph_q_size = -1;
		size_t graph_updates = -1;
		if(repartition_notify_it != scheduler->repartition_notify_timestamp().end()){
			repartition_notify_time = (*repartition_notify_it - start_execution_timestamp).count();
		}
		repartition_notify_it++;
		if(graph_q_size_it != scheduler->graph_queue_sizes().end()){
			graph_q_size = (*graph_q_size_it);
		}
		graph_q_size_it++;
		if(copy_time_it != scheduler->graph_copy_duration().end()){
			copy_time = (*copy_time_it).count();
		}
		copy_time_it++;
		if(graph_updates_it != scheduler->graph_updates().end()){
			graph_updates = (*graph_updates_it);
		}
		graph_updates_it++;
		if(reconstruction_it != scheduler->reconstruction_durations().end()){
			reconstruction_time = (*reconstruction_it).count();
		}
		reconstruction_it++;
		if(repartition_end_it != scheduler->repartition_end_timestamps().end()){
			end_time = (*repartition_end_it - start_execution_timestamp).count();
		}
		repartition_end_it++;
		ofs << repartition_notify_time/pow(10,9) << "," << (repartition_time - start_execution_timestamp).count()/pow(10,9) << "," << end_time/pow(10,9) << ","<< copy_time/pow(10,9) << ","<< reconstruction_time/pow(10,9) << "," << graph_q_size << "," << graph_updates <<",";

		if(q_begin_it != scheduler->q_size_repartition_begin().end() && q_end_it != scheduler->q_size_repartition_end().end()){
			size_t sum_begin = 0;
			size_t sum_end = 0;
			for (int i = 0; i < scheduler->n_partitions_; i++)
			{
				sum_begin += q_begin_it->at(i);
				sum_end += q_end_it->at(i);
			}
			ofs << sum_begin << "," << sum_end << ",";

			for (size_t i = 0; i < scheduler->n_partitions_; i++)
			{
				ofs << q_begin_it->at(i) << "-" << q_end_it->at(i) << ",";
			}
			q_begin_it++;
			q_end_it++;
		}
		ofs << std::endl;
		
	}

	ofs << std::endl;
	ofs.flush();
    ofs.close();
	//exit(EXIT_SUCCESS);
}

static void
usage(std::string prog)
{
	std::cout << "Usage: " << prog << " config\n";
}


int
main(int argc, char const *argv[])
{
	
	if (argc < 2) {
		usage(std::string(argv[0]));
		exit(1);
	}

	params = const_cast<char**>(argv);


	const auto config = toml::parse(argv[1]);

    const auto should_export_requests = toml::find<bool>(
        config, "export"
    );

    if (should_export_requests) {
        auto export_path = toml::find<std::string>(
			config, "output", "requests", "export_path"
		);
		workload::create_requests(argv[1]);
	

    }else{
		#if defined(MICHAEL) || defined(FELDMAN)
			cds::Initialize();
			cds::gc::HP gc;
			cds::threading::Manager::attachThread();
		#endif
		run(config);
	}

	
}
