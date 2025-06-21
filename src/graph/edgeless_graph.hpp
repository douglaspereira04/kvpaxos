#ifndef MODEL_EDGELESS_GRAPH_H
#define MODEL_EDGELESS_GRAPH_H


#include <algorithm>
#include <map>
#include <vector>
#include <queue>
#include "ankerl/unordered_dense.h"
#include "utils.h"


namespace model {

template <typename T>
class EdgelessGraph {

public:

    typedef ankerl::unordered_dense::map<T, int> vertex_to_pos_t;
    typedef std::vector<int> vertex_weight_t;
    typedef std::queue<int> available_pos_t;

public:
    EdgelessGraph(){};


    inline int increment_vertice_weight(T &data, int weight) {
        int final_weight;
        if (__available_pos.size() > 0){
            int pos = __available_pos.front();
            auto [it, emplaced] = __vertex_to_pos.try_emplace(data, pos);
            if (emplaced){
                __vertex_weight[pos] = weight;
                __available_pos.pop();
            } else {
                __vertex_weight[it->second] += weight;
            }
            final_weight = __vertex_weight[it->second];
        } else {
            int pos = __vertex_weight.size();
            auto [it, emplaced] = __vertex_to_pos.try_emplace(data, pos);
            if (emplaced){
                __vertex_weight.push_back(weight);
            } else {
                __vertex_weight[it->second] += weight;
            }
            final_weight = __vertex_weight[it->second];
        }
        return final_weight;
    }


    inline int decrement_vertice_weight(T &data, int weight) {
        int final_weight;
        auto it = __vertex_to_pos.find(data);
        if (it != __vertex_to_pos.end()) {
            int pos = it->second;
            __vertex_weight[pos] -= weight;
            final_weight = __vertex_weight[pos];
            if (__vertex_weight[pos] == 0) {
                __available_pos.push(pos);
                __vertex_to_pos.erase(it);
            }
        } else {
            final_weight = 0;
        }
        return final_weight;
    }

    size_t n_vertex() const {return __vertex_to_pos.size();}
    size_t n_edges() const {return n_edges_;}

    inline vertex_weight_t vertex_weight(){
        return __vertex_weight;
    }

    inline vertex_to_pos_t vertice_to_pos(){
        return __vertex_to_pos;
    }

private:
    vertex_to_pos_t __vertex_to_pos;
    vertex_weight_t __vertex_weight;
    available_pos_t __available_pos;
    
    size_t n_edges_{0};
};

}


#endif
