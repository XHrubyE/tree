#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "read.h"

void file_init(struct file_entity *file_entity)
{
    file_entity->name[0] = '\0';
    file_entity->err = ' ';
    file_entity->byte_size = 0;
    file_entity->block_size = 0;
    file_entity->subfiles_allocated = 0;
    file_entity->subfiles = NULL;
}

void file_copy(struct file_entity *to, struct file_entity *from, size_t amount)
{
    for (size_t var = 0; var < amount; ++var) {
        to[var] = from[var];
    }
}
void getpath(char *new_path ,const char *path,const char* name)
{   
    strcat(new_path, path);
    strcat(new_path, "/");
    strcat(new_path, name);
}

void dealloc(struct file_entity *file_entity)
{
    for (size_t i = 0; i < file_entity->subfiles_allocated; ++i) {
        if (file_entity->subfiles[i].subfiles_allocated != 0) {
            dealloc(&(file_entity->subfiles[i]));
        }
    }
    free(file_entity->subfiles);
}

int set_values(struct file_entity *current_file, struct stat *stat_buf,const char *filepath,const char *name)
{
    strcpy(current_file->name, name);
    if (stat(filepath, stat_buf) < 0) {
        perror("STAT");
        return 1;
    }
    current_file->byte_size = (long long int) stat_buf->st_size;
    current_file->block_size = (long long int) stat_buf->st_blocks * 512;

    return 0;
}

static int cmpfunc_name(const void *a, const void *b)
{
    struct file_entity *first = (struct file_entity *) a;
    struct file_entity *second = (struct file_entity *) b;
    return strcasecmp(first->name, second->name);
}

static int cmpfunc_size_actual(const void *a, const void *b)
{
    struct file_entity *first = (struct file_entity *) a;
    struct file_entity *second = (struct file_entity *) b;

    return (second->byte_size - first->byte_size);

}

static int cmpfunc_size_block(const void *a, const void *b)
{
    struct file_entity *first = (struct file_entity *) a;
    struct file_entity *second = (struct file_entity *) b;

    return (second->block_size - first->block_size);
}

int dir_read(const char *filepath, struct file_entity *parent, int sort, int size_op)
{
    DIR *dir = opendir(filepath);
    if (!dir) {
        fprintf(stderr, "Open error %s\n", filepath);
        return -1;
    }

    struct dirent *dirdata = NULL;

    size_t allocated = 4;
    size_t amount = 0;

    struct file_entity *files = malloc(allocated * sizeof(struct file_entity));
    if (!files) {
        perror("dir_read file_entity malloc");
        return 1;
    }

    char err_flag = ' ';
    while ((dirdata = readdir(dir))) {               
        if ( !strcmp(dirdata->d_name, ".") || !strcmp(dirdata->d_name, "..")) {
            continue; //skip dots
        } else {

            struct file_entity current_file;
            file_init(&current_file);
            char newfilepath[4095] = {'\0'};
            getpath(newfilepath, filepath, dirdata->d_name);

            struct stat stat_buf;
            if (dirdata->d_type == DT_DIR) {
                if ((set_values(&current_file, &stat_buf, newfilepath, dirdata->d_name))) {
                    free(files);
                    return 1;                    
                }

                if (dir_read(newfilepath, &current_file, sort, size_op) < 0) {
                    err_flag = '?';
                    current_file.err = err_flag;
                }
                parent->err = err_flag;
                parent->byte_size += current_file.byte_size;
                parent->block_size += current_file.block_size;

            } else if (dirdata->d_type == DT_REG) {
                if ((set_values(&current_file, &stat_buf, newfilepath, dirdata->d_name))) {
                    free(files);
                    return 1;
                }               
                parent->byte_size += current_file.byte_size;
                parent->block_size += current_file.block_size;
            } else {
                continue;
            }

            if (amount >= allocated) {
                allocated *= 2;
                struct file_entity *tmp = realloc(files, allocated * sizeof(struct file_entity));
                if (!tmp) {
                    perror("dir_read file_entity malloc");
                    free(files);
                    return 1;
                }
                files = tmp;
            }
            files[amount] = current_file;
            ++amount;
        }
    }
    closedir(dir);

    if (amount) {
        parent->subfiles = malloc(amount * sizeof(struct file_entity));

        if (!parent->subfiles) {
            perror("Parent subfiles malloc");
            free(files);
            return 1;
            free(files);
        }

        file_copy(parent->subfiles, files, amount);
        parent->subfiles_allocated = amount;

        qsort(parent->subfiles, amount, sizeof(struct file_entity), cmpfunc_name);
        if (sort) {
            if (size_op) {
                qsort(parent->subfiles, amount, sizeof(struct file_entity), cmpfunc_size_actual);
            } else {
                qsort(parent->subfiles, amount, sizeof(struct file_entity), cmpfunc_size_block);
            }
        }

    }
    //parent->subfiles = malloc(amount * sizeof(struct file_entity));

    free(files);

    if (err_flag == '?') {
        return -1;
    }
    return 0;
}
