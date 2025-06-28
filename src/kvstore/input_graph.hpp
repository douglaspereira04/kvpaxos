#ifndef _KVPAXOS_INPUT_GRAPH_H_
#define _KVPAXOS_INPUT_GRAPH_H_

#include "graph.hpp"
#include "edgeless_graph.hpp"
#include <vector>
#include "utils.h"
#include "ankerl/unordered_dense.h"

namespace kvpaxos {

template <typename T>
struct InputGraph{
    InputGraph(){}
    InputGraph(model::EdgelessGraph<T> *graph){
        __edgeless_graph = graph;
    }

    void update(){
        greedy_partitioning_data();
    }

    void greedy_partitioning_data(){
        vertice_weight = std::move(__edgeless_graph->vertex_weight());
        vertice_to_pos = std::move(__edgeless_graph->vertice_to_pos());
        __edgeless_graph->clear();
    }

    model::EdgelessGraph<T> *__edgeless_graph;
    std::vector<int> vertice_weight;
    ankerl::unordered_dense::map<T,int> vertice_to_pos;
};

}

#endif