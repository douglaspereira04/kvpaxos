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


std::vector<int> multilevel_cut(
    std::vector<int> &vertice_weight, 
    std::vector<int> &x_edges, 
    std::vector<int> &edges, 
    std::vector<int> &edges_weight,
    int n_partitions, 
    CutMethod cut_method
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
    auto vertex_partitions = std::vector<int>(n_vertex, 0);
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
            &imbalance, true, -1, ECO, &objval,
            vertex_partitions.data()
        );
    }

    return vertex_partitions;
}

}