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

void parse_graphfile(const char *graphfile)
{
    int fd;
    if ((fd = open(graphfile, O_RDONLY)) == -1)
    {
        perror("Unable to open file graph file.");
    }
    int length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *data = (char*)mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0); //NULL means the kernel picks starting addr
    close(fd);
    if (data == MAP_FAILED)
    {
        perror("Unable to map graph file into memory");
    }
    int i = 0;

    // read header line
    int node_count = atoi(&data[i]);
    find_next_token(data, length, &i);
    int edge_count = atoi(&data[i]);
    find_next_token(data, length, &i);

    char **node_data = calloc(node_count, sizeof(char*));
    int *nodes = calloc(node_count, sizeof(int));
    int *edges = calloc(edge_count, sizeof(int));

    for (int l = 0; i < length; l++)
    {
        if (l < node_count)
        {
            nodes[l] = atoi(&data[i]);
        }
        
        if (find_next_token(data, length, &i)) continue;
        if (l < edge_count)
        {
            edges[l] = atoi(&data[i]);
        }

        if (find_next_token(data, length, &i)) continue;
        if (l < node_count)
        {
            char *s = parse_string(data, length, &i);
            node_data[l] = parse_string(data, length, &i);
        }
        find_next_token(data, length, &i);
    }

    munmap(data, length);
}

void run_with_files(const char *graphfile, const char *namefile)
{
    parse_graphfile(graphfile);
    if (namefile != NULL)
    {
        parse_namefile(namefile);
    }
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
    run_with_files(graphfile, namefile);
    return 0;
}