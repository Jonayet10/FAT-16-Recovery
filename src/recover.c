#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "directory_tree.h"
#include "fat16.h"

const size_t MASTER_BOOT_RECORD_SIZE = 0x20B;

void follow(FILE *disk, directory_node_t *node, bios_parameter_block_t bpb) {
    // start at current position of disk
    directory_entry_t entry;

    // loop to process directory_entry_t’s from disk
    while (true) {
        // fread directory_entry_t from disk
        (void) fread(&entry, sizeof(directory_entry_t), 1, disk);
        // loop until encounter entry with a name that starts with \0
        if (entry.filename[0] == '\0') {
            break;
        }
        // skip hidden entries
        if (is_hidden(entry)) {
            continue;
        }
        // retrieve name of directory_entry_t
        char *name = get_file_name(entry);

        if (is_directory(entry)) {
            directory_node_t *dnode = init_directory_node(name);
            // attach new directory node to parent node
            add_child_directory_tree(node, (node_t *) dnode);
            // get offset to next directory_entry_t's
            size_t offset = get_offset_from_cluster(entry.first_cluster, bpb);
            // save current position in disk
            long current_pos = ftell(disk);
            // seek to next directory_entry_t's
            fseek(disk, offset, SEEK_SET);
            // recursively follow next directories
            follow(disk, dnode, bpb);
            // reset position in disk to next directory_entry_t
            fseek(disk, current_pos, SEEK_SET);
        }
        else {
            size_t offset = get_offset_from_cluster(entry.first_cluster, bpb);
            // allocate memory for file contents
            uint8_t *contents = malloc(entry.file_size);
            assert(contents != NULL);
            // save curreny position in disk
            long current_pos = ftell(disk);
            // seek to file data
            fseek(disk, offset, SEEK_SET);
            // read file contents
            // 1 = sizeof(unit8_t)
            (void) fread(contents, 1, entry.file_size, disk);
            // create file node
            file_node_t *fnode = init_file_node(name, entry.file_size, contents);
            // attach new file node to parent node
            add_child_directory_tree(node, (node_t *) fnode);
            // reset position in disk to next directory_entry_t
            fseek(disk, current_pos, SEEK_SET);
            // resetting position happens so a directory_entry_t is not skipped
            // when fread is called at beginning of while(true) loop
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <image filename>\n", argv[0]);
        return 1;
    }

    FILE *disk = fopen(argv[1], "r");
    if (disk == NULL) {
        fprintf(stderr, "No such image file: %s\n", argv[1]);
        return 1;
    }

    bios_parameter_block_t bpb;

    /* TODO: Write your code here. */

    // skip past the master boot record
    fseek(disk, MASTER_BOOT_RECORD_SIZE, SEEK_SET);
    // fread the bios parameter block
    (void) fread(&bpb, sizeof(bios_parameter_block_t), 1, disk);
    // skip past the padding and the file allocation tables directly to the beginning of
    // the root directory entries block
    fseek(disk, get_root_directory_location(bpb), SEEK_SET);

    directory_node_t *root = init_directory_node(NULL);
    follow(disk, root, bpb);
    print_directory_tree((node_t *) root);
    create_directory_tree((node_t *) root);
    free_directory_tree((node_t *) root);

    int result = fclose(disk);
    assert(result == 0);
}
