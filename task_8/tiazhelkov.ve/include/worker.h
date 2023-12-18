struct Task {
    double x1;
    double x2;
    double y1;
    double y2;
    long long num_of_points;
};

double monte_carlo(struct Task task, int num_of_cores);
