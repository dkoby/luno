#ifndef _MROMFS_H
#define _MROMFS_H

#include <stdint.h>
#include <stdlib.h>

struct mromfs_t {
    const char *image;
    uint32_t size;
    uint32_t firstFile;
    int valid;
};

struct mromfs_fd_t {
    struct mromfs_t *fs;
    uint32_t start;
    uint32_t size;
    uint32_t offset;
};

int mromfsInit(struct mromfs_t *fs, const char *image, uint32_t size);
int mromfsOpen(struct mromfs_t *fs, struct mromfs_fd_t *fd, const char *name);
int mromfsRead(struct mromfs_fd_t *fd, uint8_t *buf, uint32_t len);

#define MROMFS_ERROR_INVALID         (-1)
#define MROMFS_ERROR_HEAD_INVALID    (-2)
#define MROMFS_ERROR_OUT_OF_BOUND    (-3)
#define MROMFS_ERROR_NOT_FOUND       (-4)
#define MROMFS_ERROR_XXX             (-5)

#endif

