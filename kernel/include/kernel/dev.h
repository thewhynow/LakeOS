/**
 * @file dev.h
 * @brief the device filesystem (STDIN, STDOUT), a vfs driver
 *  the root is one flat directory of t_DeviceFile's
 */

#include <kernel/vfs.h>

#ifndef __DEV_H
#define __DEV_H

typedef struct t_DeviceFile t_DeviceFile;

/**
 * a device, as a file. also what a t_FSNode points to
 *  for everything but the root
 */
struct t_DeviceFile {
    t_FileChrono modified;

    char *name;

    /**
     * reads from the device
     *  buff:  where the data goes
     *  bytes: the most that will be read
     * returns the amount read, f_read is NULL for write-only devices
     */
    size_t (*f_read)
        (void *buff, size_t bytes);
    /**
     * writes to the device
     *  buff:  the data
     *  bytes: how much of it
     * returns the amount written, f_write is NULL for read-only devices
     */
    size_t (*f_write)
        (const void *buff, size_t bytes);

    /** the next device in the list */
    t_DeviceFile *next;
};

/**
 * the driver table for the vfs
 */
extern t_VFSOperations dev_vfs_ops;

/**
 * @brief sets up the device list with STDOUT and STDIN
 *  this is the mount for the vfs
 * @return the context, which is just dev_vfs_ops
 */
t_FSContext DEV_init();

/**
 * @brief frees all the devices
 */
void DEV_uninit();

/**
 * @brief gets the root directory node
 * @return the root
 */
t_FSNode DEV_root();

/**
 * @brief finds a device by name
 * @param parent the directory to look in, only the root works
 * @param name the name of the device, case sensitive
 * @return the device, NULL if it doesn't exist
 */
t_DeviceFile *DEV_lookup(t_FSNode parent, const char *name);

/**
 * @brief reads from a device
 * @param file the device
 * @param bytes how many bytes to read
 * @param buff where the data goes
 * @return how many bytes were read
 */
size_t DEV_read(t_FSNode *file, size_t bytes, void *buff);

/**
 * @brief writes to a device
 * @param file the device
 * @param bytes how many bytes to write
 * @param buff the data
 * @return how many bytes were written
 */
size_t DEV_write(t_FSNode *file, size_t bytes, const void *buff);

/**
 * @brief gets the nth device in the directory
 * @param dir the directory, only the root works
 * @param n index of the device
 * @return the device, NULL if n is past the end
 */
t_DeviceFile *DEV_readdir(t_FSNode dir, size_t n);

/**
 * @brief gets the name of a node
 * @param file the root or a device
 * @return "DEV" for the root, otherwise the device name
 */
const char *DEV_nodename(t_FSNode *file);

/**
 * @brief fills in the stat for a node, devices are always
 *  marked as system files with a size of 0
 * @param file the root or a device
 * @param out where the stat goes, zeroed for the root
 */
void DEV_fstat(t_FSNode *file, t_FileStat *out);

#endif
