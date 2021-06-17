#include <stdio.h>

struct file_entity {
    char name[256];
    char err;
    long long int byte_size;
    long long int block_size;
    size_t subfiles_allocated;
    struct file_entity *subfiles;
};


void file_init(struct file_entity *file_entity);
void file_copy(struct file_entity *to, struct file_entity *from, size_t amount);
void get_path(char *new_path ,const char *path,const char* name);
void dealloc(struct file_entity *file_entity);
int set_values(struct file_entity *current_file, struct stat *stat_buf,const char *filepath,const char *name);
int dir_read(const char *filepath, struct file_entity *parent, int sort, int size_op);

