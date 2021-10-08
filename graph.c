#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>


/* Edge represents connection between two nodes
 * Graph contains an array of nodes, and each node contains a linked-list of edges
 * where each edge says which node it points to.
 */

/**
 * Linked list containing all the edges that correspond to a node.
 */
typedef struct edge
{
    struct edge *next;
    int dst_node_id;
} edge;

typedef struct node
{
    char* text;
    int node_number;
    struct edge *edges;

    // BFS
    bool found;
    bool visited;
    int prevNodeId;
    int dist;
} node;

// Graph 
typedef struct graph
{
    node* nodes; 
    int node_count;
} graph;


const int QUEUE_DEFAULT_CAPACITY = 16;
typedef struct 
{
    int *elems;
    int capacity;
    int count;
} queue;

queue *new_queue(int capacity)
{
    if (capacity <= 0)
    {
        capacity = QUEUE_DEFAULT_CAPACITY;
    }
    queue *q = calloc(1, sizeof(queue));
    q->elems = calloc(capacity, sizeof(int));
    q->capacity = capacity;
    q->count = 0;
    return q;
}

edge *new_edge(int dst_node_id, edge* next)
{
    edge* e = malloc(sizeof(edge));
    e->dst_node_id = dst_node_id;
    e->next = next;
    return e;
}

void queue_push(queue *q, int nodeId)
{
    if (q->count + 1 > q->capacity)
    {
        q->capacity *= 2;
        int *old = q->elems;
        q->elems = calloc(q->capacity, sizeof(int));
        memcpy(q->elems, old, q->count * sizeof(int));
        free(old);
    }
    q->elems[q->count++] = nodeId;
}

int queue_peek(queue *q, bool *found)
{
    if (q->count == 0)
    {
        if (found != NULL) *found = false;
        return 0;
    }
    if (found != NULL) *found = true;
    return q->elems[q->count];
}

int queue_pop(queue *q, bool *found)
{
    bool f = false;
    queue_peek(q, &f);
    if (f) q->count--;
    if (found != NULL) *found = f;
}

void queue_free(queue *q)
{
    free(q->elems);
    free(q);
}


//Datastructure for search
#define infinity 1000000000

int count_edges(node* n)
{
    int count;
    edge *temp = n->edges;
    while (temp!=NULL)
    {
        count++;
        temp=temp->next;
    }  
    return count;
}

//Do we need this method????
void set_text_value(node *n, void* text)
{
    n->text=(char*) text;
}

void bfsv2(graph *g, int srcNodeId, int dstNodeId)
{
    queue *q = new_queue(g->node_count - 1);
    
    // push initial node onto queue for searching
    node *src = &g->nodes[srcNodeId];
    src->visited = true;
    src->prevNodeId = -1;

    queue_push(q, srcNodeId);
    
    bool found = false;

    // look over nodes
    while (!found && q->count != 0)
    {
        int n_id = (int)queue_pop(q, NULL);
        node *n = &g->nodes[n_id];

        // look over edges and push to queue
        edge *e = n->edges;
        while (!found && e != NULL)
        {
            int targetNodeId = e->dst_node_id;
            node *target = &g->nodes[targetNodeId];
            e = e->next;
            
            // push for deeper search if not already visited
            if (target->visited) continue;
            target->prevNodeId = n_id;
            target->dist = n->dist + 1;
            queue_push(q, targetNodeId);

            // target node found? (⌐■_■)
            if (targetNodeId == dstNodeId)
            {
                found = true;
                break;
            }
        }
    }
}

void print_distance_prev(graph *g)
{
    printf(" Node  Prev  Dist");
    for(int i=0; i < g->node_count; i++)
    {
        node *n = &g->nodes[i];
        printf("%d | %d | %d \n",n->node_number, n->prevNodeId, n->dist);
    }
}
//NEXT time: finn endenode, kjør dybde først søk så print it
graph* graph_transpose(graph *g)
{
    const int node_count = g->node_count;
    graph *t = (graph *){
        .node_count = node_count,
        .nodes = calloc(node_count, sizeof(node)),
    };

    for (int i = 0; i < node_count; i++)
    {
        t.nodes[i] = g->nodes[i];
    }
    
    // loop over all the nodes
    // get the edges for that node, and reverse the direction:
    //      index is source and dst_node_id is destination
    // thereafter we set the edge into the dest node
    for (int i = 0; i < node_count; i++)
    {   
        for(edge *e=g.nodes[i]->edges; e; e=e->next)
        {
        int dst_id = g->nodes[i].edges->dst_node_id;
        edge *next_edge = e->next; //maybe is right?
        t.nodes[dst_id]->edges = new_edge(dst_id, next_edge);
        }
    }
    return t;    
}



 //DFS help method for topological sort 
void dfs_topo(graph *g, int start_id)
{
    queue *q = new_queue(0);


    // clear found flag on all nodes in graph
    for(int i=0; i<g->node_count; i++)
    {
        g->nodes[i].visited= false;
    }

    node* next_node = &g->nodes[start_id];
    queue_push(q, start_id);
    bool found = false;
    
    while (!found)
    {
        int node_id = queue_peek(q, NULL); //get last node_id in the queue
        node *next = &g->nodes[node_id];
        next->visited = true;
        bool is_leaf = next->edges == NULL;

        bool found_unvisited = false;
        // Find an unvisited node and enqueue it
        for (edge *e=next_node->edges; e; e=e->next)
        {
            int next_id = e->dst_node_id;
            node* n = &g->nodes[next_id];
            if (!n->visited)
            {
                //Visits next unvisited node
                queue_push(q, e->dst_node_id);
                found_unvisited = true;
            }
        }
        if (!found_unvisited)
        {
            queue_pop(q, NULL);
        }
        
        
        // Is leaf node? Add to list and go back up.
        // Has edges that point to unvisited nodes? Continue down until an
        // unvisited node is found.
    }
}


/*
 Former method
 
node* topologicalSort(graph *g)
{
    node *end = 0;
    for(int i = g->node_count - 1; i--;)
    {
       g->nodes[i] = (node *)calloc(1, sizeof(q)); // void* cannot be assigned to entity of type 
    }
    for(int i = g->node_count - 1; i--;)
    {
       end = df_topo(&g->node_count[&i], end); //  expression must have pointer-to-object type but has type int
    }
    return end;
}
*/


/**
 * Parsing and stuff
 */
bool find_next_token(char *data, int length, int *index)
{
    bool nextline = false;  
    int i = *index;
    while (i < length && !isspace(data[i]))
    {
        i++;
    }
    while (i < length && isspace(data[i]))
    {
        if (data[i] == '\n')
        {
            nextline = true;
        }
        i++;
    }
    *index = i;
    return nextline;
}

char *parse_string(char *data, int length, int *index)
{
    int i = *index;
    if (i >= length) return NULL;
    if (data[i] != '\"') return NULL;

    int start = ++i;
    int slen = -1;

    for (int i = start; i < length; i++)
    {
        if (data[i] == '\"')
        {
            slen = (i - start);
            break;
        }
    }
    if (slen == -1)
    {
        return NULL;
    }
    *index = i;

    char *str = malloc(slen + 1);
    str[slen] = '\0';
    memcpy(str, &data[start], slen);
    return str;
}

char *mmap_file(const char *filename, int *length)
{
    int fd;
    if ((fd = open(filename, O_RDONLY)) == -1)
    {
        return MAP_FAILED;
    }
    int len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *data = (char*)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0); //NULL means the kernel picks starting addr
    close(fd);
    
    printf("mmap: %p %d", data, len);
    perror("NO");
    
    *length = len;
    return data;
}

graph *parse_graphfile(const char *graphfile)
{
    int i = 0;
    int length;
    char *data = mmap_file(graphfile, &length);
    if (data == MAP_FAILED)
    {
        return NULL;
    }

    // read header line
    int node_count = atoi(&data[i]);
    find_next_token(data, length, &i);
    int edge_count = atoi(&data[i]);
    find_next_token(data, length, &i);
    
    graph *g = calloc(1, sizeof(graph));
    g->node_count = node_count;
    g->nodes = calloc(node_count, sizeof(node));

    for (int l = 0; i < length; l++)
    {
        int node_id = atoi(&data[i]);

        if (find_next_token(data, length, &i)) continue;
        int edge_dst_id = atoi(&data[i]);
        if (node_id < node_count && edge_dst_id < node_count)
        {
            node *n = &g->nodes[node_id];
            edge *e = malloc(sizeof(edge));
            e->next = new_edge(edge_dst_id, n->edges);
        }

        if (find_next_token(data, length, &i)) continue;
        if (node_id < node_count)
        {
            g->nodes[node_id].text = parse_string(data, length, &i);
        }

        find_next_token(data, length, &i);
    }

    munmap(data, length);
}

void parse_namefile(graph *g, const char *graphfile)
{
    int i = 0;
    int length;
    char *data = mmap_file(graphfile, &length);

    // read header line
    int node_count = atoi(&data[i]);
    find_next_token(data, length, &i);

    for (int l = 0; i < length; l++)
    {
        int node_id = atoi(&data[i]);

        if (find_next_token(data, length, &i)) continue;
        if (find_next_token(data, length, &i)) continue;

        if (node_id < node_count)
        {
            g->nodes[node_id].text = parse_string(data, length, &i);
        }

        find_next_token(data, length, &i);
    }

    munmap(data, length);
}

graph* parse_graph(const char *graphfile, const char *namefile)
{
    graph *g = parse_graphfile(graphfile);
    if (g == NULL)
    {
        perror("Failed to parse graph file");
    }
    if (namefile != NULL)
    {
        parse_namefile(g, namefile);
    }
    return g;
}

int main(int argc, const char *argv[])
{
    const int src_node = argc > 1 ? atoi(argv[1]) : -1;
    const int dst_node = argc > 2 ? atoi(argv[2]) : -1;
    const char *graphfile = argc > 3 ? argv[3] : NULL;
    const char *namefile = argc > 4 ? argv[4] : NULL;

    if (src_node == -1 || dst_node == -1 || graphfile == NULL)
    {
        printf(
            "You must provide a graph file and optionally a name file.\n"
            "Usage: ./graph <src-node> <dst-node> <graphfile> [namefile]\n"
        );
        return 1;
    }
    graph* graph = parse_graph(graphfile, namefile);
    bfsv2(graph, src_node, dst_node);
    print_distance_prev(graph);
    topologicalSort(graph);

    return 0;
}