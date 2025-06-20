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
    vertice_to_partition.resize(n_vertices);
    
    priority_queue_t partition_queue;

    for (int i = 0; i < n_partitions; i++){
        partition_queue.emplace(0, i);
    }

    std::vector<std::pair<int, int>> sorted_vertices;
    sorted_vertices.reserve(n_vertices);

    for (size_t i = 0; i < n_vertices; i++){
        sorted_vertices.emplace_back(weights[i], i);
    }
    std::sort(sorted_vertices.rbegin(), sorted_vertices.rend());

    for (const auto& [weight, vertex] : sorted_vertices) {
        auto [current_weight, partition] = partition_queue.top();
        partition_queue.pop();

        vertice_to_partition[vertex] = partition;

        partition_queue.emplace(current_weight + weight, partition);
    }
}

}