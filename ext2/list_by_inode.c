#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <zconf.h>
//#include <linux/ext2_fs.h> // most likely
#include "/usr/include/ext2fs/ext2_fs.h" // on arch

#define BASE_OFFSET 1024                   /* locates beginning of the super block (first group) */
#define FS_IMAGE "testfs2.img"               /* the file system image */
#define BLOCK_OFFSET(i_block) (BASE_OFFSET+(i_block-1)*i_block_size)
int i_block_size;

struct ext2_params {
    int fd;
    struct ext2_super_block s_super;
    struct ext2_group_desc s_first_group;
    struct ext2_inode s_root_dir_inode;
    int i_block_size;
};

int read_super(struct ext2_super_block *ps_super, int *fd) {
    lseek(*fd, BASE_OFFSET, SEEK_SET);
    read(*fd, ps_super, sizeof(*ps_super));

    if (ps_super->s_magic != EXT2_SUPER_MAGIC) {
        fprintf(stderr, "Not a Ext2 filesystem\n");
        exit(1);
    }
    return 1024 << ps_super->s_log_block_size;
}

void read_inode(int fd, int i_inode_no, const struct ext2_group_desc *s_group, struct ext2_inode *ps_inode) {
    lseek(fd, BLOCK_OFFSET(s_group->bg_inode_table) + (i_inode_no - 1) * sizeof(struct ext2_inode),
          SEEK_SET);
    read(fd, ps_inode, sizeof(struct ext2_inode));
} /* read_inode() */

void print_file_contents(int fd, struct ext2_inode *inode, struct ext2_group_desc *group) {
    void *block;
    if (!S_ISDIR(inode->i_mode)) {
        struct ext2_dir_entry_2 *entry;
        unsigned int size = 0;

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
        int i = 0;
        int _fd = dup(fd);
        while ((size < inode->i_size) && entry->inode) {
            char block_data[i_block_size + 1];
            if (i < 12) {
                lseek(_fd, i_block_size * inode->i_block[i], SEEK_SET);
                read(_fd, block_data, sizeof(struct ext2_inode));
                block_data[i_block_size] = 0;     /* append null character to the file name */
                printf("%s", block_data);
            }
            entry = (void *) entry + entry->rec_len;
            size += entry->rec_len;
            ++i;
        }

        free(block);
    }

}

void print_dir_entries(int fd, struct ext2_inode *inode, struct ext2_group_desc *group) {
    void *block;
    if (S_ISDIR(inode->i_mode)) {
        struct ext2_dir_entry_2 *entry;
        unsigned int size = 0;

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
            file_name[entry->name_len] = 0;     /* append null character to the file name */
            read_inode(fd, entry->inode, group, inode);
            if (strcmp(entry->name, ".") && strcmp(entry->name, "..")) {
                printf("%10u %s\n", entry->inode, file_name);
            }
            entry = (void *) entry + entry->rec_len;
            size += entry->rec_len;
        }

        free(block);
    }
}

void run_over_dir_tree(int fd, struct ext2_inode *inode, struct ext2_group_desc *group, int x_inode,
                       void (*print_f)(int, struct ext2_inode *, struct ext2_group_desc *)) {
    void *block;
    if (S_ISDIR(inode->i_mode)) {
        struct ext2_dir_entry_2 *entry;
        unsigned int size = 0;
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
            read_inode(fd, entry->inode, group, inode);
            if (entry->inode == x_inode) {
                print_f(fd, inode, group);
                return;
            }
            if (entry->file_type == 0x2 && strcmp(entry->name, ".") && strcmp(entry->name, "..")) {
                run_over_dir_tree(fd, inode, group, x_inode, print_f);
            }
            entry = (void *) entry + entry->rec_len;
            size += entry->rec_len;
        }

        free(block);
    }
} /* read_dir() */

struct ext2_params init_ext2() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
        exit(1);
    }

    int fd;
    if ((fd = open(FS_IMAGE, O_RDONLY)) < 0) {
        perror(FS_IMAGE);
        exit(1);  /* error while opening the floppy device */
    }

    struct ext2_super_block s_super;
    struct ext2_group_desc s_first_group;
    struct ext2_inode s_root_dir_inode;
    i_block_size = read_super(&s_super, &fd);

    lseek(fd, BASE_OFFSET + i_block_size, SEEK_SET);
    read(fd, &s_first_group, sizeof(s_first_group));

    read_inode(fd, 2, &s_first_group, &s_root_dir_inode);   /* read inode 2 (root directory) */

    struct ext2_params ext2_inited;
    ext2_inited.s_super = s_super;
    ext2_inited.s_first_group = s_first_group;
    ext2_inited.s_root_dir_inode = s_root_dir_inode;
    ext2_inited.i_block_size = i_block_size;
    ext2_inited.fd = fd;

    return ext2_inited;
}


void print_dir_entries_by_inode(int inode) {
    struct ext2_params ext2_instance = init_ext2();
    run_over_dir_tree(ext2_instance.fd, &ext2_instance.s_root_dir_inode, &ext2_instance.s_first_group, inode,
                      print_dir_entries);
    close(ext2_instance.fd);
}

void print_file_contents_by_inode(int inode) {
    struct ext2_params ext2_instance = init_ext2();
    run_over_dir_tree(ext2_instance.fd, &ext2_instance.s_root_dir_inode, &ext2_instance.s_first_group, inode,
                      print_file_contents);
    close(ext2_instance.fd);
}

void print_dir_entries_by_name(char* name) {
    struct ext2_params ext2_instance = init_ext2();
    run_over_dir_tree(ext2_instance.fd, &ext2_instance.s_root_dir_inode, &ext2_instance.s_first_group, inode,
                      print_dir_entries);
    close(ext2_instance.fd);
}

void print_file_contents_by_name(char* name) {
    struct ext2_params ext2_instance = init_ext2();
    run_over_dir_tree(ext2_instance.fd, &ext2_instance.s_root_dir_inode, &ext2_instance.s_first_group, inode,
                      print_file_contents);
    close(ext2_instance.fd);
}

int main(int argc, char **argv) {
    print_dir_entries_by_inode(23);
    print_file_contents_by_inode(35);
    printf("end\n");
}