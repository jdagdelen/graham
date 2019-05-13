//
// Created by John Dagdelen on 4/28/19.
//

#include <igraph/igraph.h>
#include <gsl/gsl_combination.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>

#define max(x, y) ((x) >= (y)) ? (x) : (y)
#define min(x, y) ((x) <= (y)) ? (x) : (y)
#define true 1
#define false 0

int MAXDEGREE = 4;
int MAXN = 8;

/* destroys a list of igraph_t objects */
void free_graphs_in_vector(igraph_vector_ptr_t *graphlist) {
    long int i;
    for (i = 0; i < igraph_vector_ptr_size(graphlist); i++) {
        igraph_destroy(VECTOR(*graphlist)[i]);
    }
}

// Returns the "available" sites for a new vertex to be connected to
// (sites are available if they have fewer than 6 neighbors)
void get_open_sites(igraph_t *seed, igraph_vector_t *open) {
    igraph_vector_clear(open);
    igraph_vector_t degrees;
    igraph_vector_init(&degrees, igraph_vcount(seed));
    igraph_degree(seed, &degrees, igraph_vss_all(), IGRAPH_ALL, 0);
    for (int i = 0; i < igraph_vcount(seed); i++) {
        if (VECTOR(degrees)[i] < MAXDEGREE)
            igraph_vector_push_back(open, i);
    }
    igraph_vector_destroy(&degrees);
//    igraph_vector_print(open);
}

void print_vertices(igraph_t *graph) {
    igraph_vs_t vs;
    igraph_vit_t vit;
    igraph_vs_seq(&vs, 0, igraph_vcount(graph));
    igraph_vit_create(graph, vs, &vit);
    while (!IGRAPH_VIT_END(vit)) {
        printf(" %li", (long int) IGRAPH_VIT_GET(vit));
        IGRAPH_VIT_NEXT(vit);
    }
    printf("\n");
    igraph_vit_destroy(&vit);
    igraph_vs_destroy(&vs);
}

/* Returns true if two graphs are isomorphic, otherwise returns false */
igraph_bool_t isomorphic(igraph_t *g1, igraph_t *g2) {
    igraph_bool_t iso;
    igraph_isomorphic_bliss(g1, g2, &iso, NULL, NULL, IGRAPH_BLISS_F, IGRAPH_BLISS_F, NULL, NULL);
    return iso;
}

/* Removes all duplicate graphs (isomorphic) from a vector of graphs*/
void reduce_isomorphic(igraph_vector_ptr_t *graphs) {
    igraph_vector_ptr_t unique;
    igraph_vector_ptr_init(&unique, 100);
    igraph_vector_ptr_clear(&unique);
    for (int i = 0; i < igraph_vector_ptr_size(graphs); i++) {
        igraph_t *g1 = VECTOR(*graphs)[i];
        // handle graphs that have already been found
        if (g1 == NULL) {
            continue;
        }
        // handle all possible pairs of graphs
        if (i < igraph_vector_ptr_size(graphs) - 1) {
            for (int j = i + 1; j < igraph_vector_ptr_size(graphs); j++) {
                igraph_t *g2 = VECTOR(*graphs)[j];
                if (!(g1 == NULL || g2 == NULL)) {
                    if (isomorphic(g1, g2)) {
                        igraph_destroy(VECTOR(*graphs)[j]);
                        VECTOR(*graphs)[j] = NULL;
                    }
                }
            }
        }
        // finally, keep the last graph
        igraph_vector_ptr_push_back(&unique, g1);
    }
    igraph_vector_ptr_clear(graphs);
    igraph_vector_ptr_copy(graphs, &unique);
    igraph_vector_ptr_destroy(&unique);
}

void igraph_vector_ptr_combine(igraph_vector_ptr_t *v1, igraph_vector_ptr_t *v2) {
    for (int i = 0; i < igraph_vector_ptr_size(v2); i++) {
        igraph_vector_ptr_push_back(v1, VECTOR(*v2)[i]);
    }
    igraph_vector_ptr_clear(v2);
}

void mutate_seed(igraph_t *seed, igraph_vector_ptr_t *candidates) {
    // gets all combinations of up to 6 open vertices to connect new vertex to
    // and creates a new graph for each case.
    gsl_combination *c;
    igraph_vector_t open_sites;
    igraph_vector_init(&open_sites, igraph_vcount(seed));
    igraph_t *candidate;
    get_open_sites(seed, &open_sites);
    int n = igraph_vector_size(&open_sites);
    int new_graphs = 0;
    if (n > 0) {
        int m = min(n, MAXDEGREE);
        for (int i = 1; i <= m; i++) {
            c = gsl_combination_calloc(n, i);
            do {
                int combo_length = gsl_combination_k(c);
                igraph_vector_t edge_list;
                igraph_vector_init(&edge_list, combo_length * 2);
                igraph_vector_clear(&edge_list);
                for (int j = 0; j < combo_length * 2; j += 2) {
                    igraph_vector_push_back(&edge_list, (igraph_real_t) gsl_combination_get(c, j / 2));
                    igraph_vector_push_back(&edge_list, (igraph_real_t) n);
                }
                //            igraph_vector_print(&edge_list);
                candidate = igraph_Calloc(1, igraph_t);
                igraph_copy(candidate, seed);
                igraph_add_vertices(candidate, 1, 0);
                igraph_add_edges(candidate, &edge_list, 0);
                igraph_vector_ptr_push_back(candidates, candidate);
                new_graphs++;
            } while (gsl_combination_next(c) == GSL_SUCCESS);
            gsl_combination_free(c);
        }
    }
    igraph_vector_destroy(&open_sites);
}


void filter_unique(igraph_vector_ptr_t *graphs,
                   igraph_vector_ptr_t *unique) {
    int n_candidates = igraph_vector_ptr_size(graphs);
    int* found = calloc(n_candidates, sizeof(int));

    #pragma omp parallel
    for (int i = 0; i < n_candidates; i++) {
        // handle graphs that have already been found
        if (!found[i] && i < n_candidates - 1) {
            igraph_t *g1 = VECTOR(*graphs)[i];
            // handle all possible pairs of graphs
            if (i < igraph_vector_ptr_size(graphs) - 1) {
                #pragma omp for
                for (int j = i + 1; j < n_candidates; j++) {
//                    printf("got inside the loop\n");
                    igraph_t *g2 = VECTOR(*graphs)[j];
                    if (!found[j]) {
                        if (isomorphic(g1, g2)) {
                            found[j] = true;
                        }
                    }
                }
                #pragma omp barrier
                if (!found[i]){
                    igraph_vector_ptr_push_back(unique, g1);
                }
            }
        }
        else if (i == n_candidates - 1 && !found[i]) {
            // finally, keep the last graph
            igraph_vector_ptr_push_back(unique, VECTOR(*graphs)[i]);
        } else{
            continue;
        }
    }
    free(found);
}

//void filter_unique(igraph_vector_ptr_t *clusters,
//                   igraph_vector_ptr_t *candidates,
//                   igraph_vector_ptr_t *unique
//) {
//    for (int i = 0; i < igraph_vector_ptr_size(candidates); i++) {
//        igraph_t *g1 = VECTOR(*candidates)[i];
//        if (g1 == NULL){
//            continue;
//        }
//        if (i < igraph_vector_ptr_size(candidates) - 1) {
//            for (int j = i + 1; j < igraph_vector_ptr_size(candidates); j++) {
//                igraph_bool_t are_isomorphic;
//                igraph_t *g2 = VECTOR(*candidates)[j];
//                if (!(g1 == NULL || g2 == NULL)) {
//                    igraph_isomorphic_bliss(VECTOR(*candidates)[i], VECTOR(*candidates)[j],
//                                            &are_isomorphic, NULL, NULL, IGRAPH_BLISS_F, IGRAPH_BLISS_F, NULL, NULL);
//                    if (are_isomorphic) {
//                        VECTOR(*candidates)[j] = NULL;
//                    }
//                }
//            }
//        }
//        igraph_vector_ptr_push_back(unique, g1);
//    }
//}




int write_graph(const igraph_t *graph, FILE *outstream) {

    igraph_eit_t it;

    IGRAPH_CHECK(igraph_eit_create(graph, igraph_ess_all(IGRAPH_EDGEORDER_FROM),
                                   &it));
    IGRAPH_FINALLY(igraph_eit_destroy, &it);

    fprintf(outstream, "\n");

    while (!IGRAPH_EIT_END(it)) {
        igraph_integer_t from, to;
        int ret;
        igraph_edge(graph, IGRAPH_EIT_GET(it), &from, &to);
        ret = fprintf(outstream, "%li %li ",
                      (long int) from,
                      (long int) to);
        if (ret < 0) {
            IGRAPH_ERROR("Write error", IGRAPH_EFILE);
        }
        IGRAPH_EIT_NEXT(it);
    }
    igraph_eit_destroy(&it);
    IGRAPH_FINALLY_CLEAN(1);
    return 0;
}

void write_to_file(igraph_vector_ptr_t *graphs) {

    for (int i = 0; i < igraph_vector_ptr_size(graphs); i++) {
        FILE *file = fopen("nonisomorphic.txt", "a");
        write_graph(VECTOR(*graphs)[i], file);
        igraph_destroy(VECTOR(*graphs)[i]);
        fclose(file);
    }
}


int main(void) {
    igraph_t graph;
    igraph_small(&graph, 0, IGRAPH_UNDIRECTED, 0, 1, -1);
    igraph_vector_t open;
    igraph_vector_init(&open, igraph_vcount(&graph));
    igraph_vector_ptr_t candidates, unique, seeds;

    igraph_vector_ptr_init(&candidates, 100);
    igraph_vector_ptr_init(&unique, 100);

    igraph_vector_ptr_clear(&candidates);
    igraph_vector_ptr_clear(&unique);
    igraph_vector_ptr_push_back(&unique, &graph);
    double total_time, generation_time, filter_time, write_time;
    clock_t tt, gt, ft, wt;
    long num_unique_found, total_number, num_generated_in_step;

    printf("%10s %10s %10s %10s %10s %10s %10s %10s\n",
           "N", "candidates", "gen_time", "unique", "filter_time", "total_found", "write_time", "total_time");
    tt = clock();
    total_number = 1;

    for (int N = 3; N <= MAXN; N++) {
        igraph_vector_ptr_clear(&candidates);
        gt = clock();
        for (int i = 0; i < igraph_vector_ptr_size(&unique); i++) {
            mutate_seed(VECTOR(unique)[i], &candidates);
        }
        generation_time = (double) (clock() - gt) / CLOCKS_PER_SEC;
        num_generated_in_step = igraph_vector_ptr_size(&candidates);

        wt = clock();
        write_to_file(&unique);
        write_time = (double) (clock() - wt) / CLOCKS_PER_SEC;

        free_graphs_in_vector(&unique);
        igraph_vector_ptr_clear(&unique);

        ft = clock();
        filter_unique(&candidates, &unique);
        num_unique_found = igraph_vector_ptr_size(&unique);
        filter_time = (double) (clock() - ft) / CLOCKS_PER_SEC;

        total_number += num_unique_found;
        total_time += (double) (clock() - tt) / CLOCKS_PER_SEC;

        printf("%10i %10li %10.4f %10li %10.4f %10li %10.4f %10.4f\n",
               N,
               num_generated_in_step,
               generation_time,
               num_unique_found,
               filter_time,
               total_number,
               write_time,
               total_time);
    }

    igraph_vector_ptr_destroy(&candidates);
    igraph_destroy(&graph);
    return 0;
}



