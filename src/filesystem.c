void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
    struct map_fs map_fs_buf;
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;

    int i, j, k = 0;
    int empt_node_idx = -1;
    int empt_data_idx = -1;
    int empt_blocks_count = 0;
    int sector_blocks;
    byte sector_idx[FS_MAX_SECTOR];

    // 1. Membaca filesystem dari disk ke memory
    readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
    readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    /* 2. Lakukan iterasi setiap item node untuk mencari node yang memiliki nama yang sama dengan metadata->node_name
          dan parent index yang sama dengan metadata->parent_index. Jika node yang dicari ditemukan, maka set status
          dengan FS_R_NODE_ALREADY_EXISTS dan keluar */
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 &&
            node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
            *status = FS_W_NODE_ALREADY_EXISTS;
            return;
        }
    }

    /* 3. Selanjutnya, cari node yang kosong (nama node adalah string kosong) dan simpan index-nya. Jika node yang
          kosong tidak ditemukan, maka set status dengan FS_W_NO_FREE_NODE dan keluar */
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] == '\0') {
            empt_node_idx = i;
            break;
        }
    }

    if (empt_node_idx == -1) {
        *status = FS_W_NO_FREE_NODE;
        return;
    }

    /* 4. Iterasi setiap item data untuk mencari data yang kosong (alamat sektor data ke-0 adalah 0x00) dan simpan
          index-nya. Jika data yang kosong tidak ditemukan, maka set status dengan FS_W_NO_FREE_DATA dan keluar */
    for (i = 0; i < FS_MAX_DATA; i++) {
        if (data_fs_buf.datas[i].sectors[0] == 0x00) {
            empt_data_idx = i;
            break;
        }
    }

    if (empt_data_idx == -1) {
        *status = FS_W_NO_FREE_DATA;
        return;
    }

    /* 5. Iterasi setiap item map dan hitung blok yang kosong (status blok adalah 0x00 atau false). Jika blok yang
         kosong kurang dari metadata->filesize / SECTOR_SIZE, maka set status dengan FS_W_NOT_ENOUGH_SPACE dan keluar */
    sector_blocks = (metadata->filesize + SECTOR_SIZE - 1) / SECTOR_SIZE;

    for (i = 0, j = 0; i < SECTOR_SIZE && j < sector_blocks; i++) {
        if (!map_fs_buf.is_used[i]) {
            sector_idx[j++] = i;
            empt_blocks_count++;
        }
    }

    if (empt_blocks_count < sector_blocks) {
        *status = FS_W_NOT_ENOUGH_SPACE;
        return;
    }

    /* 6. Set nama dari node yang ditemukan dengan metadata->node_name, parent index dengan metadata->parent_index,
          dan data index dengan index data yang kosong */
    strcpy(node_fs_buf.nodes[empt_node_idx].node_name, metadata->node_name);
    node_fs_buf.nodes[empt_node_idx].parent_index = metadata->parent_index;
    node_fs_buf.nodes[empt_node_idx].data_index = empt_data_idx;

    // 7. Lakukan penulisan data dengan cara sebagai berikut
    // -- Lakukan iterasi i dari 0 hingga SECTOR_SIZE
    for (i = 0; i < sector_blocks; i++) {
        /* -- Jika item map pada index ke-i adalah 0x00, maka tulis index i ke dalam data item sektor ke-j dan tulis
              data dari buffer ke dalam sektor ke-i */
        data_fs_buf.datas[empt_data_idx].sectors[k] = sector_idx[i];
        // -- Penulisan dapat menggunakan fungsi writeSector dari metadata->buffer + i * SECTOR_SIZE
        writeSector(metadata->buffer + k * SECTOR_SIZE, sector_idx[i]);
        // Tambahkan 1 ke j (jika disini adalah K)
        k++;
    }

    // 8. Tulis kembali filesystem yang telah diubah ke dalam disk
    writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
    writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // 9. Set status dengan FS_W_SUCCESS
    *status = FS_SUCCESS;
}
