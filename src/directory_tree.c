#include "directory_tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void helper(node_t *node, size_t lvl);
void create_tree_helper(node_t *node, char *path);
const int DIRECTORY_PERMISSIONS = 0777;

void init_node(node_t *node, char *name, node_type_t type) {
    if (name == NULL) {
        name = strdup("ROOT");
        assert(name != NULL);
    }
    node->name = name;
    node->type = type;
}

file_node_t *init_file_node(char *name, size_t size, uint8_t *contents) {
    file_node_t *node = malloc(sizeof(file_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, FILE_TYPE);
    node->size = size;
    node->contents = contents;
    return node;
}

directory_node_t *init_directory_node(char *name) {
    directory_node_t *node = malloc(sizeof(directory_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, DIRECTORY_TYPE);
    node->num_children = 0;
    node->children = NULL;
    return node;
}

void add_child_directory_tree(directory_node_t *dnode, node_t *child) {
    // reallocate memory to accommodate the new child
    dnode->children =
        realloc(dnode->children, (dnode->num_children + 1) * sizeof(node_t *));
    assert(dnode->children != NULL);

    // find the insertion index to keep the array in alphabetical order
    size_t index = 0;
    while (index < dnode->num_children &&
           strcmp(dnode->children[index]->name, child->name) < 0) {
        index++;
    }

    // shift existing nodes to the right to make space for the new node
    for (size_t i = dnode->num_children; i > index; i--) {
        dnode->children[i] = dnode->children[i - 1];
    }

    dnode->children[index] = child;

    dnode->num_children++;
}

void print_directory_tree(node_t *node) {
    size_t lvl = 0;
    helper(node, lvl);
}

void helper(node_t *node, size_t lvl) {
    for (size_t i = 0; i < 4 * lvl; i++) {
        putchar(' ');
    }

    if (lvl == 0) {
        printf("ROOT\n");
    }
    else {
        printf("%s\n", node->name);
    }
    if (node->type == DIRECTORY_TYPE) {
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            helper(dnode->children[i], lvl + 1);
        }
    }
}

void create_directory_tree(node_t *node) {
    char *path = malloc(sizeof(char) * 2);
    path[0] = '.';
    path[1] = '\0';
    create_tree_helper(node, path);
    free(path);
}

void create_tree_helper(node_t *node, char *path) {
    // add onto path
    char *next_path = malloc(sizeof(char) * (strlen(node->name) + strlen(path) + 1 + 1));
    // last two +1's are for \0 and /
    next_path[0] = '\0';
    strcat(next_path, path);
    strcat(next_path, "/");
    strcat(next_path, node->name);

    if (node->type == FILE_TYPE) {
        file_node_t *fnode = (file_node_t *) node;
        FILE *file = fopen(next_path, "w");
        size_t written = fwrite(fnode->contents, sizeof(uint8_t), fnode->size, file);
        assert(written == fnode->size);
        (void) fclose(file);
    }
    else {
        assert(node->type == DIRECTORY_TYPE);
        int mkdir_permissions = DIRECTORY_PERMISSIONS;
        mkdir(next_path, mkdir_permissions);
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            create_tree_helper(dnode->children[i], next_path);
        }
    }
    free(next_path);
}

void free_directory_tree(node_t *node) {
    if (node->type == FILE_TYPE) {
        file_node_t *fnode = (file_node_t *) node;
        free(fnode->contents);
    }
    else {
        assert(node->type == DIRECTORY_TYPE);
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            free_directory_tree(dnode->children[i]);
        }
        free(dnode->children);
    }
    free(node->name);
    free(node);
}
