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


std::vector<int> cut_graph (
    const Graph<int>& graph,
    std::unordered_map<int, kvpaxos::Partition<int>*>& partitions,
    CutMethod method,
    const std::unordered_map<int, kvpaxos::Partition<int>*>& old_data_to_partition =
        std::unordered_map<int, kvpaxos::Partition<int>*>(),
    bool firt_repartition = true
) {
    if (method == METIS) {
        return multilevel_cut(graph, partitions.size(), method);
    } else if (method == KAHIP) {
        return multilevel_cut(graph, partitions.size(), method);
    } else {
        return std::vector<int>();
    }
}


std::vector<int> multilevel_cut(
    const Graph<int>& graph, int n_partitions, CutMethod cut_method
)
{
    auto& vertex = graph.vertex();
    auto sorted_vertex = std::move(graph.sorted_vertex());
    int n_constrains = 1;

    auto vertice_weight = std::vector<int>();
    
    //Fix(1): guardar as posições de um vertice
    // em vertice_weight em um unordered_map
    std::unordered_map<int, int> vertex_positions;
    int i = 0;
    for (auto& vertice : sorted_vertex) {
        vertice_weight.push_back(vertex.at(vertice));
        vertex_positions[vertice] = i;
        i++;
    }

    auto x_edges = std::vector<int>();
    auto edges = std::vector<int>();
    auto edges_weight = std::vector<int>();

    x_edges.push_back(0);
    for (auto& vertice : sorted_vertex) {
        auto last_edge_index = x_edges.back();
        auto n_neighbours = 0;

        for (auto& vk: graph.vertice_edges(vertice)) {
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
            auto neighbour = vertex_positions[vk.first];
            auto weight = vk.second;
            edges.push_back(neighbour);
            edges_weight.push_back(weight);
            n_neighbours++;
        }
        x_edges.push_back(last_edge_index + n_neighbours);
    }

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
        double imbalance = 0.2;  // equal to METIS default imbalance
        kaffpa(
            &n_vertex, vertice_weight.data(), x_edges.data(),
            edges_weight.data(), edges.data(), &n_partitions,
            &imbalance, true, -1, FAST, &objval,
            vertex_partitions.data()
        );
    }

    return vertex_partitions;
}

}