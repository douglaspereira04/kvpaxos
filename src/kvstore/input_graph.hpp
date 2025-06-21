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
    InputGraph(model::Graph<T> *graph){
        __graph = graph;
        assert(utils::ENABLE_EDGES);
    }
    InputGraph(model::EdgelessGraph<T> *graph){
        __edgeless_graph = graph;
        assert(!utils::ENABLE_EDGES);
    }

    void update(){
        if constexpr(utils::ENABLE_EDGES){
            multilevel_cut_data();
        } else {
            greedy_partitioning_data();
        }
    }



    /*
        Stores the graph as required by KAHIP and METIS in
        vertice_weight, x_edges, edges and edges_weight.
        The returned structure is a map of vertice keys 
        to the corresponding position in vertice_weight vector/array
    */
    void multilevel_cut_data(){
        vertice_to_pos.clear();
        vertice_weight.clear();
        x_edges.clear();
        edges.clear();
        edges_weight.clear();
    
        auto g_vertex_weight = __graph->vertex_weight();
        auto g_edges_weight = __graph->edges_weight();

        int i = 0;
        vertice_weight.reserve(g_vertex_weight.size());
        vertice_to_pos.reserve(g_vertex_weight.size());
        for (auto& v_w : g_vertex_weight) {
            vertice_weight.push_back(v_w.second);
            vertice_to_pos.emplace(v_w.first, i);
            i++;
        }

        x_edges.reserve(g_vertex_weight.size());
        x_edges.push_back(0);
        for (auto& v_w : g_vertex_weight) {
            auto last_edge_index = x_edges.back();
            auto n_neighbours = 0;

            auto from_it = g_edges_weight.find(v_w.first);
            if (from_it != g_edges_weight.end()){
                for (auto& to_it: from_it->second) {
                    auto v_p_it = vertice_to_pos.find(to_it.first);
                    if(v_p_it != vertice_to_pos.end()){
                        auto neighbour = v_p_it->second;
                        auto weight = to_it.second;
                        edges.push_back(neighbour);
                        edges_weight.push_back(weight);
                        n_neighbours++;
                    }
                }
            }
            x_edges.push_back(last_edge_index + n_neighbours);
        }
    }

    void greedy_partitioning_data(){
        vertice_weight = std::move(__edgeless_graph->vertex_weight());
        vertice_to_pos = std::move(__edgeless_graph->vertice_to_pos());
        __edgeless_graph->clear();
    }

    model::Graph<T> *__graph;
    model::EdgelessGraph<T> *__edgeless_graph;
    std::vector<int> vertice_weight;
    std::vector<int> x_edges;
    std::vector<int> edges;
    std::vector<int> edges_weight;
    ankerl::unordered_dense::map<T,int> vertice_to_pos;
};

}

#endif