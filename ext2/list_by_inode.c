#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <zconf.h>
//#include <linux/ext2_fs.h> // most likely and on ubuntu
#include "/usr/include/ext2fs/ext2_fs.h" // on arch

#define BASE_OFFSET 1024                   /* locates beginning of the super block (first group) */
#define BLOCK_OFFSET(i_block) (BASE_OFFSET+(i_block-1)*i_block_size)

uint32_t i_block_size;
struct ext2_params ext2_instance;
struct ext2_super_block s_super;
struct ext2_group_desc s_first_group;
struct ext2_inode s_root_dir_inode;

uint32_t fd;

struct ext2_params {
    struct ext2_group_desc s_first_group;
    struct ext2_inode s_root_dir_inode;
};


void read_direct(char *block_data, uint32_t num) {
    lseek(fd, i_block_size * num, SEEK_SET);
    uint32_t n_read = read(fd, block_data, i_block_size);
    block_data[n_read] = 0;
}

void read_indirect(uint32_t num) { // printing here
    lseek(fd, i_block_size * num, SEEK_SET);
    uint32_t direct_blocks[i_block_size / 4];
    uint32_t n_read = read(fd, &direct_blocks, i_block_size);
    char block_data[1024];
    for (uint32_t i = 0; direct_blocks[i] && i < n_read; i++) {
        read_direct(block_data, direct_blocks[i]);
        printf("%s", block_data);
    }
}

void read_double_indirect(uint32_t num) {
    lseek(fd, i_block_size * num, SEEK_SET);
    uint32_t indirect_blocks[i_block_size / 4];
    uint32_t n_read = read(fd, &indirect_blocks, i_block_size);
    for (uint32_t i = 0; indirect_blocks[i] && i < n_read; i++) {
        read_indirect(indirect_blocks[i]);
    }
}

void read_triple_indirect(uint32_t num) {
    lseek(fd, i_block_size * num, SEEK_SET);
    uint32_t double_indirect_blocks[i_block_size / 4];
    uint32_t n_read = read(fd, &double_indirect_blocks, i_block_size);
    for (uint32_t i = 0; double_indirect_blocks[i] && i < n_read; i++) {
        read_double_indirect(double_indirect_blocks[i]);
    }
}


uint32_t read_super(struct ext2_super_block *ps_super) {
    lseek(fd, BASE_OFFSET, SEEK_SET);
    read(fd, ps_super, sizeof(*ps_super));

    if (ps_super->s_magic != EXT2_SUPER_MAGIC) {
        fprintf(stderr, "Not a Ext2 filesystem\n");
        exit(1);
    }
    return 1024 << ps_super->s_log_block_size;
}

void read_group(struct ext2_group_desc *group, uint32_t group_num) {
    uint32_t start;
    if (i_block_size == 1024) {
        start = BASE_OFFSET + i_block_size;
    } else {
        start = BASE_OFFSET;
    }

    lseek(fd, start + s_super.s_blocks_per_group * i_block_size * group_num, SEEK_SET);
    read(fd, group, sizeof(struct ext2_group_desc));

}

void read_inode(uint32_t i_inode_no, struct ext2_group_desc *group, struct ext2_inode *ps_inode) {
    lseek(fd, BLOCK_OFFSET(group->bg_inode_table) + (i_inode_no - 1) * sizeof(struct ext2_inode),
          SEEK_SET);
    read(fd, ps_inode, sizeof(struct ext2_inode));

} /* read_inode() */

/* // Getting inode by inode number for O(1) (not working)
void read_inode_by_num(uint32_t i_inode_num, struct ext2_inode *ps_inode){
    uint32_t block_group_num = (i_inode_num - 1) / s_super.s_inodes_per_group;
    struct ext2_group_desc group;
    read_group(fd, &group, block_group_num);
    uint32_t index = (i_inode_num - 1) % s_super.s_inodes_per_group;
    uint32_t containing_block = (index * s_super.s_inode_size) / i_block_size;
    char block_data[i_block_size];
    uint32_t off = BLOCK_OFFSET(group.bg_inode_table);

    lseek(fd, BLOCK_OFFSET(group.bg_inode_table) + (i_inode_no - 1) * sizeof(struct ext2_inode),
          SEEK_SET);

    read_direct(block_data, fd, containing_block);

    read_inode(fd, i_inode_num, &group, ps_inode);
}
*/

void print_file_contents(struct ext2_inode *inode, struct ext2_group_desc *group) {
    void *block;
    if (!S_ISDIR(inode->i_mode)) {
        struct ext2_dir_entry_2 *entry;
        uint32_t size = 0;

        if ((block = malloc(i_block_size)) == NULL) { /* allocate memory for the data block */
            fprintf(stderr, "Memory error\n");
            close(fd);
            exit(1);
        }
        lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
        read(fd, block, i_block_size);                /* read block from disk*/
        entry = (struct ext2_dir_entry_2 *) block;  /* first entry in the directory */
        char block_data[i_block_size + 1];
        while ((size < inode->i_size) && entry->inode) {
            for (uint32_t i = 0; inode->i_block[i]; i++) {
                if (i < 12) {
                    read_direct(block_data, inode->i_block[i]);
                    printf("%s", block_data);
                } else if (i == 12) {
                    read_indirect(inode->i_block[i]);
                } else if (i == 13) {
                    read_double_indirect(inode->i_block[i]);
                } else if (i == 14) {
                    read_triple_indirect(inode->i_block[i]);
                }
            }

            entry = (void *) entry + entry->rec_len;
            size += entry->rec_len;
        }

        free(block);
    }

}

void print_dir_entries(struct ext2_inode *inode, struct ext2_group_desc *group) {
    void *block;
    if (S_ISDIR(inode->i_mode)) {
        struct ext2_dir_entry_2 *entry;
        uint32_t size = 0;

        if ((block = malloc(i_block_size)) == NULL) { /* allocate memory for the data block */
            fprintf(stderr, "Memory error\n");
            close(fd);
            exit(1);
        }
        lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
        read(fd, block, i_block_size);                /* read block from disk*/
        entry = (struct ext2_dir_entry_2 *) block;  /* first entry in the directory */
        /* Notice that the list may be terminated with a NULL
           entry (entry->inode == NULL)*/
        while ((size < inode->i_size) && entry->inode) {
            char file_name[EXT2_NAME_LEN + 1];
            memcpy(file_name, entry->name, entry->name_len);
            file_name[entry->name_len] = 0;
            read_inode(entry->inode, group, inode);
            if (strcmp(entry->name, ".") && strcmp(entry->name, "..")) {
                printf("%10u %s\n", entry->inode, file_name);
            }
            entry = (void *) entry + entry->rec_len;
            size += entry->rec_len;
        }

        free(block);
    }
}

int compare_by_name(char *path, char *name, uint32_t inode, char *x_name, uint32_t x_inode) {
    return !strcmp(path, x_name);
}

int compare_by_inode(char *path, char *name, uint32_t inode, char *x_name, uint32_t x_inode) {
    return inode == x_inode;
}


void run_over_dir_tree(struct ext2_inode inode, struct ext2_group_desc *group, char *x_name, uint32_t x_inode,
                       void (*print_f)(struct ext2_inode *, struct ext2_group_desc *),
                       int (*compare_f)(char *, char *, uint32_t, char *, uint32_t),
                       char *_path) {
    void *block;
    if (S_ISDIR(inode.i_mode)) {
        struct ext2_dir_entry_2 *entry;
        uint32_t size = 0;

        if ((block = malloc(i_block_size)) == NULL) { /* allocate memory for the data block */
            fprintf(stderr, "Memory error\n");
            close(fd);
            exit(1);
        }
        lseek(fd, BLOCK_OFFSET(inode.i_block[0]), SEEK_SET);
        read(fd, block, i_block_size);                /* read block from disk*/
        entry = (struct ext2_dir_entry_2 *) block;  /* first entry in the directory */
        char path[1024];

        while ((size < inode.i_size) && entry->inode) {
            read_inode(entry->inode, group, &inode);
            char name[entry->name_len + 1];
            memcpy(name, entry->name, entry->name_len);
            name[entry->name_len] = 0;

            strcpy(path, _path);
            if (strlen(_path)) {
                uint len = strlen(_path);
                path[len] = '/';
                path[len + 1] = 0;
            } else {
                path[0] = 0;
            }
            strcat(path, name);


            if (compare_f(path, name, entry->inode, x_name, x_inode)) {
                print_f(&inode, group);
                free(block);
                return;
            }

            if (entry->file_type == 0x2 && strcmp(name, ".") && strcmp(name, "..")) {
                run_over_dir_tree(inode, group, x_name, x_inode, print_f, compare_f, path);
            }

            entry = (void *) entry + entry->rec_len;
            size += entry->rec_len;
        }

        free(block);
    }
} /* read_dir() */


void print_dir_entries_by_inode(uint32_t inode) {
    char path[1];
    path[0] = 0;
    run_over_dir_tree(ext2_instance.s_root_dir_inode,
                      &ext2_instance.s_first_group, NULL, inode, print_dir_entries, compare_by_inode, path);
}

void print_file_contents_by_inode(uint32_t inode) {
    char path[1];
    path[0] = 0;

    run_over_dir_tree(ext2_instance.s_root_dir_inode,
                      &ext2_instance.s_first_group, NULL, inode, print_file_contents, compare_by_inode, path);
}

void print_dir_entries_by_name(char *name) {
    char path[1];
    path[0] = 0;

    run_over_dir_tree(ext2_instance.s_root_dir_inode,
                      &ext2_instance.s_first_group, name, 2, print_dir_entries, compare_by_name, path);
}

void print_file_contents_by_name(char *name) {
    char path[1];
    path[0] = 0;

    run_over_dir_tree(ext2_instance.s_root_dir_inode,
                      &ext2_instance.s_first_group, name, 2, print_file_contents, compare_by_name, path);
}


void init_ext2(char *path_to_image) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
        exit(1);
    }

    if ((fd = open(path_to_image, O_RDONLY)) < 0) {
        // perror(FS_IMAGE);
        exit(1);
    }

    struct ext2_group_desc s_first_group;
    struct ext2_inode s_root_dir_inode;

    i_block_size = read_super(&s_super);

    read_group(&s_first_group, 0);
    read_inode(2, &s_first_group, &s_root_dir_inode);   /* read inode 2 (root directory) */

    ext2_instance.s_first_group = s_first_group;
    ext2_instance.s_root_dir_inode = s_root_dir_inode;

}

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Please specify: _path to filesystem image_ task (-entries/-contents) way (-inode/-name) path (inode/path to file with no '/' at the beginning)");
        exit(1);
    }
    if (strcmp(argv[2], "-entries") && strcmp(argv[2], "-contents")) {
        printf("Call -entries for dir entries. -contents for file contents");
        exit(1);
    }
    if (strcmp(argv[3], "-inode") && strcmp(argv[3], "-name")) {
        printf("Call -inode to address by inode. -name to address by filename");
        exit(1);
    }
    init_ext2(argv[1]);

    if (!strcmp(argv[2], "-entries")) {
        if (!strcmp(argv[3], "-inode")) {

            print_dir_entries_by_inode(atoi(argv[4]));
        } else {
            print_dir_entries_by_name(argv[4]);
        }
    } else {
        if (!strcmp(argv[3], "-inode")) {
            print_file_contents_by_inode(atoi(argv[4]));
        } else {
            print_file_contents_by_name(argv[4]);
        }
    }

    printf("\n--------------\nend\n");
    close(fd);
}