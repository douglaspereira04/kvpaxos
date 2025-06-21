#ifndef MODEL_GRAPH_H
#define MODEL_GRAPH_H


#include <algorithm>
#include <map>
#include <vector>
#include "absl/container/btree_map.h"
#include "ankerl/unordered_dense.h"
#include "utils.h"


namespace model {

template <typename T, typename Comparator = std::less<T>>
class Graph {

public:
    typedef absl::btree_map<T, int, Comparator> vertex_weight_t;
    typedef ankerl::unordered_dense::map<T, int> edges_weight_t;
    typedef ankerl::unordered_dense::map<T, edges_weight_t> edges_weights_t;

public:
    Graph() = default;
    
    Graph(Graph<T, Comparator> &g){
        vertex_weight_ = g.vertex_weight_;
        edges_weight_ = g.edges_weight_;
        n_edges_ = g.n_edges_;
    }


    inline int increment_vertice_weight(T &data, int weight) {
        auto [it, emplaced] = vertex_weight_.try_emplace(data, weight);
        if (!emplaced) {
            it->second += weight;
        }
        return it->second;
    }


    inline int decrement_vertice_weight(T &data, int weight) {
        int final_weight;
        auto it = vertex_weight_.find(data);
        if (it != vertex_weight_.end()) {
            it->second -= weight;
            final_weight = it->second;
            if constexpr(utils::ENABLE_INFO){
                if (final_weight < 0){
                    abort();
                }
            }
            if (it->second == 0){
                vertex_weight_.erase(it);
            }
        }

        if constexpr(utils::ENABLE_INFO){
            abort();
        }
        return final_weight;
    }


    inline void increment_edge_weight(T &from, T &to, int value) {
        auto [from_it, emplaced_from] = edges_weight_.try_emplace(from, edges_weight_t());
        if (emplaced_from){
            from_it->second.emplace(to, value);
            if constexpr(utils::ENABLE_INFO){
                n_edges_++;
            }
        } else {
            auto [to_it, emplaced_to] = from_it->second.try_emplace(to, value);
            if (!emplaced_to){
                to_it->second += value;
            }
        }
    }


    inline void decrement_edge_weight(T &from, T &to, int value) {
        auto from_it = edges_weight_.find(from);
        if (from_it != edges_weight_.end()) {
            auto to_it = from_it->second.find(to);
            if (to_it != from_it->second.end()){
                to_it->second -= value;
                int curr_weight = to_it->second;
                if (curr_weight == 0){
                    if (from_it->second.size() == 1){
                        edges_weight_.erase(from_it);
                    } else {
                        from_it->second.erase(to_it);
                    }
                    if constexpr(utils::ENABLE_INFO){
                        n_edges_--;
                    }
                }
                if constexpr(utils::ENABLE_INFO){
                    if(curr_weight < 0){
                        abort();
                    }
                }
            }
        }
    }

    size_t n_vertex() const {return vertex_weight_.size();}
    size_t n_edges() const {return n_edges_;}

    inline const vertex_weight_t& vertex_weight(){
        return vertex_weight_;
    }

    inline const edges_weights_t& edges_weight(){
        return edges_weight_;
    }

private:
    vertex_weight_t vertex_weight_;
    edges_weights_t edges_weight_;
    
    size_t n_edges_{0};
};

}


#endif
