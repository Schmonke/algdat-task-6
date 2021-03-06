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

/**
 * Node containing a linked list of the edges connected to that node.
 */
typedef struct node
{
    char *text;
    int node_number;
    struct edge *edges;

    // BFS
    bool visited;
    int prevNodeId;
    int dist;
} node;

const int QUEUE_DEFAULT_CAPACITY = 16;
typedef struct queue
{
    int capacity;
    int count;
    int write_index;
    int read_index;
    int *elems;
} queue;

/**
 * Stack used by DFS for Topological Sort
 */
typedef struct stack
{
    int *elems;
    size_t capacity;
    int count;
} stack;

/**
 * Graph containing all nodes read from file
 */
typedef struct graph
{
    node *nodes;
    int node_count;
} graph;

/**
 * Constructor for a new edge
 */
edge *new_edge(int dst_node_id, edge *next)
{
    edge *e = malloc(sizeof(edge));
    e->dst_node_id = dst_node_id;
    e->next = next;
    return e;
}

/**
 * Adds an edge to the front of the linked list.
 * Adds to front insted of back because its more 
 * efficient and the list order is irelevant. 
 */
void edge_add(node *n, int dst_node_id)
{
    n->edges = new_edge(dst_node_id, n->edges);
}

/**
 * Function to count the edges in the graph
 */
int edge_count(node *n)
{
    int count = 0;
    for (edge *e = n->edges; e; e = e->next)
        count++;
    return count;
}

/**
 * Creates a new queue.
 */
queue *new_queue()
{
    queue *q = malloc(sizeof(queue));
    q->capacity = QUEUE_DEFAULT_CAPACITY;
    q->elems = malloc(sizeof(int) * QUEUE_DEFAULT_CAPACITY);
    q->count = 0;
    q->write_index = 0;
    q->read_index = 0;
    return q;
}

/**
 * Pushes a node_id onto the queue.
 */
void queue_push(queue *q, int node_id)
{
    if (q->count == q->capacity)
    {
        int old_capacity = q->capacity;
        int new_capacity = q->capacity * 2;
        int *new_elems = malloc(sizeof(int) * new_capacity);

        int cp = q->read_index;
        int t = 0;
        while (t != q->count)
        {
            new_elems[t] = q->elems[cp];
            cp = (cp + 1) % old_capacity;
            t++;
        }
        free(q->elems);

        q->capacity = new_capacity;
        q->elems = new_elems;
        q->read_index = 0;
        q->write_index = t;
    }
    q->elems[q->write_index] = node_id;
    q->write_index = (q->write_index + 1) % q->capacity;
    q->count++;
}

/**
 * Peeks the bottom-most (first) element on the queue.
 */
int queue_peek(queue *q, bool *found)
{
    if (q->count == 0)
    {
        if (found != NULL)
            *found = false;
        return 0;
    }
    if (found != NULL)
        *found = true;
    return q->elems[q->read_index];
}

/**
 * Removes and returns first value in the queue.
 * Shifts the head to point to next queue_node.  
 */
int queue_pop(queue *q, bool *found)
{
    bool f;
    int elem = queue_peek(q, &f);
    if (f)
    {
        q->read_index = (q->read_index + 1) % q->capacity;
        q->count--;
    }
    if (found != NULL)
        *found = f;
    return elem;
}

/**
 * Frees the memory of the queue before a new que is made.
 */
void queue_free(queue *q)
{
    free(q->elems);
    free(q);
}

/**
 * Creates a new stack.
 */
stack *new_stack(int capacity)
{
    stack *s = malloc(sizeof(stack));
    s->capacity = capacity;
    s->count = 0;
    s->elems = malloc(sizeof(int) * capacity);
    return s;
}

/**
 * Pushes an element onto the stack.
 */
void stack_push(stack *s, int elem)
{
    s->elems[s->count] = elem;
    s->count++;
}

/**
 * Peeks the top-most element on the stack.
 */
int stack_peek(stack *s)
{
    if (s->count == 0)
    {
        return -1;
    }
    return s->elems[s->count - 1];
}

/**
 * Pops the topmost element from the stack.
 */
int stack_pop(stack *s)
{
    int result = stack_peek(s);
    if (result >= 0)
        s->count--;
    return result;
}

/**
 * Frees the memory allocated by the elements in the stack
 */
void stack_free(stack *s)
{
    free(s->elems);
    free(s);
}
/**
 * Prints the stack
 */
void print_stack(stack *s)
{
    printf("Topological order: ");
    while (s->count > 0)
    {
        printf("%d ", stack_pop(s));
    }
    printf("\n");
}

/**
 * Resets the visited flags on the nodes before starting a search
 * with BFS or DFS/Topo Sort
 */
void graph_reset_flags(graph *g)
{
    for (int i = 0; i < g->node_count; i++)
    {
        g->nodes[i].visited = false;
        g->nodes[i].prevNodeId = -1;
        g->nodes[i].dist = 0;
    }
}
/**
 * Frees the memory allocated previously allocated from the graph
 */
void graph_free(graph *g)
{
    for (int i = 0; i < g->node_count; i++)
    {
        edge *e = g->nodes[i].edges;
        while (e)
        {
            edge *next = e->next;
            free(e);
            e = next;
        }
    }
    free(g->nodes);
    free(g);
}

/**
 * Use queue as holder of nodes to use. 
 * It represents the order the nodes are visited. 
 */
void bfsv2(graph *g, int srcNodeId)
{
    graph_reset_flags(g);

    queue *q = new_queue();

    // push initial node onto queue for searching
    node *src = &g->nodes[srcNodeId];
    src->visited = true;

    queue_push(q, srcNodeId);

    // look over nodes
    while (true)
    {
        bool found;
        int n_id = (int)queue_pop(q, &found);
        if (!found)
            break;

        node *n = &g->nodes[n_id];
        n->visited = true;

        // look over edges and push to queue

        for (edge *e = n->edges; e; e = e->next) //Stops when all edges of a node are visited.
        {
            int targetNodeId = e->dst_node_id;
            node *target = &g->nodes[targetNodeId];

            if (target->visited)
                continue;

            target->prevNodeId = n_id;
            target->dist = n->dist + 1;
            queue_push(q, targetNodeId);
            target->visited = true;
        }
    }
    queue_free(q);
}

/**
 * Prints the results of BFS.
 */
void print_bfs_result(graph *g)
{
    printf("%-5s | %-5s | %-5s\n", "Node", "Prev", "Dist");
    for (int i = 0; i < g->node_count; i++)
    {
        node *n = &g->nodes[i];
        printf("%5d | %5d | %5d\n", n->node_number, n->prevNodeId, n->dist);
    }
}

/**
 * Original solution with recursion.
 * Not used because of potential stackoverflow with larger files. 
 */
void DFS_rec(graph *g, stack *s, int node_id)
{
    node *top_node = &g->nodes[node_id];
    if (!top_node->visited)
    {
        top_node->visited = true;
        for (edge *e = top_node->edges; e; e = e->next)
        {
            int next_id = e->dst_node_id;
            node *inner_node = &g->nodes[next_id];
            if (!inner_node->visited)
            {
                DFS_rec(g, s, next_id);
            }
        }
        stack_push(s, node_id);
    }
}
/**
 * Constructs the stack for the DFS/Topo sort
 * resets visited flags
 * Uses dfs with recursion
 */
stack *topo_DFS_rec(graph *g)
{
    stack *s = new_stack(g->node_count);

    graph_reset_flags(g);

    for (int i = 0; i < g->node_count; i++)
    {
        DFS_rec(g, s, i);
    }
    return s;
}

/**
 * Flags each specified node as visited.
 * Iterates throug each edge of specified node until it finds an
 * unvisited node. This node's id is set as the next id to search
 * and consequently pushed to temp_s. 
 * 
 * If a specified node does not have specified edges, or all
 * it's edges leads to visited nodes, then:
 *  - temp_s gets poped and it's node id is pushed on the res_s. 
 *  - the next id is set to the next element in temp_s.
 */
int DFS_iter(graph *g, stack *res_s, stack *temp_s, int node_id)
{
    int next_id;
    node *top_node = &g->nodes[node_id];
    edge *e = top_node->edges;
    top_node->visited = true;

    for (; e; e = e->next)
    {
        next_id = e->dst_node_id;
        node *inner_node = &g->nodes[next_id];
        if (!inner_node->visited)
        {
            stack_push(temp_s, next_id);
            break;
        }
    }
    if (e == NULL)
    {
        stack_push(res_s, g->nodes[stack_pop(temp_s)].node_number);
        next_id = stack_peek(temp_s);
    }
    return next_id;
}
/**
 * By default each iteration starts with an empty temporary stack (temp_s)
 * If the iterations number is not visited yet, it is pushed on the stack
 * and the DFS search will commence in the while loop. 
 * Else the next iteration begins. 
 */
stack *topo_DFS_iter(graph *g)
{
    graph_reset_flags(g);

    stack *temp_s = new_stack(g->node_count);
    stack *res_s = new_stack(g->node_count);

    for (int i = 0; i < g->node_count; i++)
    {
        int start_id = i;
        if (!g->nodes[start_id].visited)
        {
            stack_push(temp_s, start_id);
        }
        while (temp_s->count > 0)
        {
            start_id = DFS_iter(g, res_s, temp_s, start_id);
        }
    }
    stack_free(temp_s);
    return res_s;
}

/**
 * Find the next non-space token.
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

/**
 * Maps a file to memory.
 */
char *mmap_file(const char *filename, int *length)
{
    int fd;
    if ((fd = open(filename, O_RDONLY)) == -1)
    {
        return MAP_FAILED;
    }
    int len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *data = (char *)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    *length = len;
    return data;
}

/**
 * Parses the graphfile into a graph.
 */
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

    graph *g = malloc(sizeof(graph));
    g->node_count = node_count;
    g->nodes = calloc(node_count, sizeof(node));

    for (int i = 0; i < node_count; i++)
    {
        g->nodes[i].node_number = i;
    }

    while (i < length)
    {
        int node_id = atoi(&data[i]);

        if (find_next_token(data, length, &i))
            continue;
        int edge_dst_id = atoi(&data[i]);
        if (node_id < node_count && edge_dst_id < node_count)
        {
            node *n = &g->nodes[node_id];
            n->node_number = node_id;
            edge_add(n, edge_dst_id);
        }

        if (find_next_token(data, length, &i))
            continue;
    }

    munmap(data, length);
    return g;
}

int main(int argc, const char *argv[])
{
    const int src_node = argc > 1 ? atoi(argv[1]) : -1;
    const char *graphfile = argc > 2 ? argv[2] : NULL;

    if (src_node == -1 || graphfile == NULL)
    {
        printf(
            "You must provide a graph file and optionally a name file.\n"
            "Usage: ./graph <src-node> <graphfile>\n");
        return 1;
    }

    graph *graph = parse_graphfile(graphfile);
    if (graph == NULL)
    {
        perror("Failed to parse graph file");
        return 1;
    }

    stack *s = topo_DFS_iter(graph);
    print_stack(s);
    stack_free(s);

    bfsv2(graph, src_node);
    print_bfs_result(graph);

    graph_free(graph);

    return 0;
}
