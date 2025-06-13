#ifndef _KVPAXOS_INPUT_GRAPH_H_
#define _KVPAXOS_INPUT_GRAPH_H_

#include "graph.hpp"
#include <vector>
#include "ankerl/unordered_dense.h"

namespace kvpaxos {

template <typename T>
struct InputGraph{
    InputGraph(){}
    InputGraph(model::Graph<T> &graph){
        vertice_to_pos = graph.multilevel_cut_data(vertice_weight, x_edges, edges, edges_weight);
    }

    std::vector<int> vertice_weight;
    std::vector<int> x_edges;
    std::vector<int> edges;
    std::vector<int> edges_weight;
    ankerl::unordered_dense::map<T,int> vertice_to_pos;
};

}

#endif