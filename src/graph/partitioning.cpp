#include "partitioning.h"


namespace model {

// This is a workaround to reuse the same method for both fennel
// and refennel when calculating the vertex's partition.
struct dummy_partition {
    int id_;
    int weight_ = 0;

    dummy_partition(int id): id_{id}{}

    int id() const {return id_;}
    int weight() const {return weight_;}
};


void multilevel_cut(
    std::vector<int> &vertice_weight, 
    std::vector<int> &x_edges, 
    std::vector<int> &edges, 
    std::vector<int> &edges_weight,
    int n_partitions, 
    CutMethod cut_method,
    std::vector<int> &vertex_partitions
)
{ 
    int n_constrains = 1;

    int options[METIS_NOPTIONS];
    METIS_SetDefaultOptions(options);
    options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
    options[METIS_OPTION_NUMBERING] = 0;
    options[METIS_OPTION_UFACTOR] = 200;

    int objval;
    int n_vertex = vertice_weight.size();
    vertex_partitions.resize(n_vertex);
    if (cut_method == METIS) {
        METIS_PartGraphKway(
            &n_vertex, &n_constrains, x_edges.data(), edges.data(),
            vertice_weight.data(), NULL, edges_weight.data(), &n_partitions, NULL,
            NULL, options, &objval, vertex_partitions.data()
        );
    } else {
        double imbalance = 0.03;//default kaffpa imbalance
        kaffpa(
            &n_vertex, vertice_weight.data(), x_edges.data(),
            edges_weight.data(), edges.data(), &n_partitions,
            &imbalance, true, -1, FASTSOCIAL, &objval,
            vertex_partitions.data()
        );
    }
}


void greedy_partition(const std::vector<int>& weights, int n_partitions, std::vector<int> &vertice_to_partition) {
    const size_t n_vertices = weights.size();
    vertice_to_partition.clear();
   

    priority_queue_t heap;
    for (int i = 0; i < n_partitions; ++i) {
        heap.emplace(0, i);
    }

    std::vector<std::pair<int, int>> vertices;
    for (size_t pos = 0; pos < n_vertices; pos++) {
        if (weights[pos] > 0) {
            vertices.emplace_back(pos, weights[pos]);
        }
    }
    vertice_to_partition.resize(vertices.size());

    std::sort(vertices.begin(), vertices.end(), [](auto& a, auto& b) {
        return a.second > b.second;
    });

    for (const auto& [pos, weight] : vertices) {
        auto [current_weight, partition_id] = heap.top();
        heap.pop();

        vertice_to_partition[pos] = partition_id;
        current_weight += weight;

        heap.emplace(current_weight, partition_id);
    }
}


}