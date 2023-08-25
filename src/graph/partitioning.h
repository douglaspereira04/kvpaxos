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
#include <utility>
#include <vector>

#include "graph.hpp"
#include "scheduler/partition.hpp"


namespace model {

enum CutMethod {METIS, KAHIP, FENNEL, REFENNEL, ROUND_ROBIN};
const std::unordered_map<std::string, CutMethod> string_to_cut_method({
    {"METIS", METIS},
    {"KAHIP", KAHIP},
    {"FENNEL", FENNEL},
    {"REFENNEL", REFENNEL},
    {"ROUND_ROBIN", ROUND_ROBIN}
});


std::vector<int> multilevel_cut(
    std::vector<int> &vertice_weight, 
    std::vector<int> &x_edges, 
    std::vector<int> &edges, 
    std::vector<int> &edges_weight,
    int n_partitions, 
    CutMethod cut_method
);

}

#endif
