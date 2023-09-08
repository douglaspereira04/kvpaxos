#include "random.h"
#include "scrambled_zipfian_int_distribution.cpp"
#include "skewed_latest_int_distribution.cpp"
#include "acknowledged_counter.cpp"
#include "zipfian_int_distribution.cpp"


namespace rfunc {

RandFunction uniform_distribution_rand(int min_value, int max_value) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<long> distribution(min_value, max_value);

    return std::bind(distribution, generator);
}

DoubleRandFunction uniform_double_distribution_rand(double min_value, double max_value) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> distribution(min_value, max_value);

    return std::bind(distribution, generator);
}

RandFunction fixed_distribution(int value) {
    return [value]() {
        return value;
    };
}

RandFunction binomial_distribution(
    int n_experiments, double success_probability
) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::binomial_distribution<long> distribution(
        n_experiments, success_probability
    );

    return std::bind(distribution, generator);
}

RandFunction zipfian_distribution(long min, long max) {
    std::random_device rd;
    std::mt19937 generator(rd());
    zipfian_int_distribution<long> distribution(min, max);
    return std::bind(distribution, generator);
}

RandFunction scrambled_zipfian_distribution(long min, long max) {

    std::random_device rd;
    std::mt19937 generator(rd());
    scrambled_zipfian_int_distribution<long> distribution(min, max);
    return std::bind(distribution, generator);
}

RandFunction skewed_latest_distribution(acknowledged_counter<long> *&counter, zipfian_int_distribution<long> *& zip){
    std::random_device rd;
    std::mt19937 generator(rd());
    skewed_latest_int_distribution<long> distribution(counter, zip);
    return std::bind(distribution, generator);
}


RandFunction ranged_binomial_distribution(
    int min_value, int n_experiments, double success_probability
) {
    auto random_func = binomial_distribution(
        n_experiments - min_value, success_probability
    );

    return [min_value, random_func](){
        auto value = random_func() + min_value;
        return value;
    };
}

}
