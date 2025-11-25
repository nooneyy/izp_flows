#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MANDATORY_ARGC_LEN 2
#define OPTIONAL_ARGC_LEN 7

#define WRONG_ARGUMENTS_ERR -1
#define CLUSTER_FILL_ERR -1
#define RESIZE_CLUSTER_ERR -1

#define WRONG_N_CLUSTERS_ERR -2
#define NEGATIVE_WEIGHTS_ERR -3
#define CLUSTER_ARRAY_ERR -4

#define LINE_BUFFER_LEN 256

typedef struct Weights {
    float byte_wt;
    float flow_duration_wt;
    float interarrival_wt;
    float avg_packet_wt;
} Weights;

typedef struct Args {
    char *input_path;
    uint32_t n_clusters;
    Weights weights;
} Args;

typedef struct Flow {
    uint32_t id;
    int total_bytes;
    int flow_duration;
    float avg_interarrival_time;
    float avg_packet_len;
} Flow;

typedef struct Cluster {
    uint32_t length;
    Flow *flows;
} Cluster;

typedef struct ClusterArray {
    uint32_t length;
    Cluster **items;
} ClusterArray;

Cluster *init_cluster() {
    Cluster *new_cluster = malloc(sizeof(Cluster));
    if (new_cluster == NULL) {
        fprintf(stderr, "error: failed to allocate memory for cluster\n");
        return NULL;
    }
    new_cluster->length = 1;
    new_cluster->flows = malloc(sizeof(Flow));

    if (new_cluster->flows == NULL) {
        fprintf(stderr,
                "error: failed to allocate memory for initial cluster flow\n");
        free(new_cluster);
        return NULL;
    }

    return new_cluster;
}

void free_cluster(Cluster *cluster) {
    free(cluster->flows);
    free(cluster);
}

ClusterArray *init_cluster_arr(uint32_t length) {
    ClusterArray *cluster_arr = malloc(sizeof(ClusterArray));
    cluster_arr->length = length;
    cluster_arr->items = malloc(sizeof(Cluster *) * length);

    if (cluster_arr == NULL || cluster_arr->items == NULL) {
        fprintf(stderr, "error: failed to allocate memory for cluster array\n");
        return NULL;
    }

    return cluster_arr;
}

void free_cluster_arr(ClusterArray *cluster_arr) {
    for (uint32_t idx = 0; idx < cluster_arr->length; idx++) {
        free_cluster(cluster_arr->items[idx]);
    }
    free(cluster_arr->items);
    free(cluster_arr);
}

int merge_clusters(ClusterArray *cluster_arr, Cluster *cluster_1,
                   uint32_t cluster_2_idx) {
    Cluster *cluster_2 = cluster_arr->items[cluster_2_idx];
    Flow *temp_ptr =
        realloc(cluster_1->flows,
                (sizeof(Flow) * (cluster_1->length + cluster_2->length)));

    if (temp_ptr == NULL) {
        fprintf(stderr,
                "error: failed to resize cluster! ran out of memory?\n");
        return RESIZE_CLUSTER_ERR;
    }
    cluster_1->flows = temp_ptr;

    memcpy(&cluster_1->flows[cluster_1->length], cluster_2->flows,
           sizeof(Flow) * cluster_2->length);
    cluster_1->length += cluster_2->length;
    free_cluster(cluster_2);

    memmove(&cluster_arr->items[cluster_2_idx],
            &cluster_arr->items[cluster_2_idx + 1],
            sizeof(Cluster *) * (cluster_arr->length - cluster_2_idx - 1));

    cluster_arr->items[cluster_arr->length - 1] = NULL;
    cluster_arr->length--;

    return 0;
}

int fill_cluster_arr(ClusterArray *cluster_arr, FILE *input_file,
                     char *line_buffer) {
    for (uint32_t c_idx = 0; c_idx < cluster_arr->length; c_idx++) {
        Cluster *new_c = init_cluster();
        if (new_c == NULL) {
            return CLUSTER_FILL_ERR;
        }
        Flow new_flow = {0};

        if (fgets(line_buffer, LINE_BUFFER_LEN, input_file) != NULL) {
            char temp_src_ip[20], temp_dst_ip[20];
            float temp_packet_count;

            int amount_read = sscanf(
                line_buffer, "%u %s %s %d %d %f %f", &new_flow.id, temp_src_ip,
                temp_dst_ip, &new_flow.total_bytes, &new_flow.flow_duration,
                &temp_packet_count, &new_flow.avg_interarrival_time);

            if (amount_read != 7 || new_flow.flow_duration < 0 ||
                temp_packet_count < 0) {
                fprintf(stderr, "error: invalid input!\n");
                free_cluster(new_c);
                return CLUSTER_FILL_ERR;
            }

            new_flow.avg_packet_len = new_flow.total_bytes / temp_packet_count;
        } else {
            fprintf(stderr,
                    "error: flow count doesn't match amount of flows!\n");
            free_cluster(new_c);
            return CLUSTER_FILL_ERR;
        }

        new_c->flows[0] = new_flow;
        cluster_arr->items[c_idx] = new_c;
    }

    return 0;
}

ClusterArray *cluster_arr_from_input(char *input_path, int n_clusters) {
    FILE *input_file = fopen(input_path, "r");

    if (input_file == NULL) {
        fprintf(stderr, "error: could not read input file\n");
        return NULL;
    }

    char line_buffer[LINE_BUFFER_LEN];

    if (fgets(line_buffer, LINE_BUFFER_LEN, input_file) == NULL) {
        fprintf(stderr, "error: input file is empty\n");
        return NULL;
    }

    int flow_count;
    sscanf(line_buffer, "count=%d", &flow_count);

    if (flow_count <= 0 || flow_count < n_clusters) {
        fprintf(stderr, "error: flow count in input is <= 0 or smaller than "
                        "the n of requested clusters\n");
        return NULL;
    }

    ClusterArray *cluster_arr = init_cluster_arr(flow_count);

    if (fill_cluster_arr(cluster_arr, input_file, line_buffer) != 0) {
        free_cluster_arr(cluster_arr);
        return NULL;
    }

    fclose(input_file);
    return cluster_arr;
}

float wtd_euclidian_dist(Flow *flow_1, Flow *flow_2, Weights weights) {
    int total_bytes_diff = abs(flow_1->total_bytes - flow_2->total_bytes);
    int flow_duration_diff = abs(flow_1->flow_duration - flow_2->flow_duration);
    float avg_interarrival_time_diff =
        fabsf(flow_1->avg_interarrival_time - flow_2->avg_interarrival_time);
    float avg_packet_len_diff =
        fabsf(flow_1->avg_packet_len - flow_2->avg_packet_len);

    return sqrtf(
        weights.byte_wt * total_bytes_diff * total_bytes_diff +
        weights.flow_duration_wt * flow_duration_diff * flow_duration_diff +
        weights.interarrival_wt * avg_interarrival_time_diff *
            avg_interarrival_time_diff +
        weights.avg_packet_wt * avg_packet_len_diff * avg_packet_len_diff);
}

float min_cluster_distance(Cluster *cluster_1, Cluster *cluster_2,
                           Weights weights) {
    float min_dist = FLT_MAX;
    for (uint32_t c1_idx = 0; c1_idx < cluster_1->length; c1_idx++) {
        for (uint32_t c2_idx = 0; c2_idx < cluster_2->length; c2_idx++) {
            float dist = wtd_euclidian_dist(&cluster_1->flows[c1_idx],
                                            &cluster_2->flows[c2_idx], weights);
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
    }
    return min_dist;
}

int merge_closest_clusters(ClusterArray *cluster_arr, Weights weights) {
    float min_dist = FLT_MAX;
    Cluster *closest_cluster_1 = NULL;
    uint32_t closest_cluster_2_idx;

    for (uint32_t c1_idx = 0; c1_idx < cluster_arr->length; c1_idx++) {
        Cluster *cluster_1 = cluster_arr->items[c1_idx];
        for (uint32_t c2_idx = c1_idx + 1; c2_idx < cluster_arr->length;
             c2_idx++) {
            Cluster *cluster_2 = cluster_arr->items[c2_idx];
            float c_dist = min_cluster_distance(cluster_1, cluster_2, weights);
            if (c_dist < min_dist) {
                min_dist = c_dist;
                closest_cluster_1 = cluster_1;
                closest_cluster_2_idx = c2_idx;
            }
        }
    }

    return merge_clusters(cluster_arr, closest_cluster_1,
                          closest_cluster_2_idx);
}

uint32_t get_min_flow_id(Cluster *cluster) {
    uint32_t min_flow_id = cluster->flows[0].id;

    for (uint32_t flow_idx = 1; flow_idx < cluster->length; flow_idx++) {
        uint32_t curr_flow_id = cluster->flows[flow_idx].id;
        if (curr_flow_id < min_flow_id) {
            min_flow_id = curr_flow_id;
        }
    }

    return min_flow_id;
}

int qsort_flow_cmp(const void *a, const void *b) {
    Flow *flow_1 = (Flow *)a;
    Flow *flow_2 = (Flow *)b;

    return (flow_1->id - flow_2->id);
}

void sort_flows_in_cluster(Cluster *cluster) {
    qsort(cluster->flows, cluster->length, sizeof(Flow), qsort_flow_cmp);
}

int qsort_cluster_cmp(const void *a, const void *b) {
    Cluster **cluster_1 = (Cluster **)a;
    Cluster **cluster_2 = (Cluster **)b;
    return get_min_flow_id(*cluster_1) - get_min_flow_id(*cluster_2);
}

void sort_cluster_arr(ClusterArray *cluster_arr) {
    for (uint32_t c_idx = 0; c_idx < cluster_arr->length; c_idx++) {
        sort_flows_in_cluster(cluster_arr->items[c_idx]);
    }

    qsort(cluster_arr->items, cluster_arr->length, sizeof(Cluster *),
          qsort_cluster_cmp);
}

void print_results(ClusterArray *cluster_arr) {
    sort_cluster_arr(cluster_arr);
    printf("Clusters: \n");
    for (uint32_t c_idx = 0; c_idx < cluster_arr->length; c_idx++) {
        Cluster *current_cluster = cluster_arr->items[c_idx];
        printf("cluster %d: ", c_idx);
        for (uint32_t flow_idx = 0; flow_idx < current_cluster->length;
             flow_idx++) {
            printf("%d", current_cluster->flows[flow_idx].id);
            (flow_idx == current_cluster->length - 1) ? printf("\n")
                                                      : printf(" ");
        }
    }
}

int main(int argc, char **argv) {
    if (argc != MANDATORY_ARGC_LEN && argc != OPTIONAL_ARGC_LEN) {
        fprintf(stderr,
                "Wrong arguments! Usage: ./flows INPUT [N WB WT WD WS]\n");
        return WRONG_ARGUMENTS_ERR;
    }

    Args args = {.input_path = argv[1]};

    if (argc == OPTIONAL_ARGC_LEN) {
        int temp_n_clusters = atoi(argv[2]);

        if (temp_n_clusters <= 0) {
            fprintf(stderr, "error: n of clusters must be > 0\n");
            return WRONG_N_CLUSTERS_ERR;
        }

        args.n_clusters = temp_n_clusters;
        args.weights.byte_wt = atof(argv[3]);
        args.weights.flow_duration_wt = atof(argv[4]);
        args.weights.interarrival_wt = atof(argv[5]);
        args.weights.avg_packet_wt = atof(argv[6]);

        if (args.weights.byte_wt < 0 || args.weights.flow_duration_wt < 0 ||
            args.weights.interarrival_wt < 0 ||
            args.weights.avg_packet_wt < 0) {
            fprintf(stderr, "error: weights must not be negative\n");
            return NEGATIVE_WEIGHTS_ERR;
        }
    }

    ClusterArray *cluster_arr =
        cluster_arr_from_input(args.input_path, args.n_clusters);

    if (cluster_arr == NULL) {
        return CLUSTER_ARRAY_ERR;
    }

    if (argc == OPTIONAL_ARGC_LEN) {
        while (cluster_arr->length > args.n_clusters) {
            int merge_error = merge_closest_clusters(cluster_arr, args.weights);
            if (merge_error != 0) {
                free_cluster_arr(cluster_arr);
                return merge_error;
            }
        }
    }

    print_results(cluster_arr);

    free_cluster_arr(cluster_arr);
    return 0;
}