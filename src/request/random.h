#ifndef RFUNC_RANDOM_H
#define RFUNC_RANDOM_H

#include <functional>
#include <random>
#include <unordered_map>
#include "acknowledged_counter.cpp"
#include "zipfian_int_distribution.cpp"

namespace rfunc {

typedef std::function<long()> RandFunction;
typedef std::function<double()> DoubleRandFunction;

enum Distribution {FIXED, UNIFORM, BINOMIAL, ZIPFIAN, LATEST};
const std::unordered_map<std::string, Distribution> string_to_distribution({
    {"FIXED", Distribution::FIXED},
    {"UNIFORM", Distribution::UNIFORM},
    {"BINOMIAL", Distribution::BINOMIAL},
    {"ZIPFIAN", Distribution::ZIPFIAN},
    {"LATEST", Distribution::LATEST}
});

RandFunction uniform_distribution_rand(int min_value, int max_value, long seed = std::mt19937::default_seed);
DoubleRandFunction uniform_double_distribution_rand(double min_value, double max_value, long seed = std::mt19937::default_seed);
RandFunction zipfian_distribution(long min, long max, long seed = std::mt19937::default_seed);
RandFunction scrambled_zipfian_distribution(long min, long max, long seed = std::mt19937::default_seed);
RandFunction skewed_latest_distribution(acknowledged_counter<long> *&counter, zipfian_int_distribution<long> *& zip, long seed = std::mt19937::default_seed);
RandFunction fixed_distribution(int value);
RandFunction binomial_distribution(
    int n_experiments, double success_probability, long seed = std::mt19937::default_seed
);
RandFunction ranged_binomial_distribution(
    int min_value, int n_experiments, double success_probability, long seed = std::mt19937::default_seed
);

}

#endif
