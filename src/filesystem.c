#include "filesystem.h"
#include "kernel.h"
#include "std_lib.h"
#include "std_type.h"

struct map_fs __fs_map;
struct node_fs __fs_nodes;
struct data_fs __fs_data;

void fsInit(void) {
    int i;
    readSector(&__fs_map, FS_MAP_SECTOR_NUMBER);

    if (!__fs_map.is_used[0]) {
        for (i = 0; i < 16; i++) {
            __fs_map.is_used[i] = true;
        }
        for (i = 256; i < SECTOR_SIZE; i++) {
            __fs_map.is_used[i] = true;
        }
        writeSector(&__fs_map, FS_MAP_SECTOR_NUMBER);
    }
    
    readSector(&__fs_nodes.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector(&__fs_nodes.nodes[FS_MAX_NODE / FS_NODE_SECTOR_COUNT], FS_NODE_SECTOR_NUMBER + 1);
    readSector(&__fs_data, FS_DATA_SECTOR_NUMBER);
}

void fsRead(struct file_metadata* metadata, enum fs_return* status) {
    int i, j, data_idx;

    clear(metadata->buffer, FS_MAX_SECTOR * SECTOR_SIZE);

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (__fs_nodes.nodes[i].parent_index == metadata->parent_index) {
            if (strcmp(__fs_nodes.nodes[i].node_name, metadata->node_name) == 0) {
                if (__fs_nodes.nodes[i].data_index == FS_NODE_D_DIR) {
                    *status = FS_R_TYPE_IS_DIRECTORY;
                    return;
                }

                metadata->filesize = 0;
                data_idx = __fs_nodes.nodes[i].data_index;

                if (data_idx >= FS_MAX_DATA || data_idx < 0) {
                    *status = FS_R_NODE_NOT_FOUND;
                    return;
                }

                for (j = 0; j < FS_MAX_SECTOR; j++) {
                    if (__fs_data.datas[data_idx].sectors[j] == 0x00) {
                        break;
                    }
                    readSector(metadata->buffer + j * SECTOR_SIZE, __fs_data.datas[data_idx].sectors[j]);
                    metadata->filesize += SECTOR_SIZE;
                }
                *status = FS_R_SUCCESS;
                return;
            }
        }
    }
    *status = FS_R_NODE_NOT_FOUND;
}

void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
    int i, j;
    int empt_node_idx = -1;
    int empt_data_idx = -1;
    int empt_blocks_count = 0;
    int sector_blocks;
    byte sector_idx[FS_MAX_SECTOR];
    clear(sector_idx, FS_MAX_SECTOR); // Ganti inisialisasi otomatis

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (strcmp(__fs_nodes.nodes[i].node_name, metadata->node_name) == 0 &&
            __fs_nodes.nodes[i].parent_index == metadata->parent_index) {
            *status = FS_W_NODE_ALREADY_EXISTS;
            return;
        }
    }

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (__fs_nodes.nodes[i].node_name[0] == '\0') {
            empt_node_idx = i;
            break;
        }
    }

    if (empt_node_idx == -1) {
        *status = FS_W_NO_FREE_NODE;
        return;
    }

    for (i = 0; i < FS_MAX_DATA; i++) {
        if (__fs_data.datas[i].sectors[0] == 0x00) {
            empt_data_idx = i;
            break;
        }
    }

    if (empt_data_idx == -1) {
        *status = FS_W_NO_FREE_DATA;
        return;
    }

    sector_blocks = (metadata->filesize + SECTOR_SIZE - 1) / SECTOR_SIZE;

    if (metadata->filesize == 0) {
        sector_blocks = 0;
    }
    
    empt_blocks_count = 0; 
    for (i = 0; i < SECTOR_SIZE; i++) { 
        if (!__fs_map.is_used[i] && i <= 0xFF) {
            if (empt_blocks_count < sector_blocks) {
                sector_idx[empt_blocks_count] = i;
            }
            empt_blocks_count++;
        }
    }

    if (empt_blocks_count < sector_blocks) {
        *status = FS_W_NOT_ENOUGH_SPACE;
        return;
    }

    strcpy(__fs_nodes.nodes[empt_node_idx].node_name, metadata->node_name);
    __fs_nodes.nodes[empt_node_idx].parent_index = metadata->parent_index;
    
    if (metadata->filesize == 0) {
        __fs_nodes.nodes[empt_node_idx].data_index = FS_NODE_D_DIR;
    } else {
        __fs_nodes.nodes[empt_node_idx].data_index = empt_data_idx;
    }

    if (metadata->filesize > 0) {
        for (i = 0; i < sector_blocks; i++) {
            __fs_map.is_used[sector_idx[i]] = true;
            __fs_data.datas[empt_data_idx].sectors[i] = sector_idx[i];
            writeSector(metadata->buffer + i * SECTOR_SIZE, sector_idx[i]);
        }
        for (i = sector_blocks; i < FS_MAX_SECTOR; i++) {
            __fs_data.datas[empt_data_idx].sectors[i] = 0x00;
        }
    }

    writeSector(&__fs_map, FS_MAP_SECTOR_NUMBER);
    writeSector(&__fs_nodes.nodes[0], FS_NODE_SECTOR_NUMBER);
    writeSector(&__fs_nodes.nodes[FS_MAX_NODE / FS_NODE_SECTOR_COUNT], FS_NODE_SECTOR_NUMBER + 1);
    writeSector(&__fs_data, FS_DATA_SECTOR_NUMBER);

    *status = FS_W_SUCCESS;
}