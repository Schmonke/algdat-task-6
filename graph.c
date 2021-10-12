#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>


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
    unsigned int edge_count;

    // BFS
    bool found;
    bool visited;
    int prevNodeId;
    int dist;
} node;

// Graph 
typedef struct graph
{
    node *nodes; 
    int node_count;
} graph;


const int QUEUE_DEFAULT_CAPACITY = 16;
typedef struct 
{
    int *elems;
    int capacity;
    int count;
} queue;

edge *new_edge(int dst_node_id, edge* next)
{
    edge* e = malloc(sizeof(edge));
    e->dst_node_id = dst_node_id;
    e->next = next;
    return e;
}

/**
 * Adds an edge to the specified node's edges linked list.
 * If list is NOT empty it iterates throug each element in
 * list until list head is found. Adds new edge to list head.
 */
void edge_add(node *n, int dst_node_id)
{
    if(n->edges == NULL)
    {
        n->edges = new_edge(dst_node_id, NULL);
    }
    else
    {
        edge *e = n->edges;
        while (e->next)
        {
            e = e->next;
        }
        e->next = new_edge(dst_node_id, NULL);
    }
}

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

int queue_peek(queue *q)
{
    if (q->count == 0)
    {
        return 0;
    }
    return q->elems[q->count - 1];
}

int queue_pop(queue *q)
{
    int result = queue_peek(q);
    q->count--;
    return result;
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
        int n_id = (int)queue_pop(q);
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
    printf("Node  Prev  Dist\n");
    for(int i=0; i < g->node_count; i++)
    {
        node *n = &g->nodes[i];
        printf(" %d | %d | %d \n", n->node_number, n->prevNodeId, n->dist);
    }
}
static int count = 1;
int dfs_rec(graph *g, queue *q, int node_id)
{
    node *top_node = &g->nodes[node_id];
    if (!top_node->visited)
    {
        //printf("%d\n", count++);
        top_node->visited = true;
        for (edge *e=top_node->edges; e; e=e->next)
        {
            int next_id = e->dst_node_id;
            node *inner_node = &g->nodes[next_id];
            if (!inner_node->visited)
            {
                //printf("Calling rec - node: %d\n", next_id);
                dfs_rec(g, q, next_id);
            }
        }
        //printf("%d\n", node_id);
        queue_push(q, node_id);
    }
    return node_id;
}
 
queue *dfs_topo(graph *g)
{
    queue *q = new_queue(g->node_count - 1);

    // clear visited flag on all nodes in graph
    for(int i=0; i<g->node_count; i++)
    {
        g->nodes[i].visited= false;
    }
    for(int i = 0; i < g->node_count; i++)
    {
        //printf("%d\n", i);
        dfs_rec(g, q, i);
    }
    return q;
}


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

    for (int i = 0; i < node_count; i++)
    {
        g->nodes[i].node_number = i;
    }
    
    while (i < length)
    {
        int node_id = atoi(&data[i]);

        if (find_next_token(data, length, &i)) continue;
        int edge_dst_id = atoi(&data[i]);
        if (node_id < node_count && edge_dst_id < node_count)
        {
            node *n = &g->nodes[node_id];
            n->node_number = node_id;
            edge_add(n, edge_dst_id);
        }
        
        if (find_next_token(data, length, &i)) continue;
        if (node_id < node_count)
        {
            g->nodes[node_id].text = parse_string(data, length, &i);
        }
    }

    munmap(data, length);
    return g;
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

void print_queue(queue *q)
{
    printf("Topological order: ");
    bool t = true;
    while (q->count > 0)
    {
        printf("%d ", queue_pop(q));
    }
    printf("\n");

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
    queue *q = dfs_topo(graph);
    print_queue(q);
    bfsv2(graph, src_node, dst_node);
    print_distance_prev(graph);

    //print_graph(graph);



    //graph* t = graph_transpose(graph);
    //print_graph(t);
    
    free(graph);
    free(q);
    return 0;
}