#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
  struct map_fs map_fs_buf;
  int i = 0;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
  for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i, j;

  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 &&
        node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
      if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
        *status = FS_R_TYPE_IS_DIRECTORY;
      } else {
        metadata->filesize = 0;
        for (j = 0; j < FS_MAX_SECTOR; j++) {
          if (data_fs_buf.datas[i].sectors[j] == 0x00) break;

          readSector(&(data_fs_buf.datas[i].sectors[j]), (metadata->buffer + j * SECTOR_SIZE));
          metadata->filesize = SECTOR_SIZE;
        }
      *status = FS_SUCCESS;
      }
    } else {
      *status = FS_R_NODE_NOT_FOUND;
    }
  }
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
  struct map_fs map_fs_buf;
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;
}
