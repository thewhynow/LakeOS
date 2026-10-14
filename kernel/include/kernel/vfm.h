/**
 * @file vfm.h
 * @brief virtual file manager, an in-memory filesystem (a vfs driver)
 *  the root is one flat directory, and a file's contents are a kmalloc'd buffer
 */

#include "sal.h"
#include "vfs.h"

#ifdef _VFM_H_INTERNAL

/**
 * a virtual file, which is also the handle to it
 */
typedef struct t_VFMFile {
    t_FileChrono modified;
    /** the next file in the directory */
    struct t_VFMFile *next;
    /** the contents, its size is the size of the file */
    char *content,
    /** the file name */
         *name;
    /** where reads and writes happen */
    size_t pos;
    uint8_t opflags,  ///< 0 == closed
            fileflags; ///< e_FileAttribs
} t_VFMFile,
/** no reason to abstract handle beyond file */
    t_VFMHandle;

typedef struct {
    t_FileChrono start_chrono;
    t_VFMHandle *virt_files;
} t_VFMContext;

/**
 * @brief frees the file corresponding to the
 *  file handle.. if any reference to
 *  the name field is held anywhere it
 *  is hereby invalidated
 * @param file the file to free, it's not unlinked from the list
 */
void VFM_free_file(t_VFMFile *file);

/**
 * @brief allocates a file and adds it to the end of the directory
 * @return the new file, its fields aren't set
 */
t_VFMFile *VFM_new_file();

#else

typedef void t_VFMHandle,
             t_VFMContext,
             t_VFMFile;
#endif

#ifndef _VFM_H
#define _VFM_H

/**
 * the driver table for the vfs
 */
extern t_VFSOperations vfm_vfs_ops;


/**
 * for vfs api
 *  everything below is the vfm side of the t_VFSOperations
 *  function of the same job, see vfs.h
 */


/**
 * @brief Initializes the virtual file manager
 *  and allocates memory for all the
 *  relevant files and stuff,
 *  returns nothing really
 * @return the context, which is the list of files
 */
t_FSContext VFM_init();

/**
 * @brief Uninitializes the virtual file manager
 * 	and frees memory for all the relevant
 * 	files and stuff
 */
void VFM_uninit();

/**
 * @brief returns the root handle of the
 * 	virtual file directory
 * @return the root
 */
t_FSNode VFM_root();

/**
 * @brief returns a t_FSNode that points
 * 	to the virtual file @p name
 * @param parent the directory to look in, only the root works
 * @param name the file name, case sensitive
 * @return the file, NULL if it doesn't exist
 */
t_VFMHandle *VFM_lookup(t_FSNode parent, const char *name);

/**
 * @brief Creates a virtual file and returns a
 *  handle to it
 * @param parent the directory to create it in, only the root works
 * @param name the file name
 * @param atrribs e_FileAttribs or'd together
 * @return the handle, NULL if parent isn't the root
 */
t_VFMHandle *VFM_create(t_VFMFile *parent, const char *name, uint8_t atrribs);

/**
 * @brief Revmoes a virtual file and frees all
 * 	its corresponding memory
 *  distinct from closing a virtual file
 * @param file the file to remove
 */
void VFM_remove(t_VFMHandle *file);

/**
 * @brief Does nothing for now.. might retrieve the
 * 	cached data from disk later
 *  (it only stores the mode and sets the position to 0)
 * @param file the file to open
 * @param mode e_VFSFILEMODES
 * @return file, it is its own open context
 */
t_VFMFile *VFM_open(t_VFMFile *file, uint8_t mode);

/**
 * @brief Again does nothing for now.. might
 * 	cache the file contents to disk later
 *  (it only marks the file as closed)
 * @param file the file to close
 */
void VFM_close(t_VFMFile *file);

/**
 * @brief Read n bytes from a virtual file
 * @param file the file to read
 * @param bytes how many bytes to read
 * @param buff where the data goes
 * @return how many bytes were read, less than bytes at the end of the file
 */
size_t VFM_read(t_VFMFile *file, size_t bytes, void *buff);

/**
 * @brief Write n bytes to a virtual file
 *  at the current position, growing the file if needed
 * @param file the file to write
 * @param bytes how many bytes to write
 * @param buff the data
 * @return how many bytes were written
 */
size_t VFM_write(t_VFMFile *file, size_t bytes, const void *buff);

/**
 * @brief instead of appending data to the virtual
 *  file buffer, this function pushes data
 *  to the back of the buffer, acting as a
 *  queue pusher
 * @param file the file to push to
 * @param bytes how many bytes to push
 * @param buff the data
 * @return how many bytes were pushed
 */
size_t VFM_writeback(t_VFMFile *file, size_t bytes, const void *buff);

/**
 * @brief gets the nth file of the directory
 * @param dir the directory, only the root works
 * @param n index of the file
 * @return the file, NULL if n is past the end
 */
t_VFMFile *VFM_ReadDir(t_VFMFile *dir, size_t n);

/**
 * @brief gets the name of a file given the handle
 * @param file the root or a file
 * @return "VIRT" for the root, otherwise the file name
 */
const char *VFM_nodename(t_VFMHandle *file);

/**
 * @brief impelments fstat for virtual files
 * @param file the file
 * @param out where the stat goes
 */
void VFM_fstat(t_VFMHandle *file, t_FileStat *out);

/**
 * @brief sets the position of the file
 *  assumes it is within the bounds of the file
 * @param file the file
 * @param position the new position
 */
void VFM_seek(t_VFMFile *file, size_t position);

#endif
