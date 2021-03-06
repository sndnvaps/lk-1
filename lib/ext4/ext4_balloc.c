/*
 * Copyright (c) 2013 Grzegorz Kostka (kostka.grzegorz@gmail.com)
 *
 *
 * HelenOS:
 * Copyright (c) 2012 Martin Sucha
 * Copyright (c) 2012 Frantisek Princ
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup lwext4
 * @{
 */
/**
 * @file  ext4_balloc.c
 * @brief Physical block allocator.
 */

#include <ext4_config.h>
#include <ext4_balloc.h>
#include <ext4_super.h>
#include <ext4_block_group.h>
#include <ext4_fs.h>
#include <ext4_bitmap.h>
#include <ext4_inode.h>


/**@brief Compute number of block group from block address.
 * @param sb         Superblock pointer.
 * @param baddr Absolute address of block.
 * @return Block group index
 */
static uint32_t ext4_balloc_get_bgid_of_block(struct ext4_sblock *s,
    uint32_t baddr)
{
    if(ext4_get32(s, first_data_block))
        baddr--;

    return baddr / ext4_get32(s, blocks_per_group);
}


uint32_t ext4_balloc_get_first_data_block_in_group(struct ext4_sblock *s,
    struct ext4_block_group_ref * bg_ref)
{
    uint32_t block_group_count = ext4_block_group_cnt(s);
    uint32_t inode_table_first_block =
            ext4_bg_get_inode_table_first_block(bg_ref->block_group, s);
    uint32_t block_size  = ext4_sb_get_block_size(s);

    uint16_t inode_size = ext4_get16(s, inode_size);
    uint32_t inodes_per_group = ext4_get32(s, inodes_per_group);

    uint32_t inode_table_bytes;


    if (bg_ref->index < block_group_count - 1) {
        inode_table_bytes = inodes_per_group * inode_size;
    } else {
        /* Last block group could be smaller */
        uint32_t inodes_count_total = ext4_get32(s, inodes_count);
        inode_table_bytes =
                (inodes_count_total - ((block_group_count - 1) *
                        inodes_per_group)) * inode_size;
    }

    uint32_t inode_table_blocks = inode_table_bytes / block_size;

    if (inode_table_bytes % block_size)
        inode_table_blocks++;

    return inode_table_first_block + inode_table_blocks;
}

int ext4_balloc_free_block(struct ext4_inode_ref *inode_ref, uint32_t baddr)
{
    struct ext4_fs *fs = inode_ref->fs;
    struct ext4_sblock *sb = &fs->sb;


    uint32_t block_group    = ext4_balloc_get_bgid_of_block(sb, baddr);
    uint32_t index_in_group = ext4_fs_baddr2_index_in_group(sb, baddr);

    /* Load block group reference */
    struct ext4_block_group_ref bg_ref;
    int rc = ext4_fs_get_block_group_ref(fs, block_group, &bg_ref);
    if (rc != EOK)
        return rc;

    /* Load block with bitmap */
    uint32_t bitmap_block_addr =
            ext4_bg_get_block_bitmap(bg_ref.block_group, sb);

    struct ext4_block bitmap_block;

    rc = ext4_block_get(fs->bdev, &bitmap_block, bitmap_block_addr);
    if (rc != EOK){
        ext4_fs_put_block_group_ref(&bg_ref);
        return rc;
    }

    /* Modify bitmap */
    ext4_bmap_bit_clr(bitmap_block.data, index_in_group);
    bitmap_block.dirty = true;

    /* Release block with bitmap */
    rc = ext4_block_set(fs->bdev, &bitmap_block);
    if (rc != EOK) {
        /* Error in saving bitmap */
        ext4_fs_put_block_group_ref(&bg_ref);
        return rc;
    }

    uint32_t block_size = ext4_sb_get_block_size(sb);

    /* Update superblock free blocks count */
    uint64_t sb_free_blocks =
            ext4_sb_get_free_blocks_cnt(sb);
    sb_free_blocks++;
    ext4_sb_set_free_blocks_cnt(sb, sb_free_blocks);

    /* Update inode blocks count */
    uint64_t ino_blocks =
            ext4_inode_get_blocks_count(sb, inode_ref->inode);
    ino_blocks -= block_size / EXT4_INODE_BLOCK_SIZE;
    ext4_inode_set_blocks_count(sb, inode_ref->inode, ino_blocks);
    inode_ref->dirty = true;

    /* Update block group free blocks count */
    uint32_t free_blocks =
            ext4_bg_get_free_blocks_count(bg_ref.block_group, sb);
    free_blocks++;
    ext4_bg_set_free_blocks_count(bg_ref.block_group,
        sb, free_blocks);

    bg_ref.dirty = true;

    /* Release block group reference */
    return ext4_fs_put_block_group_ref(&bg_ref);
}

int ext4_balloc_free_blocks(struct ext4_inode_ref *inode_ref, uint32_t first,
    uint32_t count)
{
    struct ext4_fs *fs = inode_ref->fs;
    struct ext4_sblock *sb = &fs->sb;

    /* Compute indexes */
    uint32_t block_group_first =
            ext4_balloc_get_bgid_of_block(sb, first);

    ext4_assert(block_group_first ==
    		ext4_balloc_get_bgid_of_block(sb, first + count - 1));

    /* Load block group reference */
    struct ext4_block_group_ref bg_ref;
    int rc = ext4_fs_get_block_group_ref(fs, block_group_first, &bg_ref);
    if (rc != EOK)
        return rc;

    uint32_t index_in_group_first =
            ext4_fs_baddr2_index_in_group(sb, first);

    /* Load block with bitmap */
    uint32_t bitmap_block_addr =
            ext4_bg_get_block_bitmap(bg_ref.block_group, sb);

    struct ext4_block bitmap_block;

    rc = ext4_block_get(fs->bdev, &bitmap_block, bitmap_block_addr);
    if (rc != EOK){
        ext4_fs_put_block_group_ref(&bg_ref);
        return rc;
    }

    /* Modify bitmap */
    ext4_bmap_bits_free(bitmap_block.data, index_in_group_first, count);
    bitmap_block.dirty = true;

    /* Release block with bitmap */
    rc = ext4_block_set(fs->bdev, &bitmap_block);
    if (rc != EOK) {
        ext4_fs_put_block_group_ref(&bg_ref);
        return rc;
    }

    uint32_t block_size = ext4_sb_get_block_size(sb);

    /* Update superblock free blocks count */
    uint64_t sb_free_blocks =
            ext4_sb_get_free_blocks_cnt(sb);
    sb_free_blocks += count;
    ext4_sb_set_free_blocks_cnt(sb, sb_free_blocks);

    /* Update inode blocks count */
    uint64_t ino_blocks =
            ext4_inode_get_blocks_count(sb, inode_ref->inode);
    ino_blocks -= count * (block_size / EXT4_INODE_BLOCK_SIZE);
    ext4_inode_set_blocks_count(sb, inode_ref->inode, ino_blocks);
    inode_ref->dirty = true;

    /* Update block group free blocks count */
    uint32_t free_blocks =
            ext4_bg_get_free_blocks_count(bg_ref.block_group, sb);
    free_blocks += count;
    ext4_bg_set_free_blocks_count(bg_ref.block_group,
        sb, free_blocks);
    bg_ref.dirty = true;

    /* Release block group reference */
    return ext4_fs_put_block_group_ref(&bg_ref);
}


/**@brief Compute 'goal' for allocation algorithm.
 * @param inode_ref Reference to inode, to allocate block for
 * @param goal
 * @return error code
 */
static int ext4_balloc_find_goal(struct ext4_inode_ref *inode_ref, uint32_t *goal)
{
    struct ext4_sblock *sb = &inode_ref->fs->sb;
    *goal = 0;

    uint64_t inode_size = ext4_inode_get_size(sb, inode_ref->inode);
    uint32_t block_size = ext4_sb_get_block_size(sb);
    uint32_t inode_block_count = inode_size / block_size;

    if (inode_size % block_size != 0)
        inode_block_count++;

    /* If inode has some blocks, get last block address + 1 */
    if (inode_block_count > 0) {
        int rc = ext4_fs_get_inode_data_block_index(inode_ref,
                inode_block_count - 1, goal);
        if (rc != EOK)
            return rc;

        if (*goal != 0) {
            (*goal)++;
            return rc;
        }

        /* If goal == 0, sparse file -> continue */
    }

    /* Identify block group of inode */

    uint32_t inodes_per_group = ext4_get32(sb, inodes_per_group);
    uint32_t block_group = (inode_ref->index - 1) / inodes_per_group;
    block_size = ext4_sb_get_block_size(sb);

    /* Load block group reference */
    struct ext4_block_group_ref bg_ref;
    int rc = ext4_fs_get_block_group_ref(inode_ref->fs,
            block_group, &bg_ref);
    if (rc != EOK)
        return rc;

    /* Compute indexes */
    uint32_t block_group_count = ext4_block_group_cnt(sb);
    uint32_t inode_table_first_block =
            ext4_bg_get_inode_table_first_block(bg_ref.block_group, sb);
    uint16_t inode_table_item_size = ext4_get16(sb, inode_size);
    uint32_t inode_table_bytes;

    /* Check for last block group */
    if (block_group < block_group_count - 1) {
        inode_table_bytes = inodes_per_group * inode_table_item_size;
    } else {
        /* Last block group could be smaller */
        uint32_t inodes_count_total = ext4_get32(sb, inodes_count);

        inode_table_bytes =
                (inodes_count_total - ((block_group_count - 1) *
                        inodes_per_group)) * inode_table_item_size;
    }

    uint32_t inode_table_blocks = inode_table_bytes / block_size;

    if (inode_table_bytes % block_size)
        inode_table_blocks++;

    *goal = inode_table_first_block + inode_table_blocks;

    return  ext4_fs_put_block_group_ref(&bg_ref);
}


int ext4_balloc_alloc_block(struct ext4_inode_ref *inode_ref,
    uint32_t *fblock)
{
    uint32_t allocated_block = 0;
    uint32_t bitmap_block_addr;
    uint32_t rel_block_idx = 0;
    uint32_t free_blocks;
    uint32_t goal;
    struct ext4_block bitmap_block;

    int rc = ext4_balloc_find_goal(inode_ref, &goal);
    if (rc != EOK) {
        /* no goal found => partition is full */
        return rc;
    }

    struct ext4_sblock *sb = &inode_ref->fs->sb;

    /* Load block group number for goal and relative index */
    uint32_t block_group = ext4_balloc_get_bgid_of_block(sb, goal);
    uint32_t index_in_group =
            ext4_fs_baddr2_index_in_group(sb, goal);

    /* Load block group reference */
    struct ext4_block_group_ref bg_ref;
    rc = ext4_fs_get_block_group_ref(inode_ref->fs,
            block_group, &bg_ref);
    if (rc != EOK)
        return rc;

    free_blocks =
            ext4_bg_get_free_blocks_count(bg_ref.block_group, sb);
    if (free_blocks == 0) {
        /* This group has no free blocks */
        goto goal_failed;
    }

    /* Compute indexes */
    uint32_t first_in_group =
            ext4_balloc_get_first_data_block_in_group(sb, &bg_ref);

    uint32_t first_in_group_index =
            ext4_fs_baddr2_index_in_group(sb, first_in_group);

    if (index_in_group < first_in_group_index)
        index_in_group = first_in_group_index;

    /* Load block with bitmap */
    bitmap_block_addr =
            ext4_bg_get_block_bitmap(bg_ref.block_group, sb);

    rc = ext4_block_get(inode_ref->fs->bdev, &bitmap_block,
            bitmap_block_addr);
    if (rc != EOK) {
        ext4_fs_put_block_group_ref(&bg_ref);
        return rc;
    }

    /* Check if goal is free */
    if (ext4_bmap_is_bit_clr(bitmap_block.data, index_in_group)) {
        ext4_bmap_bit_set(bitmap_block.data, index_in_group);
        bitmap_block.dirty = true;
        rc = ext4_block_set(inode_ref->fs->bdev, &bitmap_block);
        if (rc != EOK) {
            ext4_fs_put_block_group_ref(&bg_ref);
            return rc;
        }

        allocated_block =
                ext4_fs_index_in_group2_baddr(sb, index_in_group,
                        block_group);

        goto success;
    }

    uint32_t blocks_in_group =
            ext4_blocks_in_group_cnt(sb, block_group);

    uint32_t end_idx = (index_in_group + 63) & ~63;
    if (end_idx > blocks_in_group)
        end_idx = blocks_in_group;

    /* Try to find free block near to goal */
    uint32_t tmp_idx;
    for (tmp_idx = index_in_group + 1; tmp_idx < end_idx;
            ++tmp_idx) {
        if (ext4_bmap_is_bit_clr(bitmap_block.data, tmp_idx)) {
            ext4_bmap_bit_set(bitmap_block.data, tmp_idx);

            bitmap_block.dirty = true;
            rc = ext4_block_set(inode_ref->fs->bdev, &bitmap_block);
            if (rc != EOK)
                return rc;

            allocated_block =
                    ext4_fs_index_in_group2_baddr(sb, tmp_idx,
                            block_group);

            goto success;
        }
    }



    /* Find free bit in bitmap */
    rc = ext4_bmap_bit_find_clr(bitmap_block.data,
            index_in_group, blocks_in_group, &rel_block_idx);
    if (rc == EOK) {
        ext4_bmap_bit_set(bitmap_block.data, rel_block_idx);
        bitmap_block.dirty = true;
        rc = ext4_block_set(inode_ref->fs->bdev, &bitmap_block);
        if (rc != EOK)
            return rc;

        allocated_block =
                ext4_fs_index_in_group2_baddr(sb, rel_block_idx,
                        block_group);

        goto success;
    }

    /* No free block found yet */
    rc = ext4_block_set(inode_ref->fs->bdev, &bitmap_block);
    if(rc != EOK){
        ext4_fs_put_block_group_ref(&bg_ref);
        return rc;
    }

goal_failed:

    rc = ext4_fs_put_block_group_ref(&bg_ref);
    if(rc != EOK)
        return rc;


    /* Try other block groups */
    uint32_t block_group_count = ext4_block_group_cnt(sb);

    uint32_t bgid = (block_group + 1) % block_group_count;
    uint32_t count = block_group_count;

    while (count > 0) {
        rc = ext4_fs_get_block_group_ref(inode_ref->fs, bgid,
                &bg_ref);
        if (rc != EOK)
            return rc;

        free_blocks =
                ext4_bg_get_free_blocks_count(bg_ref.block_group, sb);
        if (free_blocks == 0) {
            /* This group has no free blocks */
            goto next_group;
        }

        /* Load block with bitmap */
        bitmap_block_addr =
                ext4_bg_get_block_bitmap(bg_ref.block_group, sb);

        rc = ext4_block_get(inode_ref->fs->bdev, &bitmap_block,
                bitmap_block_addr);

        if (rc != EOK) {
            ext4_fs_put_block_group_ref(&bg_ref);
            return rc;
        }

        /* Compute indexes */
        first_in_group =
                ext4_balloc_get_first_data_block_in_group(sb, &bg_ref);
        index_in_group =
                ext4_fs_baddr2_index_in_group(sb, first_in_group);
        blocks_in_group = ext4_blocks_in_group_cnt(sb, bgid);

        first_in_group_index =
                ext4_fs_baddr2_index_in_group(sb, first_in_group);

        if (index_in_group < first_in_group_index)
            index_in_group = first_in_group_index;


        rc = ext4_bmap_bit_find_clr(bitmap_block.data,
                index_in_group, blocks_in_group, &rel_block_idx);

        if (rc == EOK) {

            ext4_bmap_bit_set(bitmap_block.data, rel_block_idx);

            bitmap_block.dirty = true;
            rc = ext4_block_set(inode_ref->fs->bdev, &bitmap_block);
            if (rc != EOK){
                ext4_fs_put_block_group_ref(&bg_ref);
                return rc;
            }

            allocated_block =
                    ext4_fs_index_in_group2_baddr(sb, rel_block_idx,
                            bgid);

            goto success;
        }

        rc = ext4_block_set(inode_ref->fs->bdev, &bitmap_block);
        if(rc != EOK){
            ext4_fs_put_block_group_ref(&bg_ref);
            return rc;
        }

next_group:
        rc = ext4_fs_put_block_group_ref(&bg_ref);
        if(rc != EOK){
            return rc;
        }

        /* Goto next group */
        bgid = (bgid + 1) % block_group_count;
        count--;
    }

    return ENOSPC;

    success:
    /* Empty command - because of syntax */
    ;

    uint32_t block_size = ext4_sb_get_block_size(sb);

    /* Update superblock free blocks count */
    uint64_t sb_free_blocks = ext4_sb_get_free_blocks_cnt(sb);
    sb_free_blocks--;
    ext4_sb_set_free_blocks_cnt(sb, sb_free_blocks);

    /* Update inode blocks (different block size!) count */
    uint64_t ino_blocks =
            ext4_inode_get_blocks_count(sb, inode_ref->inode);
    ino_blocks += block_size / EXT4_INODE_BLOCK_SIZE;
    ext4_inode_set_blocks_count(sb, inode_ref->inode, ino_blocks);
    inode_ref->dirty = true;

    /* Update block group free blocks count */
    uint32_t bg_free_blocks =
            ext4_bg_get_free_blocks_count(bg_ref.block_group, sb);
    bg_free_blocks--;
    ext4_bg_set_free_blocks_count(bg_ref.block_group, sb,
        bg_free_blocks);

    bg_ref.dirty = true;

    rc = ext4_fs_put_block_group_ref(&bg_ref);

    *fblock = allocated_block;
    return rc;
}

int ext4_balloc_try_alloc_block(struct ext4_inode_ref *inode_ref,
    uint32_t baddr, bool *free)
{
    int rc;

    struct ext4_fs *fs = inode_ref->fs;
    struct ext4_sblock *sb = &fs->sb;

    /* Compute indexes */
    uint32_t block_group = ext4_balloc_get_bgid_of_block(sb, baddr);
    uint32_t index_in_group =
            ext4_fs_baddr2_index_in_group(sb, baddr);

    /* Load block group reference */
    struct ext4_block_group_ref bg_ref;
    rc = ext4_fs_get_block_group_ref(fs, block_group, &bg_ref);
    if (rc != EOK)
        return rc;

    /* Load block with bitmap */
    uint32_t bitmap_block_addr =
            ext4_bg_get_block_bitmap(bg_ref.block_group, sb);


    struct ext4_block bitmap_block;

    rc = ext4_block_get(fs->bdev, &bitmap_block, bitmap_block_addr);
    if (rc != EOK){
        ext4_fs_put_block_group_ref(&bg_ref);
        return rc;
    }

    /* Check if block is free */
    *free = ext4_bmap_is_bit_clr(bitmap_block.data, index_in_group);

    /* Allocate block if possible */
    if (*free) {
        ext4_bmap_bit_set(bitmap_block.data, index_in_group);
        bitmap_block.dirty = true;
    }

    /* Release block with bitmap */
    rc = ext4_block_set(fs->bdev, &bitmap_block);
    if (rc != EOK) {
        /* Error in saving bitmap */
        ext4_fs_put_block_group_ref(&bg_ref);
        return rc;
    }

    /* If block is not free, return */
    if (!(*free))
        goto terminate;

    uint32_t block_size = ext4_sb_get_block_size(sb);

    /* Update superblock free blocks count */
    uint64_t sb_free_blocks = ext4_sb_get_free_blocks_cnt(sb);
    sb_free_blocks--;
    ext4_sb_set_free_blocks_cnt(sb, sb_free_blocks);

    /* Update inode blocks count */
    uint64_t ino_blocks =
            ext4_inode_get_blocks_count(sb, inode_ref->inode);
    ino_blocks += block_size / EXT4_INODE_BLOCK_SIZE;
    ext4_inode_set_blocks_count(sb, inode_ref->inode, ino_blocks);
    inode_ref->dirty = true;

    /* Update block group free blocks count */
    uint32_t free_blocks =
            ext4_bg_get_free_blocks_count(bg_ref.block_group, sb);
    free_blocks--;
    ext4_bg_set_free_blocks_count(bg_ref.block_group,
        sb, free_blocks);

    bg_ref.dirty = true;

    terminate:
    return ext4_fs_put_block_group_ref(&bg_ref);
}

/**
 * @}
 */
