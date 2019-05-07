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


//void get_possible_edge_lists(igraph_t *seed){
//    gsl_combination * c;
//    size_t i;
//    int n = igraph_vcount(seed);
//    c = gsl_combination_calloc(4, i);
//}


igraph_vector_ptr get_possible_changes(igraph_vector_t *open_sites)


void mutate_seed(igraph_t *seed, igraph_vector_ptr_t *results){
    // get all combinations of up to 6 open vertices to connect new vertex to


    igraph_vector_ptr new_edge_vectors;


}


int main(void) {
    igraph_t graph;
    igraph_small(&graph, 0, IGRAPH_UNDIRECTED, 0,1, 1,2, 2,3, 3,4, 6,1, -1);
    igraph_vector_t open;
    igraph_vector_init(&open, igraph_vcount(&graph));
    get_open_sites(&graph, &open);

    return 0;
}