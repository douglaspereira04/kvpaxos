#ifndef MODEL_GRAPH_H
#define MODEL_GRAPH_H


#include <algorithm>
#include <unordered_map>
#include <map>
#include <vector>


namespace model {

template <typename T>
class Graph {
public:
    Graph() = default;
    
    Graph(Graph<T> &g){
        vertex_weight_ = g.vertex_weight_;
        edges_weight_ = g.edges_weight_;
        n_edges_ = g.n_edges_;
        total_vertex_weight_ = g.total_vertex_weight_;
        total_edges_weight_ = g.total_edges_weight_;
    }

    void add_vertice(T data, int weight = 0) {
        vertex_weight_[data] = weight;
        edges_weight_[data] = std::unordered_map<T, int>();
        total_vertex_weight_ += weight;
    }

    void add_edge(T from, T to, int weight = 0) {
        if (edges_weight_[from].find(to) == edges_weight_[from].end()) {
            edges_weight_[from][to] = 0;
            edges_weight_[to][from] = 0;
            n_edges_++;
        }

        edges_weight_[from][to] = weight;
        edges_weight_[to][from] = weight;
        total_edges_weight_ += weight;
    }

    void increase_vertice_weight(T vertice, int value = 1) {
        vertex_weight_[vertice] += value;
        total_vertex_weight_ += value;
    }

    void increase_edge_weight(T from, T to, int value = 1) {
        edges_weight_[from][to] += value;
        edges_weight_[to][from] += value;
        total_edges_weight_ += value;
    }

    bool vertice_exists(T vertice) const {
        return vertex_weight_.find(vertice) != vertex_weight_.end();
    }

    bool are_connected(T vertice_a, T vertice_b) const {
        return edges_weight_.at(vertice_a).find(vertice_b) != edges_weight_.at(vertice_a).end();
    }

    std::vector<T> sorted_vertex() const {
        std::vector<T> sorted_vertex_;
        for (auto& it : vertex_weight_) {
            sorted_vertex_.emplace_back(it.first);
        }
        std::sort(sorted_vertex_.begin(), sorted_vertex_.end());
        return sorted_vertex_;
    }


    std::vector<T> sorted_vertex(std::unordered_map<T, int>& vertex_weight) const {
        std::vector<T> sorted_vertex_;
        for (auto& it : vertex_weight_) {
            sorted_vertex_.emplace_back(it.first);
            vertex_weight[it.first] = it.second;
        }
        std::sort(sorted_vertex_.begin(), sorted_vertex_.end());
        return sorted_vertex_;
    }

    std::size_t n_vertex() const {return vertex_weight_.size();}
    std::size_t n_edges() const {return n_edges_;}
    int total_vertex_weight() const {return total_vertex_weight_;}
    int total_edges_weight() const {return total_edges_weight_;}
    int vertice_weight(T vertice) const {return vertex_weight_.at(vertice);}
    int edge_weight(T from, T to) const {return edges_weight_.at(from).at(to);}
    const std::unordered_map<T, int>& vertice_edges(T vertice) const {
        return edges_weight_.at(vertice);
    }
    const std::unordered_map<T, int>& vertex() const {return vertex_weight_;}

    void vertice_weight(T data, int weight) {vertex_weight_[data] = weight;}

    const std::unordered_map<T, int> edge_weight(T from) const {return edges_weight_[from];}
    void edge_weight(T from, T to, int weight) {vertex_weight_[from][to] = weight;}

private:
    std::unordered_map<T, int> vertex_weight_;
    std::unordered_map<T, std::unordered_map<T, int>>
        edges_weight_;
    int n_edges_{0};
    int total_vertex_weight_{0};
    int total_edges_weight_{0};
};

}


#endif
