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
#include "request/request_generation.h"
#include "types/types.h"
#include "graph/graph.hpp"


#if defined(FREE)
	#include "scheduler/free_scheduler.hpp"
	typedef kvpaxos::FreeScheduler<int, TRACK_LENGTH, Q_SIZE> Scheduler;
#elif defined(NON_STOP)
	#include "scheduler/non_stop_scheduler.hpp"
	typedef kvpaxos::NonStopScheduler<int, TRACK_LENGTH, Q_SIZE> Scheduler;
#else
	#include "scheduler/scheduler.hpp"
	typedef kvpaxos::Scheduler<int, TRACK_LENGTH, Q_SIZE, interval_type::MICROSECONDS> Scheduler;
#endif

typedef boost::lockfree::spsc_queue<client_message*, boost::lockfree::capacity<SCHEDULE_QUEUE_SIZE>> scheduling_queue_t;


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
int REQUEST_RATE = 7;
int RATE_RAISE_INTERVAL = 8;

char* *params;

void
metrics_loop(int sleep_duration, int n_requests, Scheduler* scheduler)
{
	auto already_counted_throughput = 0;
	auto counter = 0;
	std::cout << "Sec,Throughput,Graph Vertices,Graph Edges";
	int n_partitions =  atoi(params[N_PARTITIONS]);
	for (int i = 0; i < n_partitions; i++)
	{
		std::cout << ", In Queue " << i;
	}
	std::cout << ", In Queue Total" << std::endl;

	while (RUNNING) {
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
		auto executed_requests = scheduler->n_executed_requests();
		auto graph_vertices = scheduler->graph_vertices();
		auto graph_edges = scheduler->graph_edges();
		auto in_queue = scheduler->in_queue_amount();
		auto throughput = executed_requests - already_counted_throughput;
		std::cout << counter << ",";
		std::cout << throughput << ",";
		std::cout << graph_vertices << ",";
		std::cout << graph_edges << ",";
		size_t total = 0;
		for (int i = 0; i < n_partitions; i++)
		{
			total += in_queue[i];
			std::cout << in_queue[i] << ", ";
		}
		std::cout << total << std::endl;

		already_counted_throughput += throughput;
		counter++;

		if (executed_requests >= n_requests) {
			break;
		}
	}
}

static Scheduler*
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

	auto* scheduler = new Scheduler(
		n_requests, repartition_interval, n_partitions,
		repartition_method
	);
	auto n_initial_keys = atoi(params[N_INITIAL_KEYS]);

	for (auto i = 0; i <= n_initial_keys; i++) {
		struct client_message client_message;
		client_message.type = WRITE; 
		client_message.key = i;
		scheduler->process_populate_request(client_message);
	}

	scheduler->run();
	return scheduler;
}
static int client_message_id = 0;
struct client_message *
to_client_message(
	workload::Request& request)
{
	struct client_message *client_message = new struct client_message();
	client_message->sin_port = htons(0);
	client_message->id = client_message_id;
	client_message->type = request.type();
	client_message->key = request.key();
	for (auto i = 0; i < request.args().size(); i++) {
		client_message->args[i] = request.args()[i];
	}
	client_message->args[request.args().size()] = 0;
	client_message->size = request.args().size();
	client_message_id++;
	return client_message;
}

int request_rate = atoi(params[REQUEST_RATE]);
int rate_raise_interval = atoi(params[RATE_RAISE_INTERVAL]);
int64_t sleep_duration = static_cast<int64_t>(1.0E9/request_rate);

void
workload_loop(std::vector<client_message*> *messages, scheduling_queue_t *scheduling_queue)
{
	for(auto message = messages->begin(); message != messages->end(); message++){
		scheduling_queue->push(*message);
		std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_duration));
	}
	scheduling_queue->push(nullptr);
}

void
scheduling_loop(
	Scheduler *scheduler, scheduling_queue_t *scheduling_queue)
{
	while(true){
		while(scheduling_queue->read_available()==0){
			std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_duration));
		}
		client_message* message = std::move(scheduling_queue->front());
		scheduling_queue->pop();
		if (message == nullptr){
			break;
		}
		scheduler->schedule_and_answer(*message);
	}

}


static void
run(const toml_config& config)
{
	std::string requests_path = params[REQUESTS_PATH];
	
	
	std::ifstream requests_file(requests_path);
	std::vector<client_message*> *messages = new std::vector<client_message*>();
	while (requests_file.peek() != EOF) {
		workload::Request request= workload::import_cs_request(requests_file);
		client_message* message = to_client_message(request);
		messages->push_back(message);
	}
	requests_file.close();
	int n_requests = messages->size();

	auto* scheduler = initialize_scheduler(n_requests, config);
	auto throughput_thread = std::thread(
		metrics_loop, SLEEP, n_requests, scheduler
	);
	
	auto start_execution_timestamp = std::chrono::system_clock::now();
	scheduling_queue_t scheduling_queue;
	auto scheduling_thread = std::thread(scheduling_loop, scheduler, &scheduling_queue);
	auto workload_thread = std::thread(workload_loop, messages, &scheduling_queue);

	auto end_scheduling = std::chrono::system_clock::now();

	throughput_thread.join();
	scheduling_thread.join();
	workload_thread.join();
	auto end_execution_timestamp = std::chrono::system_clock::now();

	delete messages;

	auto makespan = end_execution_timestamp - start_execution_timestamp;


    std::ofstream ofs("details.csv", std::ofstream::out);
	ofs << "Scheduling End," << (end_scheduling - start_execution_timestamp).count()/pow(10,9) << "\n";
	ofs << "Makespan," << makespan.count()/pow(10,9) << "\n";
	ofs << "Error Count," << scheduler->error_count() << "\n";
	

	auto& repartition_times = scheduler->repartition_timestamps();
	ofs << "Repartition Request, Graph Copy Duration, Repartition Begin, Repartition End, Reconstruction Duration, Apply Time" << std::endl;
	
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

		ofs << std::endl;

	}

	ofs << std::endl;
	ofs.flush();
    ofs.close();
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
		run(config);
	}
	
}
