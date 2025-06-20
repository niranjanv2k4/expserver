#include "../xps.h"

void file_source_handler(void *ptr);
void file_source_close_handler(void *ptr);

xps_file_t *xps_file_create(xps_core_t *core, const char *file_path, int *error){
    assert(core!=NULL);
    assert(file_path!=NULL);
    assert(error!=NULL);


    logger(LOG_INFO, "xps_file_create()", "in xps_file_create()");


    *error = E_FAIL;

    FILE *file_struct = fopen(file_path, "rb");
    if(file_struct==NULL){
        logger(LOG_ERROR, "xps_file_create()", "fopen() failed");
        perror("Error message");
        return NULL;
    }

    if(fseek(file_struct, 0, SEEK_END)!=0){
        logger(LOG_ERROR, "get_file_size()", "fseek() failed");
        perror("Error message");
        fclose(file_struct);
        return NULL;
    }

    long size = ftell(file_struct);

    if(size<0){
        logger(LOG_ERROR, "xps_file_create()", "ftell() failed");
        perror("Error message");
        return NULL;
    }

    if(fseek(file_struct, 0, SEEK_SET)!=0){
        logger(LOG_ERROR, "get_file_size()", "fseek() failed");
        perror("Error message");
        fclose(file_struct);
        return NULL;
    }

    const char *mime_type = xps_get_mime(file_path);

    xps_file_t *file = malloc(sizeof(xps_file_t));
    if(file==NULL){
        logger(LOG_ERROR, "xps_file_create()", "malloc() failed for 'file'");
        perror("Error message");
        fclose(file_struct);
        return NULL;
    }

    xps_pipe_source_t *source = xps_pipe_source_create((void *)file, file_source_handler, file_source_close_handler);
    if(source==NULL){
        logger(LOG_ERROR, "xps_file_create()", "xps_pipe_source_create() failed");
        perror("Error message");
        fclose(file_struct);
        free(file);
        return NULL;
    }

    source->ready = true;

    file->file_path = file_path;
    file->file_struct = file_struct;
    file->size = size;
    file->source = source;
    file->mime_type = mime_type;

    *error = E_SUCCESS;

    logger(LOG_INFO, "xps_file_create()", "file created");

    return file;
}

void xps_file_destroy(xps_file_t *file){
    assert(file!=NULL);

    fclose(file->file_struct);
    xps_pipe_source_destroy(file->source);
    free(file);

    logger(LOG_DEBUG, "xps_file_destroy()", "file destroyed");
}

void file_source_handler(void *ptr){
    assert(ptr!=NULL);

    xps_pipe_source_t *source = ptr;
    xps_file_t *file = source->ptr;

    xps_buffer_t *buff = xps_buffer_create(DEFAULT_BUFFER_SIZE, 0, NULL);
    if(buff==NULL){
        logger(LOG_ERROR, "file_source_handler()", "xps_buffer_create() failed");
        return;
    }

    ssize_t read_n = fread(buff->data, 1, buff->size, file->file_struct);
    buff->len = read_n;

    if(ferror(file->file_struct)){
        logger(LOG_ERROR, "file_source_handler()", "read error occured");
        perror("Error message");
        return;
    }

    if(read_n==0 && feof(file->file_struct)){
        xps_buffer_destroy(buff);
        xps_file_destroy(file);
        return;
    }

    if(xps_pipe_source_write(source, buff)!=E_SUCCESS){
        logger(LOG_ERROR, "file_source_handler()", "xps_pipe_source_write() failed");
        return;
    }

    xps_buffer_destroy(buff);
}


void file_source_close_handler(void *ptr){
    assert(ptr!=NULL);

    xps_pipe_source_t *source = ptr;
    xps_file_t *file = source->ptr;

    xps_file_destroy(file);
}
