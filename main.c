#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <math.h>

#include "read.h"

#define B_MIN 0
#define B_MAX 1023
#define KiB_MIN 1024
#define KiB_MAX ((long long int) (pow(2, 20) - 1))
#define MiB_MIN ((long long int) (pow(2, 20)))
#define MiB_MAX ((long long int) (pow(2, 30) - 1))
#define GiB_MIN ((long long int) (pow(2, 30)))
#define GiB_MAX ((long long int) (pow(2, 40) - 1))
#define TiB_MIN ((long long int) (pow(2, 40)))
#define TiB_MAX ((long long int) (pow(2, 50) - 1))
#define PiB_MIN ((long long int) (pow(2, 50)))
#define PiB_MAX ((long long int) (pow(2, 60) - 1))

void check(int num, int *flag)
{
    if (num != 1) {
        *flag = 1;
    }
}

int set_options(int *_a, int *_s, long int *tree_size, int *_p, int arg, char **array)
{
    int error_flag = 0;
    int skip = 0;
    for (int i = 0; i < (arg - 1); ++i) {
        if (skip) {
            skip = 0;
            continue;
        }
        if (!strcmp(array[i + 1], "-a")) {
            *_a += 1;
            check(*_a, &error_flag);
            continue;
        } else if (!strcmp(array[i + 1], "-s")) {
            *_s += 1;
            check(*_s, &error_flag);
            continue;
        } else if (!strcmp(array[i + 1], "-d")) {            

            char *ptr;
            char *number = array[i + 2];
            *tree_size = strtol(number, &ptr, 10);
            if ((*tree_size < 0) || (*ptr) || (i + 2 == arg)) {
                error_flag = 1;
            }
            skip = 1;
            continue;
        } else if (!strcmp(array[i + 1], "-p")) {
            *_p += 1;
             check(*_p, &error_flag);
             continue;
        }
    }

    return error_flag;
}


void get_unit(struct file_entity *file, int size_op, double *ret, char *unit)
{
    long long int size;
    if (size_op) {
        size = file->byte_size;
    } else {
        size = file->block_size;
    }

    if ((size >= B_MIN) && (size <= B_MAX)) {
        *ret = (double) size;
        memcpy(unit, "B", 1);
    } else if ((size >= KiB_MIN) && (size <= KiB_MAX)) {
        *ret = (double) size/1024;
        memcpy(unit, "KiB", 3);

    } else if ((size >= MiB_MIN) && (size <= MiB_MAX)) {
        *ret = (double) size/pow(2,20);
        memcpy(unit, "MiB", 3);

    } else if ((size >= GiB_MIN) && (size <= GiB_MAX)) {
        *ret = (double) size/pow(2,30);
        memcpy(unit, "GiB", 3);

    } else if ((size >= TiB_MIN) && (size <= TiB_MAX)) {
        *ret = (double) size/pow(2,40);
        memcpy(unit, "TiB", 3);

    } else if ((size >= PiB_MIN) && (size <= PiB_MAX)) {
        *ret = (double) size/pow(2,50);
        memcpy(unit, "PiB", 3);
    }
}

void print_tree(struct file_entity *root, int size_op, int tree_size, int perctg, size_t level, const char *pipes, char err)
{
    if ((unsigned int) tree_size < (level + 1)) {
        return;
    }

    size_t size = root->subfiles_allocated;
    struct file_entity *array = root->subfiles;  

    for (size_t i = 0; i < size; ++i) {
        if (err == '?') {
            if (array[i].err == '?') {
                printf("? ");
            } else {
                printf("  ");
            }
        }

        double ret = 0;
        int ret2;
        if (perctg) {
            if (size_op) {
                ret = (double) (array[i].byte_size)/perctg;
                ret *= 100;
                ret2 = (int) (ret * 10);
                printf("%3i.%i%% ", ret2 / 10, ret2 % 10);
            } else {
                ret = (double) (array[i].block_size)/perctg;
                ret *= 100;
                ret2 = (int) (ret * 10);
                printf("%3i.%i%% ", ret2 / 10, ret2 % 10);
            }
        } else {
            char unit[4] = {"   "};
            get_unit(&array[i], size_op, &ret, unit);
            ret2 = (int) (ret * 10);
            printf("%4i.%i ", ret2 / 10, ret2 % 10);

            //printf("%6.1f ", ret);
            printf("%3s ", unit);
        }

        if (level) {
            for (size_t space = 0; space < level; ++space) {
                printf("%c", pipes[space]);
                printf("   ");
            }
        }
        if ( i != (size - 1)) {
            printf("|");
        } else {
            printf("\\");
        }

        printf("-- ");
        printf("%s\n", array[i].name);

        if (array[i].subfiles_allocated) {
            char newtmp[1000] = {'\0'};
            strcat(newtmp, pipes);
            if (i == size - 1) {
                newtmp[level] = ' ';
            } else {
                newtmp[level] = '|';
            }
            print_tree(&array[i], size_op, tree_size, perctg, level + 1, newtmp, err);
        }
    }
}

void print_root(struct file_entity *root, int size_op, int tree_size, int perctg)
{
    char err = ' ';
    if ((root->err) == '?' ) {
        err = '?';
    }
    if (perctg) {
        if (err == '?') {
            printf("? ");
        }
        printf("100.0%% %s\n", root->name);
        long long root_size;
        if (size_op) {
            root_size = root->byte_size;
        } else {
            root_size = root->block_size;
        }
        print_tree(root, size_op, tree_size, root_size, 0, "|" ,err);

    } else {
        if (err == '?') {
            printf("? ");
        }
        double ret = 0;
        char unit[4] = {"   "};
        get_unit(root, size_op, &ret, unit);

        int ret2 = (int) (ret * 10);
        printf("%4i.%i ", ret2 / 10, ret2 % 10);

        printf("%3s %s\n", unit, root->name);
        print_tree(root, size_op, tree_size, 0, 0, "|", err);
    }
}
int main(int argc, char **argv)
{

    if (argc < 2) {
        return 1;
    }
    /*OPTIONS 0-YES 1-NO*/
    int _a = 0;
    int _s = 0;
    long int tree_size = -1;
    int _p = 0;
    if (set_options(&_a, &_s, &tree_size, &_p, argc, argv)) {
        fprintf(stderr, "Wrong options\n");
        return 1;
    }

    char filepath[4095] = {'\0'};    
    strcat(filepath, argv[argc - 1]);

    struct file_entity root;
    file_init(&root);
    strcpy(root.name, argv[argc - 1]);
    struct stat stat_buf;
    if (stat(argv[argc - 1], &stat_buf) < 0) {
        perror("STAT");
        return 1;
    }

    root.byte_size = (long long int) stat_buf.st_size;
    root.block_size = (long long int) stat_buf.st_blocks * 512;

    if (S_ISREG(stat_buf.st_mode)) {
        if (_p) {
            printf("100.0%% %s\n", filepath);
            return 0;
        }
        char unit[4] = {"   "};
        double ret = 0;
        get_unit(&root, _a, &ret, unit);
        int ret2 = (int) (ret * 10);

        printf("%4i.%i %3s %s\n", ret2 / 10, ret2 % 10, unit, root.name);

        return 0;
    }

    if (dir_read(filepath, &root, _s, _a) > 0) {
        dealloc(&root);
        return 1;
    }

    print_root(&root, _a, tree_size, _p);

    dealloc(&root);
    return 0;
}
