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
 * @file  ext4_fs.c
 * @brief More complex filesystem functions.
 */

#include <ext4_config.h>
#include <ext4_types.h>
#include <ext4_fs.h>
#include <ext4_errno.h>
#include <ext4_blockdev.h>
#include <ext4_super.h>
#include <ext4_debug.h>
#include <ext4_block_group.h>
#include <ext4_balloc.h>
#include <ext4_bitmap.h>
#include <ext4_inode.h>
#include <ext4_ialloc.h>
#include <ext4_extent.h>
#include <string.h>

int ext4_fs_init(struct ext4_fs *fs, struct ext4_blockdev *bdev)
{
    int r, i;
    uint16_t tmp;
    uint32_t bsize;
    bool read_only = false;

    ext4_assert(fs && bdev);

    fs->bdev = bdev;

    r = ext4_sb_read(fs->bdev, &fs->sb);
    if(r != EOK)
        return r;

    if(!ext4_sb_check(&fs->sb))
        return ENOTSUP;

    bsize = ext4_sb_get_block_size(&fs->sb);
    if (bsize > EXT4_MAX_BLOCK_SIZE)
        return ENXIO;

    r = ext4_fs_check_features(fs, &read_only);
    if(r != EOK)
        return r;

#if !CONFIG_EXT4_READONLY
    if(read_only)
        return ENOTSUP;
#endif

    /* Compute limits for indirect block levels */
    uint32_t blocks_id = bsize / sizeof(uint32_t);

    fs->inode_block_limits[0] = EXT4_INODE_DIRECT_BLOCK_COUNT;
    fs->inode_blocks_per_level[0] = 1;

    for (i = 1; i < 4; i++) {
        fs->inode_blocks_per_level[i] = fs->inode_blocks_per_level[i - 1] *
                blocks_id;
        fs->inode_block_limits[i] = fs->inode_block_limits[i - 1] +
                fs->inode_blocks_per_level[i];
    }

    /*Validate FS*/
    tmp = ext4_get16(&fs->sb, state);
    if (tmp & EXT4_SUPERBLOCK_STATE_ERROR_FS) {
        ext4_dprintf(EXT4_DEBUG_FS,
                "Filesystem was not cleanly unmounted before \n");
    }

    /* Mark system as mounted */
    ext4_set16(&fs->sb, state, EXT4_SUPERBLOCK_STATE_ERROR_FS);
    r = ext4_sb_write(fs->bdev, &fs->sb);
    if (r != EOK)
        return r;


    /*Update mount count*/
    ext4_set16(&fs->sb, mount_count, ext4_get16(&fs->sb, mount_count) + 1);

    return r;
}


int ext4_fs_fini(struct ext4_fs *fs)
{
    ext4_assert(fs);

    /*Set superblock state*/
    ext4_set16(&fs->sb, state, EXT4_SUPERBLOCK_STATE_VALID_FS);

    return ext4_sb_write(fs->bdev, &fs->sb);
}

static void ext4_fs_debug_features_incomp(uint32_t features_incompatible)
{

    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_COMPRESSION){
        ext4_dprintf(EXT4_DEBUG_FS, "COMPRESSION\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_FILETYPE){
        ext4_dprintf(EXT4_DEBUG_FS, "FILETYPE\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_RECOVER){
        ext4_dprintf(EXT4_DEBUG_FS, "RECOVER\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_JOURNAL_DEV){
        ext4_dprintf(EXT4_DEBUG_FS,"JOURNAL_DEV\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_META_BG){
        ext4_dprintf(EXT4_DEBUG_FS, "META_BG\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_EXTENTS){
        ext4_dprintf(EXT4_DEBUG_FS, "EXTENTS\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_64BIT){
        ext4_dprintf(EXT4_DEBUG_FS, "64BIT\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_MMP){
        ext4_dprintf(EXT4_DEBUG_FS, "MMP\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_FLEX_BG){
        ext4_dprintf(EXT4_DEBUG_FS, "FLEX_BG\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_EA_INODE){
        ext4_dprintf(EXT4_DEBUG_FS, "EA_INODE\n");
    }
    if(features_incompatible &
            EXT4_FEATURE_INCOMPAT_DIRDATA){
        ext4_dprintf(EXT4_DEBUG_FS, "DIRDATA\n");
    }
}
static void ext4_fs_debug_features_comp(uint32_t features_compatible)
{
    if(features_compatible &
            EXT4_FEATURE_COMPAT_DIR_PREALLOC){
        ext4_dprintf(EXT4_DEBUG_FS, "DIR_PREALLOC\n");
    }
    if(features_compatible &
            EXT4_FEATURE_COMPAT_IMAGIC_INODES){
        ext4_dprintf(EXT4_DEBUG_FS, "IMAGIC_INODES\n");
    }
    if(features_compatible &
            EXT4_FEATURE_COMPAT_HAS_JOURNAL){
        ext4_dprintf(EXT4_DEBUG_FS, "HAS_JOURNAL\n");
    }
    if(features_compatible &
            EXT4_FEATURE_COMPAT_EXT_ATTR){
        ext4_dprintf(EXT4_DEBUG_FS, "EXT_ATTR\n");
    }
    if(features_compatible &
            EXT4_FEATURE_COMPAT_RESIZE_INODE){
        ext4_dprintf(EXT4_DEBUG_FS, "RESIZE_INODE\n");
    }
    if(features_compatible &
            EXT4_FEATURE_COMPAT_DIR_INDEX){
        ext4_dprintf(EXT4_DEBUG_FS, "DIR_INDEX\n");
    }
}

static void ext4_fs_debug_features_ro(uint32_t features_ro)
{
    if(features_ro &
            EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER){
        ext4_dprintf(EXT4_DEBUG_FS, "SPARSE_SUPER\n");
    }
    if(features_ro &
            EXT4_FEATURE_RO_COMPAT_LARGE_FILE){
        ext4_dprintf(EXT4_DEBUG_FS, "LARGE_FILE\n");
    }
    if(features_ro &
            EXT4_FEATURE_RO_COMPAT_BTREE_DIR){
        ext4_dprintf(EXT4_DEBUG_FS, "BTREE_DIR\n");
    }
    if(features_ro &
            EXT4_FEATURE_RO_COMPAT_HUGE_FILE){
        ext4_dprintf(EXT4_DEBUG_FS, "HUGE_FILE\n");
    }
    if(features_ro &
            EXT4_FEATURE_RO_COMPAT_GDT_CSUM){
        ext4_dprintf(EXT4_DEBUG_FS, "GDT_CSUM\n");
    }
    if(features_ro &
            EXT4_FEATURE_RO_COMPAT_DIR_NLINK){
        ext4_dprintf(EXT4_DEBUG_FS, "DIR_NLINK\n");
    }
    if(features_ro &
            EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE){
        ext4_dprintf(EXT4_DEBUG_FS, "EXTRA_ISIZE\n");
    }
}

int ext4_fs_check_features(struct ext4_fs *fs, bool *read_only)
{
    ext4_assert(fs && read_only);
    uint32_t v;
    if(ext4_get32(&fs->sb, rev_level) == 0){
        *read_only = false;
        return EOK;
    }
    ext4_dprintf(EXT4_DEBUG_FS,
        "\nSblock rev_level: \n%d\n", (int)ext4_get32(&fs->sb, rev_level));

    ext4_dprintf(EXT4_DEBUG_FS,
        "\nSblock minor_rev_level: \n%d\n",
        ext4_get32(&fs->sb, minor_rev_level));

    ext4_dprintf(EXT4_DEBUG_FS,
        "\nSblock features_incompatible:\n");
    ext4_fs_debug_features_incomp(ext4_get32(&fs->sb, features_incompatible));

    ext4_dprintf(EXT4_DEBUG_FS,
        "\nSblock features_compatible:\n");
    ext4_fs_debug_features_comp(ext4_get32(&fs->sb, features_compatible));

    ext4_dprintf(EXT4_DEBUG_FS,
        "\nSblock features_read_only:\n");
    ext4_fs_debug_features_ro(ext4_get32(&fs->sb, features_read_only));

    /*Check features_incompatible*/
    v = (ext4_get32(&fs->sb, features_incompatible) &
            (~EXT4_FEATURE_INCOMPAT_SUPP));
    if (v){
        ext4_dprintf(EXT4_DEBUG_FS,
                "\nERROR sblock features_incompatible. Unsupported:\n");
        ext4_fs_debug_features_incomp(v);
#if !CONFIG_EXT4_READONLY
        return ENOTSUP;
#endif
    }

    v = (ext4_get32(&fs->sb, features_incompatible) &
            (EXT4_FEATURE_INCOMPAT_RECOVER));
    if(v & EXT4_FEATURE_INCOMPAT_RECOVER) {
        printf("Filesystem needs recovery");
#if !CONFIG_EXT4_READONLY
        printf(" - abort\n");
        return;
#else
        printf(" - ignore!\n");
#endif
    }


    /*Check features_read_only*/
    v = (ext4_get32(&fs->sb, features_read_only) &
            (~EXT4_FEATURE_RO_COMPAT_SUPP));
    if (v){
        ext4_dprintf(EXT4_DEBUG_FS,
                "\nERROR sblock features_read_only . Unsupported:\n");
        ext4_fs_debug_features_incomp(v);

        *read_only = true;
        return EOK;
    }
    *read_only = false;

    return EOK;
}

uint32_t ext4_fs_baddr2_index_in_group(struct ext4_sblock *s, uint32_t baddr)
{
    ext4_assert(baddr);
    if(ext4_get32(s, first_data_block))
        baddr--;

    return  baddr % ext4_get32(s, blocks_per_group);
}



uint32_t ext4_fs_index_in_group2_baddr(struct ext4_sblock *s, uint32_t index,
    uint32_t bgid)
{
    if(ext4_get32(s, first_data_block))
        index++;

    return ext4_get32(s, blocks_per_group) * bgid + index;
}



/**@brief Initialize block bitmap in block group.
 * @param bg_ref Reference to block group
 * @return Error code
 */
static int ext4_fs_init_block_bitmap(struct ext4_block_group_ref *bg_ref)
{
    uint32_t i;
    uint32_t bitmap_block_addr = ext4_bg_get_block_bitmap(
            bg_ref->block_group, &bg_ref->fs->sb);

    struct ext4_block block_bitmap;
    int rc = ext4_block_get(bg_ref->fs->bdev, &block_bitmap,
            bitmap_block_addr);
    if (rc != EOK)
        return rc;


    memset(block_bitmap.data, 0, ext4_sb_get_block_size(&bg_ref->fs->sb));

    /* Determine first block and first data block in group */
    uint32_t first_idx = 0;

    uint32_t first_data = ext4_balloc_get_first_data_block_in_group(
            &bg_ref->fs->sb, bg_ref);
    uint32_t first_data_idx = ext4_fs_baddr2_index_in_group(
            &bg_ref->fs->sb, first_data);

    /*Set bits from to first block to first data block - 1 to one (allocated)*/
    /*TODO: Optimize it*/
    for (i = first_idx; i < first_data_idx; ++i)
        ext4_bmap_bit_set(block_bitmap.data, i);


    block_bitmap.dirty = true;

    /* Save bitmap */
    return ext4_block_set(bg_ref->fs->bdev, &block_bitmap);
}

/**@brief Initialize i-node bitmap in block group.
 * @param bg_ref Reference to block group
 * @return Error code
 */
static int ext4_fs_init_inode_bitmap(struct ext4_block_group_ref *bg_ref)
{
    /* Load bitmap */
    uint32_t bitmap_block_addr = ext4_bg_get_inode_bitmap(
            bg_ref->block_group, &bg_ref->fs->sb);

    struct ext4_block block_bitmap;
    int rc = ext4_block_get(bg_ref->fs->bdev, &block_bitmap,
            bitmap_block_addr);
    if (rc != EOK)
        return rc;

    /* Initialize all bitmap bits to zero */
    uint32_t block_size = ext4_sb_get_block_size(&bg_ref->fs->sb);
    uint32_t inodes_per_group = ext4_get32(&bg_ref->fs->sb, inodes_per_group);

    memset(block_bitmap.data, 0, (inodes_per_group + 7) / 8);

    uint32_t start_bit = inodes_per_group;
    uint32_t end_bit = block_size * 8;

    uint32_t i;
    for (i = start_bit; i < ((start_bit + 7) & ~7UL); i++)
        ext4_bmap_bit_set(block_bitmap.data, i);

    if (i < end_bit)
        memset(block_bitmap.data + (i >> 3), 0xff, (end_bit - i) >> 3);

    block_bitmap.dirty = true;

    /* Save bitmap */
    return ext4_block_set(bg_ref->fs->bdev, &block_bitmap);
}

/**@brief Initialize i-node table in block group.
 * @param bg_ref Reference to block group
 * @return Error code
 */
static int ext4_fs_init_inode_table(struct ext4_block_group_ref *bg_ref)
{
    struct ext4_sblock *sb = &bg_ref->fs->sb;

    uint32_t inode_size = ext4_get32(sb, inode_size);
    uint32_t block_size = ext4_sb_get_block_size(sb);
    uint32_t inodes_per_block = block_size / inode_size;
    uint32_t inodes_in_group  = ext4_inodes_in_group_cnt(sb, bg_ref->index);
    uint32_t table_blocks = inodes_in_group / inodes_per_block;
    uint32_t fblock;

    if (inodes_in_group % inodes_per_block)
        table_blocks++;

    /* Compute initialization bounds */
    uint32_t first_block = ext4_bg_get_inode_table_first_block(
            bg_ref->block_group, sb);

    uint32_t last_block = first_block + table_blocks - 1;

    /* Initialization of all itable blocks */
    for (fblock = first_block; fblock <= last_block; ++fblock) {

        struct ext4_block block;
        int rc = ext4_block_get(bg_ref->fs->bdev, &block, fblock);
        if (rc != EOK)
            return rc;

        memset(block.data, 0, block_size);
        block.dirty = true;

        ext4_block_set(bg_ref->fs->bdev, &block);
        if (rc != EOK)
            return rc;
    }

    return EOK;
}


int ext4_fs_get_block_group_ref(struct ext4_fs *fs, uint32_t bgid,
    struct ext4_block_group_ref *ref)
{
    /* Compute number of descriptors, that fits in one data block */
    uint32_t dsc_per_block = ext4_sb_get_block_size(&fs->sb) /
            ext4_sb_get_desc_size(&fs->sb);

    /* Block group descriptor table starts at the next block after superblock */
    uint64_t block_id = ext4_get32(&fs->sb, first_data_block) + 1;

    /* Find the block containing the descriptor we are looking for */
    block_id += bgid / dsc_per_block;
    uint32_t offset = (bgid % dsc_per_block) *
            ext4_sb_get_desc_size(&fs->sb);


    int rc = ext4_block_get(fs->bdev, &ref->block, block_id);
    if (rc != EOK)
        return rc;

    ref->block_group = (void *)(ref->block.data + offset);
    ref->fs = fs;
    ref->index = bgid;
    ref->dirty = false;

    if (ext4_bg_has_flag(ref->block_group,
            EXT4_BLOCK_GROUP_BLOCK_UNINIT)) {
        rc = ext4_fs_init_block_bitmap(ref);
        if (rc != EOK) {
            ext4_block_set(fs->bdev, &ref->block);
            return rc;
        }
        ext4_bg_clear_flag(ref->block_group,
            EXT4_BLOCK_GROUP_BLOCK_UNINIT);

        ref->dirty = true;
    }

    if (ext4_bg_has_flag(ref->block_group,
            EXT4_BLOCK_GROUP_INODE_UNINIT)) {
        rc = ext4_fs_init_inode_bitmap(ref);
        if (rc != EOK) {
            ext4_block_set(ref->fs->bdev, &ref->block);
            return rc;
        }

        ext4_bg_clear_flag(ref->block_group,
            EXT4_BLOCK_GROUP_INODE_UNINIT);

        if (!ext4_bg_has_flag(ref->block_group,
                EXT4_BLOCK_GROUP_ITABLE_ZEROED)) {
            rc = ext4_fs_init_inode_table(ref);
            if (rc != EOK){
                ext4_block_set(fs->bdev, &ref->block);
                return rc;
            }

            ext4_bg_set_flag(ref->block_group,
                EXT4_BLOCK_GROUP_ITABLE_ZEROED);
        }

        ref->dirty = true;
    }

    return EOK;
}

/**@brief  Compute checksum of block group descriptor.
 * @param sb   Superblock
 * @param bgid Index of block group in the filesystem
 * @param bg   Block group to compute checksum for
 * @return Checksum value
 */
static uint16_t ext4_fs_bg_checksum(struct ext4_sblock *sb, uint32_t bgid,
    struct ext4_bgroup *bg)
{
    /* If checksum not supported, 0 will be returned */
    uint16_t crc = 0;

    /* Compute the checksum only if the filesystem supports it */
    if (ext4_sb_check_read_only(sb,
            EXT4_FEATURE_RO_COMPAT_GDT_CSUM)) {
        uint8_t  *base = (uint8_t  *)bg;
        uint8_t  *checksum = (uint8_t  *)&bg->checksum;

        uint32_t offset = (uint32_t) (checksum - base);

        /* Convert block group index to little endian */
        uint32_t le_group = to_le32(bgid);

        /* Initialization */
        crc = ext4_bg_crc16(~0, sb->uuid, sizeof(sb->uuid));

        /* Include index of block group */
        crc = ext4_bg_crc16(crc, (uint8_t *) &le_group, sizeof(le_group));

        /* Compute crc from the first part (stop before checksum field) */
        crc = ext4_bg_crc16(crc, (uint8_t *) bg, offset);

        /* Skip checksum */
        offset += sizeof(bg->checksum);

        /* Checksum of the rest of block group descriptor */
        if ((ext4_sb_check_feature_incompatible(sb,
                EXT4_FEATURE_INCOMPAT_64BIT)) &&
                (offset < ext4_sb_get_desc_size(sb)))

            crc = ext4_bg_crc16(crc, ((uint8_t *) bg) + offset,
                    ext4_sb_get_desc_size(sb) - offset);
    }
    return crc;
}

int ext4_fs_put_block_group_ref(struct ext4_block_group_ref *ref)
{
    /* Check if reference modified */
    if (ref->dirty) {
        /* Compute new checksum of block group */
        uint16_t checksum =
                ext4_fs_bg_checksum(&ref->fs->sb, ref->index,
                        ref->block_group);

        ref->block_group->checksum = to_le16(checksum);

        /* Mark block dirty for writing changes to physical device */
        ref->block.dirty = true;
    }

    /* Put back block, that contains block group descriptor */
    return ext4_block_set(ref->fs->bdev, &ref->block);
}

int ext4_fs_get_inode_ref(struct ext4_fs *fs, uint32_t index,
    struct ext4_inode_ref *ref)
{
    /* Compute number of i-nodes, that fits in one data block */
    uint32_t inodes_per_group = ext4_get32(&fs->sb, inodes_per_group);

    /*
     * Inode numbers are 1-based, but it is simpler to work with 0-based
     * when computing indices
     */
    index -= 1;
    uint32_t block_group = index / inodes_per_group;
    uint32_t offset_in_group = index % inodes_per_group;

    /* Load block group, where i-node is located */
    struct ext4_block_group_ref bg_ref;

    int rc = ext4_fs_get_block_group_ref(fs, block_group, &bg_ref);
    if (rc != EOK) {
        return rc;
    }

    /* Load block address, where i-node table is located */
    uint32_t inode_table_start =
            ext4_bg_get_inode_table_first_block(bg_ref.block_group,
                    &fs->sb);

    /* Put back block group reference (not needed more) */
    rc = ext4_fs_put_block_group_ref(&bg_ref);
    if (rc != EOK) {
        return rc;
    }

    /* Compute position of i-node in the block group */
    uint16_t inode_size = ext4_get16(&fs->sb, inode_size);
    uint32_t block_size = ext4_sb_get_block_size(&fs->sb);
    uint32_t byte_offset_in_group = offset_in_group * inode_size;

    /* Compute block address */
    uint64_t block_id = inode_table_start +
            (byte_offset_in_group / block_size);


    rc = ext4_block_get(fs->bdev, &ref->block, block_id);
    if (rc != EOK) {
        return rc;
    }

    /* Compute position of i-node in the data block */
    uint32_t offset_in_block = byte_offset_in_group % block_size;
    ref->inode = (struct ext4_inode *)(ref->block.data + offset_in_block);

    /* We need to store the original value of index in the reference */
    ref->index = index + 1;
    ref->fs = fs;
    ref->dirty = false;

    return EOK;
}

int ext4_fs_put_inode_ref(struct ext4_inode_ref *ref)
{
    /* Check if reference modified */
    if (ref->dirty) {
        /* Mark block dirty for writing changes to physical device */
        ref->block.dirty = true;
    }

    /* Put back block, that contains i-node */
    return  ext4_block_set(ref->fs->bdev, &ref->block);
}

int ext4_fs_alloc_inode(struct ext4_fs *fs, struct ext4_inode_ref *inode_ref,
    bool is_directory)
{
    /* Check if newly allocated i-node will be a directory */
    uint32_t i;
    bool is_dir;

    is_dir = is_directory;

    /* Allocate inode by allocation algorithm */
    uint32_t index;
    int rc = ext4_ialloc_alloc_inode(fs, &index, is_dir);
    if (rc != EOK)
        return rc;

    /* Load i-node from on-disk i-node table */
    rc = ext4_fs_get_inode_ref(fs, index, inode_ref);
    if (rc != EOK) {
        ext4_ialloc_free_inode(fs, index, is_dir);
        return rc;
    }

    /* Initialize i-node */
    struct ext4_inode *inode = inode_ref->inode;

    uint16_t mode;
    if (is_dir) {
        /*
         * Default directory permissions to be compatible with other systems
         * 0777 (octal) == rwxrwxrwx
         */

        mode = 0777;
        mode |= EXT4_INODE_MODE_DIRECTORY;
        ext4_inode_set_mode(&fs->sb, inode, mode);
        ext4_inode_set_links_count(inode, 1);  /* '.' entry */

    } else {
        /*
         * Default file permissions to be compatible with other systems
         * 0666 (octal) == rw-rw-rw-
         */

        mode = 0666;
        mode |= EXT4_INODE_MODE_FILE;
        ext4_inode_set_mode(&fs->sb, inode, mode);
        ext4_inode_set_links_count(inode, 0);
    }

    ext4_inode_set_uid(inode, 0);
    ext4_inode_set_gid(inode, 0);
    ext4_inode_set_size(inode, 0);
    ext4_inode_set_access_time(inode, 0);
    ext4_inode_set_change_inode_time(inode, 0);
    ext4_inode_set_modification_time(inode, 0);
    ext4_inode_set_deletion_time(inode, 0);
    ext4_inode_set_blocks_count(&fs->sb, inode, 0);
    ext4_inode_set_flags(inode, 0);
    ext4_inode_set_generation(inode, 0);

    /* Reset blocks array */
    for (i = 0; i < EXT4_INODE_BLOCKS; i++)
        inode->blocks[i] = 0;

#if CONFIG_EXTENT_ENABLE
    /* Initialize extents if needed */
    if (ext4_sb_check_feature_incompatible(
            &fs->sb, EXT4_FEATURE_INCOMPAT_EXTENTS)) {
        ext4_inode_set_flag(inode, EXT4_INODE_FLAG_EXTENTS);


        /* Initialize extent root header */
        struct ext4_extent_header *header = ext4_inode_get_extent_header(inode);
        ext4_extent_header_set_depth(header, 0);
        ext4_extent_header_set_entries_count(header, 0);
        ext4_extent_header_set_generation(header, 0);
        ext4_extent_header_set_magic(header, EXT4_EXTENT_MAGIC);

        uint16_t max_entries = (EXT4_INODE_BLOCKS * sizeof(uint32_t) -
                sizeof(struct ext4_extent_header)) / sizeof(struct ext4_extent);

        ext4_extent_header_set_max_entries_count(header, max_entries);
    }
#endif

    inode_ref->dirty = true;

    return EOK;
}

int ext4_fs_free_inode(struct ext4_inode_ref *inode_ref)
{
    struct ext4_fs *fs = inode_ref->fs;
    uint32_t offset;
    uint32_t suboffset;
#if CONFIG_EXTENT_ENABLE
    /* For extents must be data block destroyed by other way */
    if ((ext4_sb_check_feature_incompatible(&fs->sb,
            EXT4_FEATURE_INCOMPAT_EXTENTS)) &&
            (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))){
        /* Data structures are released during truncate operation... */
        goto finish;
    }
#endif
    /* Release all indirect (no data) blocks */

    /* 1) Single indirect */
    uint32_t fblock = ext4_inode_get_indirect_block(inode_ref->inode, 0);
    if (fblock != 0) {
        int rc = ext4_balloc_free_block(inode_ref, fblock);
        if (rc != EOK)
            return rc;

        ext4_inode_set_indirect_block(inode_ref->inode, 0, 0);
    }

    uint32_t block_size = ext4_sb_get_block_size(&fs->sb);
    uint32_t count = block_size / sizeof(uint32_t);

    struct ext4_block  block;

    /* 2) Double indirect */
    fblock = ext4_inode_get_indirect_block(inode_ref->inode, 1);
    if (fblock != 0) {
        int rc = ext4_block_get(fs->bdev, &block, fblock);
        if (rc != EOK)
            return rc;

        uint32_t ind_block;
        for (offset = 0; offset < count; ++offset) {
            ind_block = to_le32(((uint32_t *) block.data)[offset]);

            if (ind_block != 0) {
                rc = ext4_balloc_free_block(inode_ref, ind_block);
                if (rc != EOK) {
                    ext4_block_set(fs->bdev, &block);
                    return rc;
                }
            }
        }

        ext4_block_set(fs->bdev, &block);
        rc = ext4_balloc_free_block(inode_ref, fblock);
        if (rc != EOK)
            return rc;

        ext4_inode_set_indirect_block(inode_ref->inode, 1, 0);
    }

    /* 3) Tripple indirect */
    struct ext4_block  subblock;
    fblock = ext4_inode_get_indirect_block(inode_ref->inode, 2);
    if (fblock != 0) {
        int rc = ext4_block_get(fs->bdev, &block, fblock);
        if (rc != EOK)
            return rc;

        uint32_t ind_block;
        for ( offset = 0; offset < count; ++offset) {
            ind_block = to_le32(((uint32_t *) block.data)[offset]);

            if (ind_block != 0) {
                rc = ext4_block_get(fs->bdev, &subblock, ind_block);
                if (rc != EOK) {
                    ext4_block_set(fs->bdev, &block);
                    return rc;
                }

                uint32_t ind_subblock;
                for (suboffset = 0; suboffset < count;
                        ++suboffset) {
                    ind_subblock = to_le32(((uint32_t *)
                            subblock.data)[suboffset]);

                    if (ind_subblock != 0) {
                        rc = ext4_balloc_free_block(inode_ref, ind_subblock);
                        if (rc != EOK) {
                            ext4_block_set(fs->bdev, &subblock);
                            ext4_block_set(fs->bdev, &block);
                            return rc;
                        }
                    }
                }

                ext4_block_set(fs->bdev, &subblock);


                rc = ext4_balloc_free_block(inode_ref, ind_block);
                if (rc != EOK) {
                    ext4_block_set(fs->bdev, &block);
                    return rc;
                }
            }
        }

        ext4_block_set(fs->bdev, &block);
        rc = ext4_balloc_free_block(inode_ref, fblock);
        if (rc != EOK)
            return rc;

        ext4_inode_set_indirect_block(inode_ref->inode, 2, 0);
    }

    finish:
    /* Mark inode dirty for writing to the physical device */
    inode_ref->dirty = true;

    /* Free block with extended attributes if present */
    uint32_t xattr_block = ext4_inode_get_file_acl(
            inode_ref->inode, &fs->sb);
    if (xattr_block) {
        int rc = ext4_balloc_free_block(inode_ref, xattr_block);
        if (rc != EOK)
            return rc;

        ext4_inode_set_file_acl(inode_ref->inode, &fs->sb, 0);
    }

    /* Free inode by allocator */
    int rc;
    if (ext4_inode_is_type(&fs->sb, inode_ref->inode,
            EXT4_INODE_MODE_DIRECTORY))
        rc = ext4_ialloc_free_inode(fs, inode_ref->index, true);
    else
        rc = ext4_ialloc_free_inode(fs, inode_ref->index, false);

    return rc;
}

int ext4_fs_truncate_inode(struct ext4_inode_ref *inode_ref,
    uint64_t new_size)
{
    struct ext4_sblock *sb = &inode_ref->fs->sb;
    uint32_t i;

    /* Check flags, if i-node can be truncated */
    if (!ext4_inode_can_truncate(sb, inode_ref->inode))
        return EINVAL;

    /* If sizes are equal, nothing has to be done. */
    uint64_t old_size = ext4_inode_get_size(sb, inode_ref->inode);
    if (old_size == new_size)
        return EOK;

    /* It's not suppported to make the larger file by truncate operation */
    if (old_size < new_size)
        return EINVAL;

    /* Compute how many blocks will be released */
    uint64_t size_diff = old_size - new_size;
    uint32_t block_size  = ext4_sb_get_block_size(sb);
    uint32_t diff_blocks_count = size_diff / block_size;
    if (size_diff % block_size != 0)
        diff_blocks_count++;

    uint32_t old_blocks_count = old_size / block_size;
    if (old_size % block_size != 0)
        old_blocks_count++;
#if CONFIG_EXTENT_ENABLE
    if ((ext4_sb_check_feature_incompatible(sb,
            EXT4_FEATURE_INCOMPAT_EXTENTS)) &&
            (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))) {

        /* Extents require special operation */
        int rc = ext4_extent_release_blocks_from(inode_ref,
                old_blocks_count - diff_blocks_count);
        if (rc != EOK)
            return rc;

    } else
#endif
    {
        /* Release data blocks from the end of file */

        /* Starting from 1 because of logical blocks are numbered from 0 */
        for (i = 1; i <= diff_blocks_count; ++i) {
            int rc = ext4_fs_release_inode_block(inode_ref,
                    old_blocks_count - i);
            if (rc != EOK)
                return rc;
        }
    }

    /* Update i-node */
    ext4_inode_set_size(inode_ref->inode, new_size);
    inode_ref->dirty = true;

    return EOK;
}

int ext4_fs_get_inode_data_block_index(struct ext4_inode_ref *inode_ref,
    uint64_t iblock, uint32_t *fblock)
{
    struct ext4_fs *fs = inode_ref->fs;

    /* For empty file is situation simple */
    if (ext4_inode_get_size(&fs->sb, inode_ref->inode) == 0) {
        *fblock = 0;
        return EOK;
    }

    uint32_t current_block;
#if CONFIG_EXTENT_ENABLE
    /* Handle i-node using extents */
    if ((ext4_sb_check_feature_incompatible(&fs->sb,
            EXT4_FEATURE_INCOMPAT_EXTENTS)) &&
            (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))) {


        int rc = ext4_extent_find_block(inode_ref, iblock, &current_block);
        if (rc != EOK)
            return rc;

        *fblock = current_block;
        return EOK;
    }
#endif

    struct ext4_inode *inode = inode_ref->inode;

    /* Direct block are read directly from array in i-node structure */
    if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
        current_block = ext4_inode_get_direct_block(inode, (uint32_t) iblock);
        *fblock = current_block;
        return EOK;
    }

    /* Determine indirection level of the target block */
    unsigned int level = 0;
    unsigned int i;
    for (i = 1; i < 4; i++) {
        if (iblock < fs->inode_block_limits[i]) {
            level = i;
            break;
        }
    }

    if (level == 0)
        return EIO;

    /* Compute offsets for the topmost level */
    uint64_t block_offset_in_level =
            iblock - fs->inode_block_limits[level - 1];
    current_block = ext4_inode_get_indirect_block(inode, level - 1);
    uint32_t offset_in_block =
            block_offset_in_level / fs->inode_blocks_per_level[level - 1];

    /* Sparse file */
    if (current_block == 0) {
        *fblock = 0;
        return EOK;
    }

    struct ext4_block block;

    /*
     * Navigate through other levels, until we find the block number
     * or find null reference meaning we are dealing with sparse file
     */
    while (level > 0) {
        /* Load indirect block */
        int rc = ext4_block_get(fs->bdev, &block, current_block);
        if (rc != EOK)
            return rc;

        /* Read block address from indirect block */
        current_block =
                to_le32(((uint32_t *) block.data)[offset_in_block]);

        /* Put back indirect block untouched */
        rc = ext4_block_set(fs->bdev, &block);
        if (rc != EOK)
            return rc;

        /* Check for sparse file */
        if (current_block == 0) {
            *fblock = 0;
            return EOK;
        }

        /* Jump to the next level */
        level--;

        /* Termination condition - we have address of data block loaded */
        if (level == 0)
            break;

        /* Visit the next level */
        block_offset_in_level %= fs->inode_blocks_per_level[level];
        offset_in_block =
                block_offset_in_level / fs->inode_blocks_per_level[level - 1];
    }

    *fblock = current_block;

    return EOK;
}

int ext4_fs_set_inode_data_block_index(struct ext4_inode_ref *inode_ref,
    uint64_t iblock, uint32_t fblock)
{
    struct ext4_fs *fs = inode_ref->fs;

#if CONFIG_EXTENT_ENABLE
    /* Handle inode using extents */
    if ((ext4_sb_check_feature_incompatible(&fs->sb,
            EXT4_FEATURE_INCOMPAT_EXTENTS)) &&
            (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))) {
        /* Not reachable */
        return ENOTSUP;
    }
#endif

    /* Handle simple case when we are dealing with direct reference */
    if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
        ext4_inode_set_direct_block(inode_ref->inode, (uint32_t) iblock,
                fblock);
        inode_ref->dirty = true;

        return EOK;
    }

    /* Determine the indirection level needed to get the desired block */
    unsigned int level = 0;
    unsigned int i;
    for (i = 1; i < 4; i++) {
        if (iblock < fs->inode_block_limits[i]) {
            level = i;
            break;
        }
    }

    if (level == 0)
        return EIO;

    uint32_t block_size = ext4_sb_get_block_size(&fs->sb);

    /* Compute offsets for the topmost level */
    uint64_t block_offset_in_level =
            iblock - fs->inode_block_limits[level - 1];
    uint32_t current_block =
            ext4_inode_get_indirect_block(inode_ref->inode, level - 1);
    uint32_t offset_in_block =
            block_offset_in_level / fs->inode_blocks_per_level[level - 1];

    uint32_t new_block_addr;

    struct ext4_block block;
    struct ext4_block new_block;

    /* Is needed to allocate indirect block on the i-node level */
    if (current_block == 0) {
        /* Allocate new indirect block */
        int rc = ext4_balloc_alloc_block(inode_ref, &new_block_addr);
        if (rc != EOK)
            return rc;

        /* Update i-node */
        ext4_inode_set_indirect_block(inode_ref->inode, level - 1,
            new_block_addr);
        inode_ref->dirty = true;

        /* Load newly allocated block */
        rc = ext4_block_get(fs->bdev, &new_block, new_block_addr);
        if (rc != EOK) {
            ext4_balloc_free_block(inode_ref, new_block_addr);
            return rc;
        }

        /* Initialize new block */
        memset(new_block.data, 0, block_size);
        new_block.dirty = true;

        /* Put back the allocated block */
        rc = ext4_block_set(fs->bdev, &new_block);
        if (rc != EOK)
            return rc;

        current_block = new_block_addr;
    }

    /*
     * Navigate through other levels, until we find the block number
     * or find null reference meaning we are dealing with sparse file
     */
    while (level > 0) {
        int rc = ext4_block_get(fs->bdev, &block, current_block);
        if (rc != EOK)
            return rc;

        current_block =
                to_le32(((uint32_t *) block.data)[offset_in_block]);

        if ((level > 1) && (current_block == 0)) {
            /* Allocate new block */
            rc = ext4_balloc_alloc_block(inode_ref, &new_block_addr);
            if (rc != EOK) {
                ext4_block_set(fs->bdev, &block);
                return rc;
            }

            /* Load newly allocated block */
            rc = ext4_block_get(fs->bdev, &new_block, new_block_addr);

            if (rc != EOK) {
                ext4_block_set(fs->bdev, &block);
                return rc;
            }

            /* Initialize allocated block */
            memset(new_block.data, 0, block_size);
            new_block.dirty = true;

            rc = ext4_block_set(fs->bdev, &new_block);
            if (rc != EOK) {
                ext4_block_set(fs->bdev, &block);
                return rc;
            }

            /* Write block address to the parent */
            ((uint32_t *) block.data)[offset_in_block] =
                    to_le32(new_block_addr);
            block.dirty = true;
            current_block = new_block_addr;
        }

        /* Will be finished, write the fblock address */
        if (level == 1) {
            ((uint32_t *) block.data)[offset_in_block] =
                    to_le32(fblock);
            block.dirty = true;
        }

        rc = ext4_block_set(fs->bdev, &block);
        if (rc != EOK)
            return rc;

        level--;

        /*
         * If we are on the last level, break here as
         * there is no next level to visit
         */
        if (level == 0)
            break;

        /* Visit the next level */
        block_offset_in_level %= fs->inode_blocks_per_level[level];
        offset_in_block =
                block_offset_in_level / fs->inode_blocks_per_level[level - 1];
    }

    return EOK;
}

int ext4_fs_release_inode_block(struct ext4_inode_ref *inode_ref,
    uint32_t iblock)
{
    uint32_t fblock;

    struct ext4_fs *fs = inode_ref->fs;

    /* Extents are handled otherwise = there is not support in this function */
    ext4_assert(!(ext4_sb_check_feature_incompatible(&fs->sb,
            EXT4_FEATURE_INCOMPAT_EXTENTS) &&
            (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))));

    struct ext4_inode *inode = inode_ref->inode;

    /* Handle simple case when we are dealing with direct reference */
    if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
        fblock = ext4_inode_get_direct_block(inode, iblock);

        /* Sparse file */
        if (fblock == 0)
            return EOK;

        ext4_inode_set_direct_block(inode, iblock, 0);
        return ext4_balloc_free_block(inode_ref, fblock);
    }

    /* Determine the indirection level needed to get the desired block */
    unsigned int level = 0;
    unsigned int i;
    for (i = 1; i < 4; i++) {
        if (iblock < fs->inode_block_limits[i]) {
            level = i;
            break;
        }
    }

    if (level == 0)
        return EIO;

    /* Compute offsets for the topmost level */
    uint64_t block_offset_in_level =
            iblock - fs->inode_block_limits[level - 1];
    uint32_t current_block =
            ext4_inode_get_indirect_block(inode, level - 1);
    uint32_t offset_in_block =
            block_offset_in_level / fs->inode_blocks_per_level[level - 1];

    /*
     * Navigate through other levels, until we find the block number
     * or find null reference meaning we are dealing with sparse file
     */
    struct ext4_block block;

    while (level > 0) {

        /* Sparse check */
        if (current_block == 0)
            return EOK;

        int rc = ext4_block_get(fs->bdev, &block, current_block);
        if (rc != EOK)
            return rc;

        current_block =
                to_le32(((uint32_t *) block.data)[offset_in_block]);

        /* Set zero if physical data block address found */
        if (level == 1) {
            ((uint32_t *) block.data)[offset_in_block] =
                    to_le32(0);
            block.dirty = true;
        }

        rc = ext4_block_set(fs->bdev, &block);
        if (rc != EOK)
            return rc;

        level--;

        /*
         * If we are on the last level, break here as
         * there is no next level to visit
         */
        if (level == 0)
            break;

        /* Visit the next level */
        block_offset_in_level %= fs->inode_blocks_per_level[level];
        offset_in_block =
                block_offset_in_level / fs->inode_blocks_per_level[level - 1];
    }

    fblock = current_block;
    if (fblock == 0)
        return EOK;

    /* Physical block is not referenced, it can be released */
    return ext4_balloc_free_block(inode_ref, fblock);
}


int ext4_fs_append_inode_block(struct ext4_inode_ref *inode_ref,
    uint32_t *fblock, uint32_t *iblock)
{
#if CONFIG_EXTENT_ENABLE
    /* Handle extents separately */
    if ((ext4_sb_check_feature_incompatible(&inode_ref->fs->sb,
            EXT4_FEATURE_INCOMPAT_EXTENTS)) &&
            (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))){
        return ext4_extent_append_block(inode_ref, iblock, fblock, true);
    }
#endif
    struct ext4_sblock *sb = &inode_ref->fs->sb;

    /* Compute next block index and allocate data block */
    uint64_t inode_size = ext4_inode_get_size(sb, inode_ref->inode);
    uint32_t block_size = ext4_sb_get_block_size(sb);

    /* Align size i-node size */
    if ((inode_size % block_size) != 0)
        inode_size += block_size - (inode_size % block_size);

    /* Logical blocks are numbered from 0 */
    uint32_t new_block_idx = inode_size / block_size;

    /* Allocate new physical block */
    uint32_t phys_block;
    int rc = ext4_balloc_alloc_block(inode_ref, &phys_block);
    if (rc != EOK)
        return rc;

    /* Add physical block address to the i-node */
    rc = ext4_fs_set_inode_data_block_index(inode_ref,
            new_block_idx, phys_block);
    if (rc != EOK) {
        ext4_balloc_free_block(inode_ref, phys_block);
        return rc;
    }

    /* Update i-node */
    ext4_inode_set_size(inode_ref->inode, inode_size + block_size);
    inode_ref->dirty = true;

    *fblock = phys_block;
    *iblock = new_block_idx;

    return EOK;
}

/**
 * @}
 */

