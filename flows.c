#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Flow {
    unsigned int flow_id;
    unsigned int total_bytes;
    unsigned int flow_duration;
    unsigned int packet_count;
    double avg_interarrival_time;
    double avg_packet_len;
} Flow;

typedef struct Cluster {
    unsigned int flow_count;
    Flow *flows;
} Cluster;

typedef struct ClusterArray {
    unsigned int length;
    Cluster **items;
} ClusterArray;

unsigned int get_min_flow_id(Cluster *cluster) {
    unsigned int min_flow_id = cluster->flows[0].flow_id;

    for (unsigned int idx = 1; idx < cluster->flow_count; idx++) {
        unsigned int curr_flow_id = cluster->flows[idx].flow_id;
        if (curr_flow_id < min_flow_id) {
            min_flow_id = curr_flow_id;
        }
    }

    return min_flow_id;
}

int qsort_flow_cmp(const void *a, const void *b) {
    Flow *flow_1 = (Flow *)a;
    Flow *flow_2 = (Flow *)b;

    return (flow_1->flow_id - flow_2->flow_id);
}

void sort_flows_in_cluster(Cluster *cluster) {
    qsort(cluster->flows, cluster->flow_count, sizeof(Flow), qsort_flow_cmp);
}

int qsort_cluster_cmp(const void *a, const void *b) {
    Cluster **cluster_1 = (Cluster **)a;
    Cluster **cluster_2 = (Cluster **)b;
    return get_min_flow_id(*cluster_1) - get_min_flow_id(*cluster_2);
}

void sort_cluster_arr(ClusterArray *cluster_arr) {
    for (unsigned int idx = 0; idx < cluster_arr->length; idx++) {
        sort_flows_in_cluster(cluster_arr->items[idx]);
    }

    qsort(cluster_arr->items, cluster_arr->length, sizeof(Cluster *),
          qsort_cluster_cmp);
}

void print_results(ClusterArray *cluster_arr) {
    sort_cluster_arr(cluster_arr);
    printf("Clusters: \n");
    for (unsigned int idx = 0; idx < cluster_arr->length; idx++) {
        Cluster *current_cluster = cluster_arr->items[idx];
        printf("cluster %d: ", idx);
        for (unsigned int idx2 = 0; idx2 < current_cluster->flow_count;
             idx2++) {
            printf("%d", current_cluster->flows[idx2].flow_id);
            (idx2 == current_cluster->flow_count - 1) ? printf("\n")
                                                      : printf(" ");
        }
    }
}

double wtd_euclidian_dist(Flow flow_1, Flow flow_2, double wb, double wt,
                          double wd, double ws) {
    int total_bytes_diff = abs((int)(flow_1.total_bytes - flow_2.total_bytes));
    int flow_duration_diff =
        abs((int)(flow_1.flow_duration - flow_2.flow_duration));
    double avg_interarrival_time_diff =
        fabs(flow_1.avg_interarrival_time - flow_2.avg_interarrival_time);
    double avg_packet_len_diff =
        fabs(flow_1.avg_packet_len - flow_2.avg_packet_len);

    return sqrt(wb * total_bytes_diff + wt * flow_duration_diff +
                wd * avg_interarrival_time_diff + ws * avg_packet_len_diff);
}

double calc_min_cluster_distance(Cluster *cluster_1, Cluster *cluster_2,
                                 double wb, double wt, double wd, double ws) {
    double min_dist = DBL_MAX;
    for (unsigned int idx = 0; idx < cluster_1->flow_count; idx++) {
        for (unsigned int idx2 = 0; idx2 < cluster_2->flow_count; idx2++) {
            double dist = wtd_euclidian_dist(
                cluster_1->flows[idx], cluster_2->flows[idx2], wb, wt, wd, ws);
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
    }
    return min_dist;
}

Cluster *init_cluster(unsigned int flow_count) {
    Cluster *new_cluster = malloc(sizeof(Cluster));
    if (new_cluster != NULL) {
        new_cluster->flow_count = flow_count;
        new_cluster->flows = malloc(sizeof(Flow) * new_cluster->flow_count);

        if (new_cluster->flows != NULL) {
            return new_cluster;
        }
    }
    return NULL;
}

void free_cluster(Cluster *cluster) {
    free(cluster->flows);
    free(cluster);
}

void merge_clusters(ClusterArray *cluster_arr, Cluster *cluster_1,
                    unsigned int cluster_2_idx) {
    Cluster *cluster_2 = cluster_arr->items[cluster_2_idx];
    cluster_1->flows = realloc(
        cluster_1->flows,
        (sizeof(Flow) * (cluster_1->flow_count + cluster_2->flow_count)));

    for (unsigned int idx = 0; idx < cluster_2->flow_count; idx++) {
        cluster_1->flows[cluster_1->flow_count + idx] = cluster_2->flows[idx];
    }

    cluster_1->flow_count += cluster_2->flow_count;

    free_cluster(cluster_2);
    for (unsigned int idx = cluster_2_idx; idx < cluster_arr->length - 1;
         idx++) {
        cluster_arr->items[idx] = cluster_arr->items[idx + 1];
    }

    cluster_arr->items[cluster_arr->length - 1] = NULL;
    cluster_arr->length--;
}

void free_cluster_arr(ClusterArray *cluster_arr) {
    for (unsigned int idx = 0; idx < cluster_arr->length; idx++) {
        free_cluster(cluster_arr->items[idx]);
    }
    free(cluster_arr->items);
    free(cluster_arr);
}

void merge_closest_clusters(ClusterArray *cluster_arr, double wb, double wt,
                            double wd, double ws) {
    double minimum_cluster_distance = DBL_MAX;
    Cluster *closest_cluster_1 = NULL;
    unsigned int closest_cluster_2_idx;

    for (unsigned int c1_idx = 0; c1_idx < cluster_arr->length; c1_idx++) {
        Cluster *cluster_1 = cluster_arr->items[c1_idx];
        for (unsigned int c2_idx = c1_idx + 1; c2_idx < cluster_arr->length;
             c2_idx++) {
            Cluster *cluster_2 = cluster_arr->items[c2_idx];
            double c_dist =
                calc_min_cluster_distance(cluster_1, cluster_2, wb, wt, wd, ws);
            if (c_dist < minimum_cluster_distance) {
                minimum_cluster_distance = c_dist;
                closest_cluster_1 = cluster_1;
                closest_cluster_2_idx = c2_idx;
            }
        }
    }

    if (minimum_cluster_distance < DBL_MAX && closest_cluster_1) {
        merge_clusters(cluster_arr, closest_cluster_1, closest_cluster_2_idx);
    } else {
        printf("error? nothing to merge");
    }
}

ClusterArray *cluster_arr_init(int length) {
    ClusterArray *cluster_arr = malloc(sizeof(ClusterArray));
    cluster_arr->length = length;
    cluster_arr->items = malloc(sizeof(Cluster *) * cluster_arr->length);

    return cluster_arr;
}

int process_input(char *src, unsigned int n_clusters, double wb, double wt,
                  double wd, double ws) {
    FILE *input_file = fopen(src, "r");

    if (input_file == NULL) {
        return -4;
    }

    char line_buffer[256];

    if (fgets(line_buffer, 256, input_file) == NULL) {
        return -5;
    }

    int flow_n = atoi(&(line_buffer[6]));

    ClusterArray *cluster_arr = cluster_arr_init(flow_n);

    for (unsigned int idx = 0; idx < cluster_arr->length; idx++) {
        Cluster *single_flow_cluster = init_cluster(1);
        Flow current_flow;
        if (fgets(line_buffer, 256, input_file) != NULL) {
            char *token = strtok(line_buffer, " "); // flow_id
            current_flow.flow_id = atoi(token);

            token = strtok(NULL, " "); // src_ip
            token = strtok(NULL, " "); // dst_ip
            token = strtok(NULL, " "); // total_bytes
            current_flow.total_bytes = atoi(token);

            token = strtok(NULL, " "); // flow_duration
            current_flow.flow_duration = atoi(token);

            token = strtok(NULL, " "); // packet_count
            current_flow.packet_count = atoi(token);

            token = strtok(NULL, " "); // avg_interarrival_time
            current_flow.avg_interarrival_time = atof(token);

            current_flow.avg_packet_len =
                current_flow.total_bytes / (double)current_flow.packet_count;
        } else {
            fprintf(stderr, "Error: count in input file is wrong!\n");
            return -6;
        }

        single_flow_cluster->flows[0] = current_flow;
        cluster_arr->items[idx] = single_flow_cluster;
    }

    // for (size_t idx = 0; idx < flow_count; idx++) {
    //     Flow flow_on_idx = flow_array[idx];

    //     printf("%d %d %d %d %f, %f\n", flow_on_idx.flow_id,
    //            flow_on_idx.total_bytes, flow_on_idx.flow_duration,
    //            flow_on_idx.packet_count, flow_on_idx.avg_interarrival_time,
    //            flow_on_idx.avg_packet_len);
    // }

    while (cluster_arr->length > n_clusters && n_clusters != 0) {
        merge_closest_clusters(cluster_arr, wb, wt, wd, ws);
    }

    print_results(cluster_arr);

    free_cluster_arr(cluster_arr);
    fclose(input_file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 7) {
        fprintf(stderr,
                "Wrong arguments! Usage: ./flows INPUT [N WB WT WD WS]\n");
        return -1;
    }

    char *input = argv[1];
    int n_clusters = 0;
    double wb = 0, wt = 0, wd = 0, ws = 0;

    if (argc == 7) {
        n_clusters = atoi(argv[2]);
        wb = atof(argv[3]);
        wt = atof(argv[4]);
        wd = atof(argv[5]);
        ws = atof(argv[6]);

        if (n_clusters <= 0) {
            fprintf(stderr, "error: n of clusters must be > 0\n");
            return -2;
        }

        if (wb < 0 || wt < 0 || wd < 0 || ws < 0) {
            fprintf(stderr, "error: weights must not be negative\n");
            return -3;
        }
    }

    return process_input(input, n_clusters, wb, wt, wd, ws);
}