#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Edge represents connection between two nodes */
/* Graph contains a list of nodes, and each node contains a list of edges each edge says which node it points to*/

typedef struct edges
{
    int *to_node;
} edges;

/* Node can have several edges and one value */

typedef struct 
{
    int *from_node; 
    edges **connected_to;

    node **next;
}node;

typedef struct 
{
    node** nodes; 

    int totoal_amount_of_edges;
    int amount_of_nodes;
}graph;

graph* create_new_graph(size_t size)
{
 graph *new_graph = (graph *)malloc(sizeof(graph));
 new_graph->nodes = (node **)calloc(size, sizeof(node));
 return new_graph;
}

int* count_edges(node *node)
{
    int count;
    node->connected_to;
    count++;
    return count;    
}

void add_new_subGraph(graph graph, void* from_node, void* to_node)
{
    node *new_node = calloc(1, sizeof(node));
    new_node->from_node = from_node;
    new_node ->connected_to= to_node;


}

void read_lines(const int *values, size_t size)
{
    graph* graph  = create_new_graph(size);
    for(size_t i = 0; i<size; i++)
    {
        add_new_subGraph(graph, (void *) &values[0], &values[1])
    }

}

int run_file(File *file)
{
    //Get the size of the file 
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //Allocate memory
    int* graph = create_new_graph(size);

    fread(graph, sizeof(int), size, file);
    read_lines(graph,(node *) new_node, )

}
