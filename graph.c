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

    // BFS
    bool visited;
    int prevNodeId;
    int dist;
} node;

typedef struct graph
{
    node *nodes; 
    int node_count;
} graph;

/**
 * Implements queue as linkedlist of queue_nodes
 */
typedef struct queue_node
{
    int data;
    struct queue_node * next;
} queue_node;

typedef struct queue
{
    struct queue_node *head;
} queue;

typedef struct stack
{
    int *elems;
    size_t capacity;
    int count;
} stack;

edge *new_edge(int dst_node_id, edge* next)
{
    edge* e = malloc(sizeof(edge));
    e->dst_node_id = dst_node_id;
    e->next = next;
    return e;
}

void graph_reset_flags(graph *g)
{
    for(int i = 0; i < g->node_count; i++)
    {
        g->nodes[i].visited = false;
        g->nodes[i].prevNodeId = -1;
        g->nodes[i].dist = 0;
    }
}

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
 * Adds an edge to the front of the linked list.
 * Adds to front insteda of back because its more 
 * efficient and the list order is irelevant. 
 */
void edge_add(node *n, int dst_node_id)
{
    n->edges = new_edge(dst_node_id, n->edges);
}

queue *new_queue()
{
    queue *q = calloc(1, sizeof(queue));
    q->head = NULL;
    return q;
}

queue_node *new_queue_node(int data, queue_node *next)
{
    queue_node *qn = calloc(1, sizeof(queue_node));
    qn->data = data;
    qn->next = next;
    return qn;
}

void queue_push(queue *q, int node_id)
{
    queue_node *qn = new_queue_node(node_id, NULL);

    if(q->head)
    {
        queue_node *head = q->head;
        while (head->next)
        {
            head = head->next;
        } 
        head->next = qn;
    }
    else
    {
        q->head = qn;
    }
}

/**
 * Returns the first value in the queue without changing the queue.
 * 
 */
// int queue_peek(queue *q)
// {
//     if (q->count == 0)
//     {
//         return 0;
//     }
//     return q->head->data;
// }

int queue_peek(queue *q, bool *found)
{
    if (q->head == NULL)
    {
        if (found != NULL) *found = false;
        return -1;
    }
    if (found != NULL) *found = true;
    return q->head->data;
}

/**
 * Removes and returns first value in the queue.
 * Shifts the head to point to next queue_node.  
 */
// int queue_pop(queue *q)
// {
//     if (q->count == 0) return 0;

//     int result = queue_peek(q);
//     queue_node *old_node = q->head;
//     q->head = q->head->next;
//     q->count--;
//     free(old_node); //free the node that got popped. 
//     //printf("%d", result);
//     return result;
// }

int queue_pop(queue *q, bool *found)
{
    bool f = false;
    int result = queue_peek(q, &f);
    if(result > -1) 
    {
        queue_node *old_node = q->head;
        q->head = q->head->next;
        free(old_node);
        if (found != NULL) *found = f;
    }
    return result;
}

void queue_free(queue *q)
{
    queue_node *head = q->head;
    while (head)
        {
            queue_node *old_node = head;
            head = head->next;
            free(old_node);
        }
    free(q);
}

stack *new_stack(int capacity)
{
    stack *s = malloc(sizeof(stack));
    s->capacity = capacity;
    s->count = 0;
    s->elems = malloc(sizeof(int) * capacity);
    return s;
}

void stack_push(stack *s, int elem)
{
    s->elems[s->count] = elem;
    s->count++;
}

int stack_peek(stack *s)
{
    if (s->count == 0)
    {
        return 0;
    }
    return s->elems[s->count - 1];
}

int stack_pop(stack *s)
{
    int result = stack_peek(s);
    s->count--;
    return result;
}

void stack_free(stack *s)
{
    free(s->elems);
    free(s);
}

void print_stack(stack *s)
{
    printf("Topological order: ");
    while (s->count > 0)
    {
        printf("%d ", stack_pop(s));
    }
    printf("\n");
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
void set_text_value(node *n, void *text)
{
    n->text=(char*) text;
}

/**
 * What to do: 
 *  Reset graphs flags.
 *  Start in sorcenode, loop throug this node's edges and
 * flag each in turn.
 *  After all edges are flagged, go to the node connected 
 * to first the edge. 
 *  Stop when each node in a layer have edges only pointing
 * to already visited nodes. 
 * 
 * Use queue as holder of nodes to use. 
 * It represents the order the nodes are visited. 
 * 
 */
void bfsv2(graph *g, int srcNodeId)
{
    graph_reset_flags(g);
    
    queue *q = new_queue();
    
    // push initial node onto queue for searching
    node *src = &g->nodes[srcNodeId];
    src->visited = true;
    src->prevNodeId = -1;

    queue_push(q, srcNodeId);

    bool found = true;
    
    // look over nodes
    while (found)
    {
        int n_id = (int)queue_pop(q, &found);
        if(n_id == -1) break;
        //printf(":%d\n",n_id);
        node *n = &g->nodes[n_id];
        n->visited = true;

        // look over edges and push to queue
        edge *e = n->edges;
        //printf(":::%d\n", e->dst_node_id);
        while (e != NULL) //Stops when all edges of a node are visited.
        {
            int targetNodeId = e->dst_node_id;
            //printf(":::%d\n",targetNodeId);
            node *target = &g->nodes[targetNodeId];
            e = e->next;
            
            // push for deeper search if not already visited
            if (target->visited) continue;

            target->prevNodeId = n_id;
            target->dist = n->dist + 1;
            queue_push(q, targetNodeId);
            target->visited = true;
            //printf(":%d -> %d\n", target->prevNodeId, targetNodeId);
        }
    }
    queue_free(q);
}

void print_distance_prev(graph *g)
{
    printf("%-5s | %-5s | %-5s\n", "Node", "Prev", "Dist");
    for(int i=0; i < g->node_count; i++)
    {
        node *n = &g->nodes[i];
        printf("%5d | %5d | %5d\n", n->node_number, n->prevNodeId, n->dist);
    }
}

// void dfs_rec(graph *g, stack *s, int node_id)
// {
//     node *top_node = &g->nodes[node_id];
//     if (!top_node->visited)
//     {
//         //printf("%d\n", count++);
//         top_node->visited = true;
//         for (edge *e=top_node->edges; e; e=e->next)
//         {
//             int next_id = e->dst_node_id; //3
//             node *inner_node = &g->nodes[next_id];
//             if (!inner_node->visited)
//             {
//                 //printf("Calling rec - node: %d\n", next_id);
//                 dfs_rec(g, s, next_id);
//             }
//         }
//         //printf("%d\n", node_id);
//         stack_push(s, node_id);
//     }
// }


int some_func(graph *g, stack *res_s, stack *temp_s, int node_id)
{
    //printf("::%d\n", temp_s->count);
    node *top_node = &g->nodes[node_id];
    // for (edge *e=top_node->edges; e; e=e->next)
    // {
    //     printf("%d\n", e->dst_node_id);
    // }
    if (!top_node->visited)
    {
        top_node->visited = true;
        for (edge *e=top_node->edges; e; e=e->next)
        {
            //printf("%d\n", e->dst_node_id);
            int next_id = e->dst_node_id;
            node_id = next_id;
            //printf("::%d\n", next_id);
            node *inner_node = &g->nodes[next_id];
            if (!inner_node->visited)
            {
                //printf("Calling temp_s PUSH - node: %d\n", next_id);
                stack_push(temp_s, next_id);
                
                //node_id = stack_peek(temp_s);
                break;
            }
            
            //printf("::G\n");
        }
        
    }
    printf(":%d\n", temp_s->count);
    for (edge *e=top_node->edges; e; e=e->next)
    {
        if(e->next == NULL)
        {
            //printf(":::%d\n", node_id);
            node *node = &g->nodes[stack_pop(temp_s)];
            node_id = node->node_number;
            //printf("::2:%d\n", node_id);
            stack_push(res_s, node->node_number);
        }
    }
    return node_id;
}
 
stack *dfs_topo(graph *g, int node_id)
{
    graph_reset_flags(g);

    stack *temp_s = new_stack(g->node_count);
    stack *res_s = new_stack(g->node_count);

    for(int i = 0; i < g->node_count; i++)
    {
        //printf(":%d\n", node_id);
        int id = i;
        do
        {
            id = some_func(g, res_s, temp_s, id);
            
        } while (temp_s->count > 0);
        
    }
    stack_free(temp_s);
    return res_s;
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

int main(int argc, const char *argv[])
{
    const int src_node = argc > 1 ? atoi(argv[1]) : -1;
    const char *graphfile = argc > 2 ? argv[2] : NULL;
    const char *namefile = argc > 3 ? argv[3] : NULL;

    if (src_node == -1 || graphfile == NULL)
    {
        printf(
            "You must provide a graph file and optionally a name file.\n"
            "Usage: ./graph <src-node> <dst-node> <graphfile> [namefile]\n"
        );
        return 1;
    }
    graph* graph = parse_graph(graphfile, namefile);
    
    stack *s = dfs_topo(graph, src_node);
    print_stack(s);
    stack_free(s);

    bfsv2(graph, src_node);
    print_distance_prev(graph);

    graph_free(graph);
    
    return 0;
}
