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


    /*
        The original sorted_vertex function iterates over the 
        vertex_weights_ map pushing to the back of a vector, taking O(N), 
        and then sorts the vector, average O(N log(N)) (cpluplus.com). Iterating over the
        resulting vector is O(N).
        The insertion in a map is O(log(map.size)) (cpluplus.com), resulting
        in O(N log(0->N)) to insert all itens. After that, iterating is O(N).
        No need to sort, the map is already sorted.
    */
    std::map<T, int> sorted_map() const {
        std::map<T, int> sorted_map;
        for (auto& it : vertex_weight_) {
            sorted_map.insert(it);
        }
        return sorted_map;
    }


    /*
        Stores the graph as required by KAHIP and METIS in
        vertice_weight, x_edges, edges and edges_weight.
        The returned structure is a map of vertice keys 
        to the corresponding position in vertice_weight vector/array
    */
    std::unordered_map<T, int> multilevel_cut_data(
        std::vector<int> &vertice_weight, 
        std::vector<int> &x_edges, 
        std::vector<int> &edges, 
        std::vector<int> &edges_weight){
            
        std::unordered_map<T, int> vertice_positions;
        std::vector<T> sorted_vertex = this->sorted_vertex();

        int i = 0;
        for (auto& v : sorted_vertex) {
            vertice_weight.push_back(vertex_weight_.at(v));
            vertice_positions[v] = i;
            i++;
        }

        x_edges.push_back(0);
        for (auto& v : sorted_vertex) {
            auto last_edge_index = x_edges.back();
            auto n_neighbours = 0;
            
            for (auto& e_it: edges_weight_.at(v)) {
                //auto neighbour = vk.first;
                /*
                    Os procedimentos do Metis e Kahip mantém controle dos 
                    elementos do grafo por meio somente das posições dos 
                    vetores de entrada. A entrada requer que o vizinho indique 
                    a posição correspondente do vertice vizinho no vetor vertice_weight. 
                    Se os valores de chave dos vertices do grafo forem contiguos de 0 à N, 
                    o vetor vertice_weight teria posições de 0 à N e os valores 
                    de chave dos vizinhos seriam equivalentes às posições do vertice 
                    em vertice_weight, não resultando em erros na construção da entrada. 
                    No entanto, caso o conjunto de chaves não contenha valores contiguos 
                    começando por 0, o vizinho pode apontar para uma posição fora do vetor vertice_weight.
                */
        
                //Fix(2): usar o valor das posições em vertice_weight
                if(vertice_positions.find(e_it.first) != vertice_positions.end()){
                    auto neighbour = vertice_positions[e_it.first];
                    auto weight = e_it.second;
                    edges.push_back(neighbour);
                    edges_weight.push_back(weight);
                    n_neighbours++;
                }
            }
            x_edges.push_back(last_edge_index + n_neighbours);
        }

        return vertice_positions;
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
