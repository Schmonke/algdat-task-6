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


/* Edge represents connection between two nodes */
/* Graph contains a list of nodes, and each node contains a list of edges each edge says which node it points to*/
/**
 * Linked list containing all the edges that correspond to a node.
 */
typedef struct edge
{
    struct edge *next;
    int dstNodeId;
} edge;

typedef struct node
{
    char* text;
    int node_number;
    struct edge *edges;

    // BFS
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

typedef struct topo_lst
{
   struct node *next;
   int found;
}topo_lst;

const int QUEUE_DEFAULT_CAPACITY = 16;
typedef struct 
{
    void **elems;
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
    q->elems = calloc(capacity, sizeof(void*));
    q->capacity = capacity;
    q->count = 0;
    return q;
}

void queue_push(queue *q, void *elem)
{
    if (q->count + 1 > q->capacity)
    {
        q->capacity *= 2;
        void** old = q->elems;
        q->elems = calloc(q->capacity, sizeof(void*));
        memcpy(q->elems, old, q->count * sizeof(void*));
        free(old);
    }
    q->elems[q->count++] = elem;
}

void *queue_pop(queue *q, bool *found)
{
    if (q->count == 0)
    {
        if (found != NULL) *found = false;
        return NULL;
    }
    if (found != NULL) *found = true;
    return q->elems[--q->count];
}

void queue_free(queue *q)
{
    free(q->elems);
    free(q);
}


//Datastructure for search
#define infinity 1000000000

//Do we need this method???
graph* new_graph(int node_count)
{
    graph *new_graph = (graph *)malloc(sizeof(graph));
    new_graph->node_count = node_count;
    new_graph->nodes = (node **)calloc(node_count, sizeof(node));
    return new_graph;
}

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

//Is this method necessary??
void read_lines(const int *values, size_t size)
{
    graph *new_graph  = create_new_graph(size);
    for(size_t i = 0; i<size; i++)
    {
        add_new_subGraph(new_graph, (void *) &values[0], &values[1]);
    };
}

//Burde calle en print method her for å printe resultatet fra søket?
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
        int n_id = queue_pop(q, NULL);
        node *n = &g->nodes[n_id];

        // look over edges and push to queue
        edge *e = n->edges;
        while (!found && e != NULL)
        {
            int targetNodeId = e->dstNodeId;
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

void print_distance_prev(graph *g, node* start)
{
    printf(" Node  Prev  Dist");
    for(int i=0; i < g->node_count; i++)
    {
        node *n = &g->nodes[i];
        printf("%d | %d | %d \n",n->node_number, n->prevNodeId, n->dist);
    }
}
/* DFS help method for topological sort */
node* df_topo(node *n, node *l)
{
    topo_lst *tpl=n->edges->next;
    if(n->found) return l;

    tpl->found = 1;
    
    for(edge *e=n->edges; e; e=e->next)
    {
        l = df_topo(e->next, l);
    }
    tpl->next = l;
    return n;
}

/**
 * Topological sort function
 */
node *topologicalSort(graph *g)
{
    node *end = 0;
    for(int i = g->node_count - 1; i--;)
    {node
        g->nodes[i] = calloc(sizeof(topo_lst), 1); // void* cannot be assigned to entity of type 
    }
    for(int i = g->node_count - 1; i--;)
    {
       end = df_topo(&g->node_count[i], end); //  expression must have pointer-to-object type but has type int [i]
    }
    return end;
}

void print_topologica_sorted_graph(node* n)
{
    
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

    for (int l = 0; i < length; l++)
    {
        int node_id = atoi(&data[i]);

        if (find_next_token(data, length, &i)) continue;
        int edge_dst_id = atoi(&data[i]);
        if (node_id < node_count && edge_dst_id < node_count)
        {
            node *n = &g->nodes[node_id];
            edge *e = malloc(sizeof(edge));
            e->next = n->edges;
            n->edges = e;
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

graph* run_with_files(const char *graphfile, const char *namefile)
{
    graph *g = parse_graphfile(graphfile);
    if (g == NULL)
    {
        perror("Failed to parse graph file.");
    }
    if (namefile != NULL)
    {
        parse_namegraph(g, namefile);
    }
    return graph;
}

int main(int argc, const char *argv[])
{
    const char *graphfile = argc > 1 ? argv[1] : NULL;
    const char *namefile = argc > 2 ? argv[2] : NULL;

    if (graphfile == NULL)
    {
        printf(
            "You must provide a graph file and optionally a name file.\n"
            "Usage: ./graph <graphfile> [namefile]\n"
        );
        return 1;
    }
    graph* graph = run_with_files(graphfile, namefile);

        
    return 0;
}