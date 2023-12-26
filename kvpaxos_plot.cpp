#include <matplot/matplot.h>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string> 
#include <string_view> 
#include <numeric>
#include <vector>
#include <regex>
#include <iostream>
#include <limits>




std::string escaped_underline(std::string str){
  return std::regex_replace(str, std::regex("_"), " ");
}

std::vector<std::string> sizes = {
    "100000",
    "500000",
    "1000000",
    "5000000",
    "10000000"
};

std::vector<std::pair<std::string, std::string>> non_stop_ycsb_d = {
    {"NS","output/ycsb_d/METIS/0_0_1000000_0_non_stop_8_1.csv"},
    {"NS, {/:Italic {/Symbol d}}{/:Italic r} = 100","output/ycsb_d/METIS/100_0_1000000_0_non_stop_8_1.csv"},
    {"NS, {/:Italic {/Symbol d}}{/:Italic r} = 1.000","output/ycsb_d/METIS/1000_0_1000000_0_non_stop_8_1.csv"},
    {"NS, {/:Italic {/Symbol d}}{/:Italic r} = 10.000","output/ycsb_d/METIS/10000_0_1000000_0_non_stop_8_1.csv"},
    {"NS, {/:Italic {/Symbol d}}{/:Italic r} = 100.000","output/ycsb_d/METIS/100000_0_1000000_0_non_stop_8_1.csv"},
};

std::vector<std::pair<std::string, std::string>> non_stop_ycsb_e = {
    {"NS","output/ycsb_e/METIS/0_0_1000000_0_non_stop_8_1.csv"},
    {"NS, {/:Italic {/Symbol d}}{/:Italic r} = 100","output/ycsb_e/METIS/100_0_1000000_0_non_stop_8_1.csv"},
    {"NS, {/:Italic {/Symbol d}}{/:Italic r} = 1.000","output/ycsb_e/METIS/1000_0_1000000_0_non_stop_8_1.csv"},
    {"NS, {/:Italic {/Symbol d}}{/:Italic r} = 10.000","output/ycsb_e/METIS/10000_0_1000000_0_non_stop_8_1.csv"},
    {"NS, {/:Italic {/Symbol d}}{/:Italic r} = 100.000","output/ycsb_e/METIS/100000_0_1000000_0_non_stop_8_1.csv"},
};

std::vector<std::pair<std::string, std::string>> old_ycsb_d = {
    {"Base {/:Italic {/Symbol d}}{/:Italic p} = 100.000","output/ycsb_d/METIS/0_0_1000000_100000_old_8_1.csv"},
    {"Base {/:Italic {/Symbol d}}{/:Italic p} = 1.000.000","output/ycsb_d/METIS/0_0_1000000_1000000_old_8_1.csv"},
    {"Base {/:Italic {/Symbol d}}{/:Italic p} = 10.000.000","output/ycsb_d/METIS/0_0_1000000_10000000_old_8_1.csv"},
};

std::vector<std::pair<std::string, std::string>> old_ycsb_e = {
    {"Base {/:Italic {/Symbol d}}{/:Italic p} = 10.000","output/ycsb_e/METIS/0_0_1000000_10000_old_8_1.csv"},
    {"Base {/:Italic {/Symbol d}}{/:Italic p} = 100.000","output/ycsb_e/METIS/0_0_1000000_100000_old_8_1.csv"},
    {"Base {/:Italic {/Symbol d}}{/:Italic p} = 1.000.000","output/ycsb_e/METIS/0_0_1000000_1000000_old_8_1.csv"},
};


void plot_files(std::vector<std::pair<std::string,std::string>> files, std::string image_name, std::string folder, bool fit_to_faster){
    std::vector<double> x;
    std::map<std::string,std::vector<double>> y;
    double min_x = std::numeric_limits<double>::max();

    for (auto &&file : files)
    {
        std::cout << file.first << std::endl;
        std::ifstream infile(file.second);
        std::string line;
        y[file.first] = std::vector<double>();

        std::vector<double> x_temp;

        getline( infile, line );
        while(getline( infile, line )){
            std::string temp;
            if(line.empty()){
                break;
            }
            std::istringstream  ss(line);

            getline(ss, temp, ',');
            int sec = std::stoi(temp);
            x_temp.push_back(sec);

            getline(ss, temp, ',');
            double throughput = std::stol(temp);
            y[file.first].push_back(throughput/1000.f);

        }

        if(x_temp.size() > x.size()){
            x = x_temp;
        }
        if(x_temp.back() < min_x){
            min_x = x_temp.back();
        }

    }
    std::vector<std::string> legend_vector;
    std::cout << "x.size():" << x.size() << std::endl;

    int i = 0;
    for (const auto& kv : y) {
        auto p = matplot::plot(x, kv.second);
        p->line_width(1);
        p->marker_size(8);
        p->display_name(kv.first);
        i = (i+1)%8;
        matplot::hold(matplot::on);
    }

    if(fit_to_faster){
        matplot::xrange({0.0,100.0});
    }else{
        matplot::xrange({0.0,x.back()});
    }
    matplot::grid(matplot::on);
    matplot::xlabel("Tempo (segundos)");
    matplot::ylabel("Vazão (kRequisições)");
    auto lgd = matplot::legend(true);
    lgd->location(matplot::legend::general_alignment::topright);
    lgd->inside(false);
    lgd->font_size(14.f);
    std::string path = folder+image_name+".png";
    matplot::title("");
    matplot::save(path);
    matplot::hold(false);
}

void plot_all(std::string folder, bool fit_to_faster = false){

    std::vector<std::pair<std::string,std::string>> files;

	/*
    //ycsb_d
    for (size_t i = 0; i < 5; i++)
    {
        files.clear();
        files.push_back(original_ycsb_d[i]);
        files.push_back(lc_ycsb_d[i]);
        files.push_back(lc_nb_ycsb_d[i]);
        plot_files(files, "old_free_ycsb_d_"+sizes[i], folder, fit_to_faster);
    }

    //ycsb_e
    for (size_t i = 0; i < 3; i++)
    {
        files.clear();
        files.push_back(original_ycsb_e[i]);
        files.push_back(lc_ycsb_e[i]);
        files.push_back(lc_nb_ycsb_e[i]);
        plot_files(files, "old_free_ycsb_e_"+sizes[i], folder, fit_to_faster);
    }*/

    plot_files(non_stop_ycsb_d, "non_stop_ycsb_d", folder, fit_to_faster);
    plot_files(non_stop_ycsb_e, "non_stop_ycsb_e", folder, fit_to_faster);

    plot_files(old_ycsb_d, "old_ycsb_d", folder, fit_to_faster);
    plot_files(old_ycsb_e, "old_ycsb_e", folder, fit_to_faster);


    /*
    //old_free_ycsb_d
    for (size_t i = 0; i < 5; i++)
    {
        files.clear();
        files.push_back(old_ycsb_d[i]);
        files.push_back(free_ycsb_d[i]);
        plot_files(files, "old_free_ycsb_d_"+sizes[i], folder, fit_to_faster);
    }

    //old_free_ycsb_e
    for (size_t i = 0; i < 3; i++)
    {
        files.clear();
        files.push_back(old_ycsb_e[i]);
        files.push_back(free_ycsb_e[i]);
        plot_files(files, "old_free_ycsb_e_"+sizes[i], folder, fit_to_faster);
    }

    //blocking_nonblocking_ycsb_d
    for (size_t i = 0; i < 5; i++)
    {
        files.clear();
        files.push_back(blocking_free_ycsb_d[i]);
        files.push_back(nonblocking_free_ycsb_d[i]);
        plot_files(files, "blocking_nonblocking_ycsb_d_"+sizes[i], folder, fit_to_faster);
    }

    //blocking_nonblocking_ycsb_e
    for (size_t i = 0; i < 3; i++)
    {
        files.clear();
        files.push_back(blocking_free_ycsb_e[i]);
        files.push_back(nonblocking_free_ycsb_e[i]);
        plot_files(files, "blocking_nonblocking_ycsb_e_"+sizes[i], folder, fit_to_faster);
    }*/

}
int main(int argc, char *argv[]){
    plot_all("results/");
    plot_all("results/zoom/", true);
    return 0;
}
