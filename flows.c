#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define u_int unsigned int
// #define WRONG_ARGUMENTS -1

// typedef struct Weights {
//     float total_bytes_w;
//     float flow_duration_w;
//     float avg_interarrival_time_w;
//     float avg_packet_len_w;
// } Weights;

// typedef struct Args {
//     char *input;
//     u_int n_clusters;
//     Weights weights;
// } Args;

// Args *init_args(int argc, char *argv[]) {
//     Args *args = malloc(sizeof(Args) + strlen(argv[2]));

//     strcpy(args->input, argv[1]);
//     if (argc == 7) {
//         args->n_clusters = atoi(argv[2]);
//         args->weights.total_bytes_w = atof(argv[3]);
//         args->weights.flow_duration_w = atof(argv[4]);
//         args->weights.avg_interarrival_time_w = atof(argv[5]);
//         args->weights.avg_packet_len_w = atof(argv[6]);
//     } else {
//         args->n_clusters = 0;
//     }

//     return args;
// }

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
    unsigned int capacity;
    unsigned int length;
    Cluster **items;
} ClusterArray;

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

double minimum_cluster_distance(Cluster *cluster_1, Cluster *cluster_2,
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
};

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

Cluster *merge_clusters(Cluster *cluster_1, Cluster *cluster_2) {
    Cluster *new_cluster =
        init_cluster(cluster_1->flow_count + cluster_2->flow_count);

    if (new_cluster != NULL && new_cluster->flows != NULL) {
        new_cluster->flow_count = cluster_1->flow_count + cluster_2->flow_count;

        for (unsigned int c1_idx = 0; c1_idx < cluster_1->flow_count;
             c1_idx++) {
            new_cluster->flows[c1_idx] = cluster_1->flows[c1_idx];
        }

        for (unsigned int c2_idx = 0; c2_idx < cluster_2->flow_count;
             c2_idx++) {
            new_cluster->flows[c2_idx + cluster_1->flow_count] =
                cluster_1->flows[c2_idx];
        }
    }

    return new_cluster;
}

void free_cluster_arr(ClusterArray *cluster_arr) {
    for (unsigned int idx = 0; idx < cluster_arr->length; idx++) {
        free_cluster(cluster_arr->items[idx]);
    }
    free(cluster_arr->items);
    free(cluster_arr);
}

void merge_closest_clusters(ClusterArray *cluster_arr) {

    for (unsigned int idx = 0; idx < cluster_arr->length; idx++) {
    }
}

int process_input(char *src) {
    FILE *input_file = fopen(src, "r");

    if (input_file == NULL) {
        return -4;
    }

    char line_buffer[256];

    if (fgets(line_buffer, 256, input_file) == NULL) {
        return -5;
    }

    ClusterArray *cluster_arr = malloc(sizeof(Cluster));
    cluster_arr->length = atoi(&(line_buffer[6]));
    cluster_arr->capacity = cluster_arr->length;

    cluster_arr->items = malloc(sizeof(Cluster *) * cluster_arr->length);

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
    unsigned int n_clusters = 0;
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

    printf("input: %s, clusters: %d, wb: %f, wt: %f, wd: %f, ws: %f\n", input,
           n_clusters, wb, wt, wd, ws);

    process_input(input);

    return 0;
}