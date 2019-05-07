//
// Created by John Dagdelen on 4/28/19.
//

#include <igraph/igraph.h>
#include <gsl/gsl_combination.h>
#include <stdio.h>


// Returns the "available" sites for a new vertex to be connected to
// (sites are available if they have fewer than 6 neighbors)
void get_open_sites(igraph_t *seed, igraph_vector_t *open){
    igraph_vector_clear(open);
    igraph_vector_t degrees;
    igraph_vector_init(&degrees, igraph_vcount(seed));
    igraph_degree(seed, &degrees, igraph_vss_all(), IGRAPH_ALL, 0);
    for(int i=0; i<igraph_vcount(seed); i++) {
        if(VECTOR(degrees)[i]<6)
            igraph_vector_push_back(open, i);
    }
    igraph_vector_destroy(&degrees);
//    igraph_vector_print(open);
}

void mutate_seed(igraph_t *seed, igraph_vector_ptr_t *candidates){
    // gets all combinations of up to 6 open vertices to connect new vertex to
    // and creates a new graph for each case.

    gsl_combination * c;
    igraph_vector_t open_sites;
    igraph_vector_init(&open_sites, igraph_vcount(seed));
    get_open_sites(seed, &open_sites);
    int n = igraph_vector_size(&open_sites);
    if (n > 0) {
        for (int i = 1; i <= 6; i++) {
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

                igraph_t candidate;
                igraph_copy(&candidate, seed);
                igraph_add_vertices(&candidate, 1, 0);
                igraph_add_edges(&candidate, &edge_list, 0);
                printf("%i\n", igraph_vcount(&candidate));
                igraph_vector_ptr_push_back(candidates, &candidate);
                printf("x %i\n", igraph_vcount(VECTOR(*candidates)[0]));
            } while (gsl_combination_next(c) == GSL_SUCCESS);
            gsl_combination_free(c);
        }
    }
}


int main(void) {
    igraph_t graph;
    igraph_small(&graph, 0, IGRAPH_UNDIRECTED, 0,1, 1,2, 2,3, 3,4, 6,1, -1);
    igraph_vector_t open;
    igraph_vector_init(&open, igraph_vcount(&graph));
    igraph_vector_ptr_t candidates;
    igraph_vector_ptr_init(&candidates, 1000);
    igraph_vector_ptr_clear(&candidates);

    igraph_vector_ptr_t new_candidates;
    igraph_vector_ptr_init(&new_candidates, 1000);
    igraph_vector_ptr_clear(&new_candidates);

    printf("%i\n", igraph_vector_ptr_size(&candidates));
    mutate_seed(&graph, &candidates);
    printf("%i\n", igraph_vector_ptr_size(&candidates));

    printf("Got here also.\n");

    igraph_vector_ptr_destroy(&candidates);
    igraph_destroy(&graph);
    return 0;
}