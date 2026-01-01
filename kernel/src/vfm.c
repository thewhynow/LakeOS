#define _VFM_H_INTERNAL
#include "../include/vfm.h"
#include "../include/rtc.h"
#include "../include/kmm.h"
#include "../../libc/include/string.h"

static t_FileChrono vfm_start_chrono;
static t_VFMFile *vfm_files;
t_VFSOperations vfm_vfs_ops = (t_VFSOperations){
	.f_Mount    = (void*) VFM_init,
	.f_UnMount  = (void*) VFM_uninit,
	.f_Root     = (void*) VFM_root,
	.f_Lookup   = (void*) VFM_lookup,
	.f_Create   = (void*) VFM_create,
	.f_Remove   = (void*) VFM_remove,
	.f_Open     = (void*) VFM_open,
	.f_Close    = (void*) VFM_close,
	.f_Read     = (void*) VFM_read,
	.f_Write    = (void*) VFM_write,
	.f_ReadDir  = (void*) VFM_ReadDir,
	.f_NodeName = (void*) VFM_nodename,
	.f_Stat     = (void*) VFM_fstat
};

/**
 * frees the file corresponding to the
 *  file handle.. if any reference to
 *  the name field is held anywhere it
 *  is hereby invalidated
 */
void VFM_free_file(t_VFMFile *file){
    kfree(file->content);
	kfree(file->name);
    kfree(file);
}

t_VFMFile *VFM_new_file(){
	if (!vfm_files){
		vfm_files = kmalloc(sizeof *vfm_files);

		return vfm_files;
	}

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

t_FSContext VFM_init(){
	VFS_curr_chrono(&vfm_start_chrono);
	return &vfm_files;
}

void VFM_uninit(){
	while (vfm_files->next){
		VFM_free_file(vfm_files);
		vfm_files = vfm_files->next;
	}

	VFM_free_file(vfm_files);
}

t_FSNode VFM_root(){
	return &vfm_files;
}

t_VFMHandle *VFM_lookup(t_FSNode _parent, const char *name){
	if (*(void**)_parent != vfm_files)
		return NULL;
	
	for (
		t_VFMHandle *iter = vfm_files; iter != NULL; iter = iter->next
	)
		if (!strcmp(iter->name, name))
			return iter;

	return NULL;
}

t_VFMHandle *VFM_create(t_VFMFile *parent, const char *name, uint8_t attribs){
	if (*(void**)parent != vfm_files) 
		return NULL;

    t_VFMFile *new = VFM_new_file();
    *new = (t_VFMFile){
        .content = NULL,
        .name = kmalloc(strlen(name)),
        .pos = 0,
        .opflags = 0,
        .fileflags = attribs, .modified = vfm_start_chrono
    };
    memcpy(new->name, name, strlen(name));
    t_VFMHandle *handle = kmalloc(sizeof(t_VFMHandle)); 
    
    handle = new;
    return handle;
}

/**
 * removes a virtual file and clears its corresponding memory
 * ... distinct from closing a virtual file
 */
void VFM_remove(t_VFMHandle *file){
    t_VFMFile *iter;
    while (iter->next != file) iter = iter->next;
    iter->next = file->next;

	/* remember, since the file and file handle are the same thing */
   	VFM_free_file((t_VFMFile*) file); 
}

t_VFMFile *VFM_open(t_VFMHandle *file, uint8_t mode){
    file->opflags = mode;
    file->pos = 0;
    
    return file;
}

void VFM_close(t_VFMFile *file){
    file->opflags = 0;
    file->pos = 0;

	/**
	 * future idea.. maybe write this file to a temporary location in disk ??
	 */
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

size_t VFM_writeback(t_VFMFile *file, size_t bytes, const void *buff){
    size_t fsize = ksize(file->content);
    if (file->pos + bytes > fsize)
        file->content = kexpand(file->content, bytes);
    memcpy(file->content + bytes, file->content, bytes);     
    memcpy(file->content, buff, bytes);

    VFS_curr_chrono(&file->modified);
    return bytes;
}

t_VFMFile *VFM_ReadDir(t_VFMFile *dir, size_t n){
	if (*(void**)dir != vfm_files) 
		return NULL;

	t_VFMFile *iter = dir;

	for (size_t i = 0; i < n; ++i)
		if (!(iter = iter->next))
			return NULL;

	return iter;
}

const char *VFM_nodename(t_VFMHandle *file){
	if (*(void**)file == vfm_files)
		return "VIRT";
	else
    	return file->name;
}

void VFM_fstat(t_VFMHandle *file, t_FileStat *out){
    *out = (t_FileStat){
        .created = vfm_start_chrono,
        .modified = file->modified,
        .size = ksize(file->content),
        .flags = file->fileflags
    };
}
