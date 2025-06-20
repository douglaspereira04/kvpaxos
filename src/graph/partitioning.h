#ifndef MODEL_PARTITIONING_H
#define MODEL_PARTITIONING_H


#include <algorithm>
#include <float.h>
#include <fstream>
#include <kaHIP_interface.h>
#include <math.h>
#include <metis.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>
#include <utility>
#include <functional>

#include "graph.hpp"


namespace model {

enum CutMethod {METIS, KAHIP, FENNEL, REFENNEL, ROUND_ROBIN};
const std::unordered_map<std::string, CutMethod> string_to_cut_method({
    {"METIS", METIS},
    {"KAHIP", KAHIP},
    {"FENNEL", FENNEL},
    {"REFENNEL", REFENNEL},
    {"ROUND_ROBIN", ROUND_ROBIN}
});


void multilevel_cut(
    std::vector<int> &vertice_weight, 
    std::vector<int> &x_edges, 
    std::vector<int> &edges, 
    std::vector<int> &edges_weight,
    int n_partitions, 
    CutMethod cut_method,
    std::vector<int> &vertex_partitions
);

typedef std::pair<int, int> queue_entry_t;
typedef std::priority_queue<queue_entry_t, std::vector<queue_entry_t>, std::greater<>> priority_queue_t;

void greedy_partition(const std::vector<int>& weights, int n_partitions, std::vector<int> &vertice_to_partition);

}

#endif
