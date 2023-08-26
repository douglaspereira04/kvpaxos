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

#include <cds/init.h>
#include <cds/gc/hp.h>

#include "request/request_generation.h"
#include "types/types.h"
#include "graph/graph.hpp"


#if defined(COPY)
	#include "scheduler/copy_scheduler.hpp"
	#define Scheduler CopyScheduler
#else
	#include "scheduler/scheduler.hpp"
#endif


using toml_config = toml::basic_value<
	toml::discard_comments, std::unordered_map, std::vector
>;

static int verbose = 0;
static int SLEEP = 1000;
static bool RUNNING = true;


void
metrics_loop(int sleep_duration, int n_requests, kvpaxos::Scheduler<int>* scheduler)
{
	auto already_counted_throughput = 0;
	auto counter = 0;
	std::cout << "Sec,Throughput,GraphVertices,GraphEdges\n";
	while (RUNNING) {
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
		auto executed_requests = scheduler->n_executed_requests();
		//auto graph_queue_size = scheduler->graph_queue_size();
		auto graph_vertices = scheduler->graph_vertices();
		auto graph_edges = scheduler->graph_edges();
		auto throughput = executed_requests - already_counted_throughput;
		std::cout << counter << ",";
		std::cout << throughput << ",";
		std::cout << graph_vertices << ",";
		std::cout << graph_edges << "\n";
		already_counted_throughput += throughput;
		counter++;

		if (executed_requests == n_requests) {
			break;
		}
	}
}

static kvpaxos::Scheduler<int>*
initialize_scheduler(
	int n_requests,
	const toml_config& config)
{
	auto n_partitions = toml::find<int>(
		config, "n_partitions"
	);
	auto repartition_method_s = toml::find<std::string>(
		config, "repartition_method"
	);
	auto repartition_method = model::string_to_cut_method.at(
		repartition_method_s
	);
	auto repartition_interval = toml::find<int>(
		config, "repartition_interval"
	);
	auto* scheduler = new kvpaxos::Scheduler<int>(
		n_requests, repartition_interval, n_partitions,
		repartition_method
	);

	auto n_initial_keys = toml::find<int>(
		config, "n_initial_keys"
	);
	std::vector<workload::Request> populate_requests;
	for (auto i = 0; i <= n_initial_keys; i++) {
		populate_requests.emplace_back(WRITE, i, "");
	}

	scheduler->process_populate_requests(populate_requests);

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
	std::ifstream &requests_file,
	int print_percentage)
{	
	while (requests_file.peek() != EOF) {
		workload::Request request= workload::import_cs_request(requests_file);
		struct client_message message = to_client_message(request);
		scheduler.schedule_and_answer(message);
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

size_t count_requests(std::string &requests_path){
    std::ifstream requests_file (requests_path);   
	size_t count = (size_t)std::count_if(std::istreambuf_iterator<char>{requests_file}, {}, [](char c) { return c == '\n'; });
	requests_file.close();
	return count;
}

static void
run(const toml_config& config)
{
	auto requests_path = toml::find<std::string>(
		config, "requests_path"
	);

	auto n_requests = toml::find<int>(
		config, "n_requests"
	);
	
	std::ifstream requests_file(requests_path);
	auto* scheduler = initialize_scheduler(n_requests, config);
	auto throughput_thread = std::thread(
		metrics_loop, SLEEP, n_requests, scheduler
	);

	auto print_percentage = toml::find<int>(
		config, "print_percentage"
	);

	auto start_execution_timestamp = std::chrono::system_clock::now();
	execute_requests(*scheduler, requests_file, print_percentage);
	requests_file.close();

	throughput_thread.join();
	auto end_execution_timestamp = std::chrono::system_clock::now();

	auto makespan = end_execution_timestamp - start_execution_timestamp;
	std::cout << "Makespan," << makespan.count() << "\n";

	auto& repartition_times = scheduler->repartition_timestamps();
	std::cout << "Repartition,Duration,CopyTime\n";
	auto copy_time_it = scheduler->graph_copy_duration().begin();
	auto repartition_end_it = scheduler->repartition_end_timestamps().begin();
	for (auto& repartition_time : repartition_times) {
		double end_time = -1;
		double copy_time = -1;
		if(copy_time_it != scheduler->graph_copy_duration().end()){
			copy_time = (*copy_time_it).count();
		}
		if(repartition_end_it != scheduler->repartition_end_timestamps().end()){
			end_time = (*repartition_end_it - start_execution_timestamp).count();
		}
		std::cout << (repartition_time - start_execution_timestamp).count()/pow(10,9) << "," << end_time/pow(10,9) << ","<< copy_time/pow(10,9) << "\n";
		copy_time_it++;
		repartition_end_it++;
	}

	std::cout << std::endl;
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

	const auto config = toml::parse(argv[1]);

    const auto should_export_requests = toml::find<bool>(
        config, "export"
    );

    if (should_export_requests) {
        auto export_path = toml::find<std::string>(
			config, "output", "requests", "export_path"
		);
		workload::create_requests(argv[1]);
		
		//workload::export_requests(requests, export_path);

    }else{
		#if defined(MICHAEL) || defined(FELDMAN)
			cds::Initialize();
			cds::gc::HP gc;
			cds::threading::Manager::attachThread();
		#endif
		run(config);
	}

	
}
