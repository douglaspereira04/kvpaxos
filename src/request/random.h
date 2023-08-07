#ifndef RFUNC_RANDOM_H
#define RFUNC_RANDOM_H

#include <functional>
#include <random>
#include <unordered_map>

namespace rfunc {

typedef std::function<int()> RandFunction;
typedef std::function<double()> DoubleRandFunction;

enum Distribution {FIXED, UNIFORM, BINOMIAL, ZIPFIAN};
const std::unordered_map<std::string, Distribution> string_to_distribution({
    {"FIXED", Distribution::FIXED},
    {"UNIFORM", Distribution::UNIFORM},
    {"BINOMIAL", Distribution::BINOMIAL},
    {"ZIPFIAN", Distribution::ZIPFIAN}
});

RandFunction uniform_distribution_rand(int min_value, int max_value);
DoubleRandFunction uniform_double_distribution_rand(double min_value, double max_value);
RandFunction zipfian_distribution(long min, long max);
RandFunction scrambled_zipfian_distribution(long min, long max);
RandFunction fixed_distribution(int value);
RandFunction binomial_distribution(
    int n_experiments, double success_probability
);
RandFunction ranged_binomial_distribution(
    int min_value, int n_experiments, double success_probability
);

}

#endif
