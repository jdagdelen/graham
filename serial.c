//
// Created by John Dagdelen on 4/28/19.
//

#include <igraph/igraph.h>
#include <gsl/gsl_combination.h>
#include <stdio.h>
#include <time.h>

#define max(x, y) ((x) >= (y)) ? (x) : (y)
#define min(x, y) ((x) <= (y)) ? (x) : (y)

int MAXDEGREE = 6;

/* destroys a list of igraph_t objects */
void free_graphs_in_vector(igraph_vector_ptr_t *graphlist) {
    long int i;
    for (i = 0; i < igraph_vector_ptr_size(graphlist); i++) {
        igraph_destroy(VECTOR(*graphlist)[i]);
        free(VECTOR(*graphlist)[i]);
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
                for (int j = 0; j < combo_length * 2; j += 2) {
                    VECTOR(edge_list)[j] = (igraph_real_t) gsl_combination_data(c)[j / 2];
                    VECTOR(edge_list)[j] = n;
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


void filter_unique(igraph_vector_ptr_t *clusters,
                   igraph_vector_ptr_t *candidates,
                   igraph_vector_ptr_t *unique
) {
    for (int i = 0; i < igraph_vector_ptr_size(candidates); i++) {
        igraph_t *g1 = VECTOR(*candidates)[i];
        if (g1 == NULL){
            continue;
        }
        if (i < igraph_vector_ptr_size(candidates) - 1) {
            for (int j = i + 1; j < igraph_vector_ptr_size(candidates); j++) {
                igraph_bool_t are_isomorphic;
                igraph_t *g2 = VECTOR(*candidates)[j];
                if (!(g1 == NULL || g2 == NULL)) {
                    igraph_isomorphic_bliss(VECTOR(*candidates)[i], VECTOR(*candidates)[j],
                                            &are_isomorphic, NULL, NULL, IGRAPH_BLISS_F, IGRAPH_BLISS_F, NULL, NULL);
                    if (are_isomorphic) {
                        VECTOR(*candidates)[j] = NULL;
                    }
                }
            }
        }
        igraph_vector_ptr_push_back(clusters, g1);
        igraph_vector_ptr_push_back(unique, g1);
    }
}



int main(void) {
    igraph_t graph;
    igraph_small(&graph, 0, IGRAPH_UNDIRECTED, 0, 1, -1);
    igraph_vector_t open;
    igraph_vector_init(&open, igraph_vcount(&graph));
    igraph_vector_ptr_t candidates, clusters, unique, seeds;

    igraph_vector_ptr_init(&clusters, 1000);
    igraph_vector_ptr_init(&candidates, 100);
    igraph_vector_ptr_init(&unique, 100);

    igraph_vector_ptr_clear(&clusters);
    igraph_vector_ptr_clear(&candidates);
    igraph_vector_ptr_clear(&unique);
    igraph_vector_ptr_push_back(&clusters, &graph);
    igraph_vector_ptr_push_back(&unique, &graph);
    double total_time, generation_time, filter_time;
    clock_t tt, gt, ft;
    long num_unique_found, total_number, num_generated_in_step;

    printf("%10s %10s %10s %10s %10s %10s %10s\n", "N", "candidates", "gen_time", "unique", "filter_time", "total_found", "total_time");
    tt = clock();
    for (int N = 3; N < 10; N++) {
        igraph_vector_ptr_clear(&candidates);
        gt = clock();
        for (int i = 0; i < igraph_vector_ptr_size(&unique); i++) {
            mutate_seed(VECTOR(unique)[i], &candidates);

        }
        num_generated_in_step = igraph_vector_ptr_size(&candidates);
        igraph_vector_ptr_clear(&unique);
        generation_time = ((double)gt)/CLOCKS_PER_SEC;

        ft = clock();
        filter_unique(&clusters, &candidates, &unique);
        num_unique_found = igraph_vector_ptr_size(&unique);
        filter_time = ((double)ft)/CLOCKS_PER_SEC;

        tt=clock();
        total_number = igraph_vector_ptr_size(&clusters);
        total_time += ((double)tt)/CLOCKS_PER_SEC;

        printf("%10i %10li %10f %10li %10f %10li %10f\n",
               N,
               num_generated_in_step,
               generation_time,
               num_unique_found,
               filter_time,
               total_number,
               total_time);
    }

    igraph_vector_ptr_destroy(&candidates);
    igraph_destroy(&graph);
    return 0;
}
