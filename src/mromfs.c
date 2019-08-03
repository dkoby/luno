/*
 *
 */
#include <string.h>
/* */
#include "mromfs.h"

#if 0
#include <stdio.h>
    #define _PREFIX "ROMFS "
    #define _NL "\r\n"
    #define PRINTF(...) printf(__VA_ARGS__)
#else
    #define PRINTF(...)
#endif

#pragma pack(push, 1)
struct mromfs_head_t {
#define _SIGNATURE_SIZE    8
    const char signature[_SIGNATURE_SIZE]; /* -mromfs- */
    uint8_t reserved0[512 - _SIGNATURE_SIZE];
};
/* XXX Limit head of file to 512 bytes */
struct mromfs_file_head_t {
    uint32_t next;   /* Next file offset. */
    uint32_t size;   /* File size. */
    uint32_t offset; /* Offset of data in image. */
    const char name[];
    /* Zero terminated file name. */
    /* Four byte aligned data. */
};
#pragma pack(pop)

/*
 * RETURN
 *     0 on success, value less then zero on error.
 */
int mromfsInit(struct mromfs_t *fs, const char *image, uint32_t size)
{
    struct mromfs_head_t *head;

    fs->valid = 0;
    fs->image = image;
    fs->size = size;

    if (size < sizeof(struct mromfs_head_t))
        return MROMFS_ERROR_HEAD_INVALID;

    head = (struct mromfs_head_t*)image;

    if (strncmp(head->signature, "-MROMFS1", _SIGNATURE_SIZE) != 0)
        return MROMFS_ERROR_HEAD_INVALID;

    fs->firstFile = sizeof(struct mromfs_head_t);
    fs->valid = 1;
    return 0;
}
/*
 * RETURN
 *     0 on success, value less then zero on error.
 */
int mromfsOpen(struct mromfs_t *fs, struct mromfs_fd_t *fd, const char *name)
{
    struct mromfs_file_head_t *head;
    uint32_t offset;

    if (!fs->valid)
        return MROMFS_ERROR_INVALID;

    offset = fs->firstFile;
    do
    {
        if (offset == 0)
            break;
        if (offset >= fs->size)
            return MROMFS_ERROR_OUT_OF_BOUND;

        head = (struct mromfs_file_head_t *)(fs->image + offset);

        PRINTF(_PREFIX "File: next %08X, size %08X, offset %08X, name \"%s\"" _NL,
                head->next,
                head->size,
                head->offset,
                head->name);

        if (strcmp(name, head->name) == 0)
        {
            if (head->offset + head->size > fs->size)
                return MROMFS_ERROR_OUT_OF_BOUND;
            fd->fs = fs;
            fd->start  = head->offset;
            fd->size   = head->size;
            fd->offset = fd->start;
            return 0;
        }

        if (head->next == 0)
            break;
        offset = head->next;
    } while (1);

    return MROMFS_ERROR_NOT_FOUND;
}
/*
 * RETURN
 *    Count of readed bytes, zero if no bytes was readed, -1 on error.
 */
int mromfsRead(struct mromfs_fd_t *fd, uint8_t *buf, uint32_t len)
{
    uint32_t r;

    if (!fd->fs->valid)
        return MROMFS_ERROR_INVALID;

    if (len == 0)
        return 0;

    if (fd->offset > (fd->start + fd->size))
    {
        /* NOTREACHED */
        return MROMFS_ERROR_XXX;
    }

    r = fd->start + fd->size - fd->offset;
    if (r > len)
        r = len;

    PRINTF("R %u" _NL, r);
    memcpy(buf, fd->fs->image + fd->offset, r);
    fd->offset += r;

    return r;
}

