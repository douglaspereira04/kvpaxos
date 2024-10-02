library(ggplot2)


rate_csv <- function(csv_file, output_file){
  data <- read.csv(csv_file, header = TRUE, stringsAsFactors = FALSE)
  x_values <- data[, 1]
  labels <- colnames(data)[-1]

  plot_data <- data.frame(x = rep(x_values, times = length(labels)),
                          y = unlist(data[, -1]),
                          Legenda = rep(labels, each = length(x_values)))

  plot <- ggplot(plot_data, aes(x = x, y = y, color = Legenda)) +
    geom_line() +
    geom_point() +
    labs(x = "Arrival Rate", y = "Makespan") +
    theme_minimal() + 
    scale_x_continuous(labels = function(x) format(x, scientific = TRUE))

  ggsave(filename = output_file, plot = plot, limitsize = FALSE, width = 10, height = 5, units = "in", dpi = 300)
}

args <- commandArgs(trailingOnly = TRUE)
rate_csv(args[1], args[2])