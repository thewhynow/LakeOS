#define _VFM_H_INTERNAL
#include "../include/vfm.h"
#include "../include/rtc.h"
#include "../include/kmm.h"
#include "../../libc/include/string.h"

static t_FileChrono vfs_start_chrono;
static t_VFMFile *vfm_files;

/**
 * frees the file corresponding to the
 *  file handle.. if any reference to
 *  the name field is held anywhere it
 *  is hereby invalidated
 */
void VFM_free_file(t_VFMFile *file){
    kfree(file->content);
    kfree((void*)file);
    kfree(file);
}

t_VFMFile *VFM_new_file(){
    t_VFMFile *iter = vfm_files;
    while (iter->next) iter = iter->next; 
    iter->next = kmalloc(sizeof *iter);
    return iter->next;
}

/**
 * functions below this comment are direct definitions
 *  for the VFS API.. anything above are utility
 *  functions written
 */

void VFM_init(){
    VFS_curr_chrono(&vfs_start_chrono);
}

t_VFMHandle *VFM_create(const char *name, uint8_t attribs){
    t_VFMFile *new = VFM_new_file();
    *new = (t_VFMFile){
        .content = NULL,
        .name = kmalloc(strlen(name)),
        .pos = 0,
        .opflags = 0,
        .fileflags = attribs,
        .modified = vfs_start_chrono
    };
    memcpy(new->name, name, strlen(name));
    t_VFMHandle *handle = kmalloc(sizeof(t_VFMHandle)); 
    
    handle = new;
    return handle;
}

void VFM_remove(t_VFMHandle *file){
    t_VFMFile *iter;
    while (iter->next != file) iter = iter->next;
    iter->next = file->next;

    kfree(file);
}

t_VFMFile *VFM_open(t_VFMHandle *file, uint8_t mode){
    file->opflags = mode;
    file->pos = 0;
    
    return file;
}

void VFM_close(t_VFMFile *file){
    file->opflags = 0;
    file->pos = 0;
}

size_t VFM_read(t_VFMFile *file, size_t bytes, void *buff){
    size_t fsize = ksize(file->content),
           csize = file->pos + bytes > fsize
                   ? fsize - file->pos : bytes;

    memcpy(buff, file->content + file->pos, csize);
    file->pos += csize;
    return csize;
}

size_t VFM_write(t_VFMFile *file, size_t bytes, const void *buff){
    size_t fsize = ksize(file->content);
    if (file->pos + bytes > fsize)
        file->content = kexpand(file->content, bytes); 

    memcpy(file->content + file->pos, buff, bytes);
    file->pos += bytes;
    VFS_curr_chrono(&file->modified);
    return bytes;
}

size_t VFM_pushqueue(t_VFMFile *file, size_t bytes, const void *buff){
    size_t fsize = ksize(file->content);
    if (file->pos + bytes > fsize)
        file->content = kexpand(file->content, bytes);
    memcpy(file->content + bytes, file->content, bytes);     
    memcpy(file->content, buff, bytes);

    VFS_curr_chrono(&file->modified);
    return bytes;
}

const char *VFM_nodename(t_VFMHandle *file){
    return file->name;
}

void VFM_fstat(t_VFMHandle *file, t_FileStat *out){
    *out = (t_FileStat){
        .created = vfs_start_chrono,
        .modified = file->modified,
        .size = ksize(file->content),
        .flags = file->fileflags
    };
}
