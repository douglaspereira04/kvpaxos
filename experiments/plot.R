# Load necessary libraries
library(ggplot2)
library(tidyr)
library(svglite)

# Path to your CSV file

plot_csv <- function(csv_file, output_file, tall_output_file){
  data <- read.csv(csv_file, header = TRUE, na.strings = "", check.names = FALSE)
  data$Time <- 1:nrow(data)
  data_long <- pivot_longer(data, cols = -Time, names_to = "Version", values_to = "Throughput")
  plot <- ggplot(data_long, aes(x = Time, y = Throughput, color = Version)) +
    geom_line() +
    labs(x = "Time(seconds)", y = "Throughput (10³ops/s)") +
    xlim(0, 155) +
    theme_minimal() +
    theme(
        axis.text=element_text(size=14), #change font size of axis text
        axis.title=element_text(size=14), #change font size of axis titles
        plot.title=element_text(size=16), #change font size of plot title
        legend.text=element_text(size=16), #change font size of legend text
        legend.title=element_text(size=16)) #change font size of legend title   
  ggsave(filename = output_file, plot = plot, limitsize = FALSE, width = 10, height = 2.5, units = "in", dpi = 300)
  #ggsave(filename = tall_output_file, plot = plot, limitsize = FALSE, width = 10, height = 5, units = "in", dpi = 300)
}

args <- commandArgs(trailingOnly = TRUE)
plot_csv(args[1], args[2], args[3])