//
// Created by John Dagdelen on 4/28/19.
//

#include <igraph/igraph.h>
#include <gsl/gsl_combination.h>
#include <stdio.h>

#define max(x,y) ((x) >= (y)) ? (x) : (y)
#define min(x,y) ((x) <= (y)) ? (x) : (y)

/* destroys a list of igraph_t objects */
void free_graphlist(igraph_vector_ptr_t *graphlist) {
    long int i;
    for (i=0; i<igraph_vector_ptr_size(graphlist); i++) {
        igraph_destroy(VECTOR(*graphlist)[i]);
        free(VECTOR(*graphlist)[i]);
    }
    igraph_vector_ptr_destroy(graphlist);
}

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

void print_vertices(igraph_t *graph){
    igraph_vs_t vs;
    igraph_vit_t vit;
    igraph_vs_seq(&vs, 0, igraph_vcount(graph));
    igraph_vit_create(graph, vs, &vit);
    while (!IGRAPH_VIT_END(vit)) {
        printf(" %li", (long int)IGRAPH_VIT_GET(vit));
        IGRAPH_VIT_NEXT(vit);
    }
    printf("\n");
    igraph_vit_destroy(&vit);
    igraph_vs_destroy(&vs);
}

void mutate_seed(igraph_t *seed, igraph_vector_ptr_t *candidates){
    // gets all combinations of up to 6 open vertices to connect new vertex to
    // and creates a new graph for each case.

    gsl_combination * c;
    igraph_vector_t open_sites;
    igraph_vector_init(&open_sites, igraph_vcount(seed));
    igraph_t *candidate;
    get_open_sites(seed, &open_sites);
    int n = igraph_vector_size(&open_sites);
    int new_graphs = 0;
    if (n > 0) {
        int m = min(n, 6);
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
    printf("new graphs added: %i\n", new_graphs);
}




int main(void) {
    igraph_t graph;
    igraph_small(&graph, 0, IGRAPH_UNDIRECTED, 0,1, -1);
    igraph_vector_t open;
    igraph_vector_init(&open, igraph_vcount(&graph));
    igraph_vector_ptr_t candidates;
    igraph_vector_ptr_init(&candidates, 1000);
    igraph_vector_ptr_clear(&candidates);
    printf("%li\n", igraph_vector_ptr_size(&candidates));
    mutate_seed(&graph, &candidates);
    printf("%li\n", igraph_vector_ptr_size(&candidates));
    for(int round = 0; round < 10; round++) {
        int m = igraph_vector_ptr_size(&candidates);
        for (int i = 0; i < m; i++) {
            mutate_seed((igraph_t *) VECTOR(candidates)[i], &candidates);
            printf("Round %i, %li graphs\n", round, igraph_vector_ptr_size(&candidates));
            //        print_vertices((igraph_t*) VECTOR(candidates)[igraph_vector_ptr_size(&candidates)-1]);
        }
    }
    igraph_vector_ptr_destroy(&candidates);
    igraph_destroy(&graph);
    return 0;
}