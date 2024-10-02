#!/bin/bash
rm -r *.csv
rm -r *.svg
rm -r *.png
rm -r *.pdf
python3 compile_data.py
files=$(find . -name "*_throughputs.csv")
for file in $files; do
    output_file=$(basename $file csv)png
    tall_output_file=tall_$(basename $file csv)png
    Rscript plot.R $file $output_file $tall_output_file
done

files=$(find . -name "*_rates.csv")
for file in $files; do
    output_file=$(basename $file csv)png
    tall_output_file=tall_$(basename $file csv)png
    Rscript rates.R $file $output_file
done
