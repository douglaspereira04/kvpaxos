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


    inline void increment_vertice_weight(T &data, int weight) {
        auto [it, emplaced] = vertex_weight_.try_emplace(data, 1);
        if (!emplaced) {
            it->second++;
        }
    }


    inline void decrement_vertice_weight(T &data, int weight) {
        auto it = vertex_weight_.find(data);
        if (it != vertex_weight_.end()) {
            if (it->second > 0){
                it->second += weight;
            } else {
                vertex_weight_.erase(it);
            }
        }
    }


    inline void increment_edge_weight(T &from, T &to, int value) {
        auto [from_it, emplaced_from] = edges_weight_.try_emplace(from, edges_weight_t());
        if (emplaced_from){
            from_it->second.emplace(to, value);
            if constexpr(utils::ENABLE_INFO){
                n_edges_++;
            }
        } else {
            from_it->second.find(to)->second += value;
        }
    
        T reverse_from = to;
        T reverse_to = from;
        auto [reverse_from_it, emplaced_reverse_from] = edges_weight_.try_emplace(reverse_from, edges_weight_t());
        if (emplaced_reverse_from){
            reverse_from_it->second.emplace(reverse_to, value);
        } else {
            reverse_from_it->second.find(reverse_to)->second += value;
        }
    }


    inline void decrement_edge_weight(T &from, T &to, int value) {
        T reverse_from = to;
        T reverse_to = from;
        auto from_it = edges_weight_.find(from);
        if (from_it != edges_weight_.end()) {
            auto to_it = from_it->second.find(to);
            if (to_it != from_it->second.end()){
                to_it->second -= value;
                if (to_it->second <= 0){
                    if (from_it->second.size() == 1){
                        edges_weight_.erase(from_it);
                    } else {
                        from_it->second.erase(to_it);
                    }
                    auto reverse_from_it  = edges_weight_.find(reverse_from);
                    if (reverse_from_it->second.size() == 1){
                        edges_weight_.erase(reverse_from_it);
                    } else {
                        reverse_from_it->second.erase(reverse_to);
                    }
                    if constexpr(utils::ENABLE_INFO){
                        n_edges_--;
                    }
                } else {
                    auto reverse_from_it  = edges_weight_.find(reverse_from);
                    auto reverse_to_it = reverse_from_it->second.find(reverse_to);
                    reverse_to_it->second -= value;
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
