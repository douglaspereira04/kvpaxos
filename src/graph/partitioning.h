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

std::vector<int> cut_graph (
    const Graph<int>& graph,
    std::unordered_map<int, kvpaxos::Partition<int>*>& partitions,
    CutMethod method,
    const std::unordered_map<int, kvpaxos::Partition<int>*>& old_data_to_partition,
        /* = std::unordered_map<int, kvpaxos::Partition<int>*>() */
    bool first_repartition /* = true */
);
std::vector<int> multilevel_cut
    (const Graph<int>& graph, int n_partitions, CutMethod cut_method);

}

#endif
