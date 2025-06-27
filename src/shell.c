#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

extern struct node_fs __fs_nodes;
extern struct map_fs __fs_map;
extern struct data_fs __fs_data;

void shell() {
    char buf[64];
    char cmd[64];
    char arg[2][64];

    byte cwd = FS_NODE_P_IDX_ROOT;

    while (true) {
        printString("MengOS:");
        printCWD(cwd);
        printString("$ ");
        readString(buf);
        parseCommand(buf, cmd, arg);

        if (strcmp(cmd, "cd") == 0) cd(&cwd, arg[0]);
        else if (strcmp(cmd, "ls") == 0) ls(cwd, arg[0]);
        else if (strcmp(cmd, "mv") == 0) mv(cwd, arg[0], arg[1]);
        else if (strcmp(cmd, "cp") == 0) cp(cwd, arg[0], arg[1]);
        else if (strcmp(cmd, "cat") == 0) cat(cwd, arg[0]);
        else if (strcmp(cmd, "mkdir") == 0) mkdir(cwd, arg[0]);
        else if (strcmp(cmd, "clear") == 0) clearScreen();
        else printString("Invalid command\n");
    }
}

// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
    int i;
    int count;
    byte current;
    char path[FS_MAX_NODE][MAX_FILENAME];

    if (cwd == FS_NODE_P_IDX_ROOT) {
        printString("/\n");
        return;
    }

    count = 0;
    current = cwd;
    while (current != FS_NODE_P_IDX_ROOT) {
        strcpy(path[count], __fs_nodes.nodes[current].node_name);
        count++;
        current = __fs_nodes.nodes[current].parent_index;
        if (count >= FS_MAX_NODE) {
            printString("/[ERROR_PATH]/");
            break;
        }
    }

    printString("/");
    for (i = count - 1; i >= 0; i--) {
        printString(path[i]);
        if (i > 0) printString("/");
    }
    printString("\n");
}

// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int i = 0;
    int word = 0;
    
    clear((byte*)cmd, 64);
    clear((byte*)arg[0], 64);
    clear((byte*)arg[1], 64);

    while (*buf != '\0' && *buf != '\n') {
        if (*buf == ' ') {
            if (word == 0) {
                cmd[i] = '\0';
            } else if (word <= 2) {
                arg[word - 1][i] = '\0';
            }
            word++;
            i = 0;
            while (*(buf + 1) == ' ') {
                buf++;
            }
        } else {
            if (word == 0) {
                if (i < 63) cmd[i++] = *buf;
            } else if (word == 1) {
                if (i < 63) arg[0][i++] = *buf;
            } else if (word == 2) {
                if (i < 63) arg[1][i++] = *buf;
            }
        }
        buf++;
    }
    
    if (word == 0) {
        cmd[i] = '\0';
    } else if (word == 1) {
        arg[0][i] = '\0';
    } else if (word == 2) {
        arg[1][i] = '\0';
    }
}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
    if (strcmp(dirname, "/") == 0) {
        *cwd = FS_NODE_P_IDX_ROOT;
        return;
    } else if (strcmp(dirname, "..") == 0) {
        if (*cwd == FS_NODE_P_IDX_ROOT) {
             printString("Already at root directory\n");
             return;
        }
        *cwd = __fs_nodes.nodes[*cwd].parent_index;
        return;
    } else {
        int i;
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (__fs_nodes.nodes[i].parent_index == *cwd &&
                strcmp(__fs_nodes.nodes[i].node_name, dirname) == 0 &&
                __fs_nodes.nodes[i].data_index == FS_NODE_D_DIR) {
                *cwd = i;
                return;
            }
        }
    }
    printString("Directory not found\n");
}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
    int i;
    byte target_node_idx = cwd;

    if (strcmp(dirname, ".") != 0 && strlen(dirname) > 0) {
        int found_target = 0;
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (__fs_nodes.nodes[i].parent_index == cwd &&
                strcmp(__fs_nodes.nodes[i].node_name, dirname) == 0) {
                if (__fs_nodes.nodes[i].data_index == FS_NODE_D_DIR) {
                    target_node_idx = i;
                    found_target = 1;
                    break;
                } else {
                    printString("ls: Not a directory\n");
                    return;
                }
            }
        }
        if (!found_target) {
            printString("ls: Directory not found\n");
            return;
        }
    }

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (__fs_nodes.nodes[i].parent_index == target_node_idx) {
            printString(__fs_nodes.nodes[i].node_name);
            if (__fs_nodes.nodes[i].data_index == FS_NODE_D_DIR) {
                printString("/");
            }
            printString("\n");
        }
    }
}

// TODO: 8. Implement mv function
void mv(byte cwd, char* src_name, char* dst_path) {
    int i, j;
    int src_node_idx = -1;
    byte new_parent_index = cwd;
    char target_name[MAX_FILENAME];
    enum fs_return status;
    struct file_metadata target_dir_meta;

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (__fs_nodes.nodes[i].parent_index == cwd && strcmp(__fs_nodes.nodes[i].node_name, src_name) == 0) {
            src_node_idx = i;
            break;
        }
    }

    if (src_node_idx == -1) {
        printString("Source file/directory not found\n");
        return;
    }

    if (__fs_nodes.nodes[src_node_idx].data_index == FS_NODE_D_DIR) {
        printString("mv: cannot move a directory\n");
        return;
    }

    int dst_len = strlen(dst_path);
    int last_slash = -1;
    for (i = 0; i < dst_len; i++) {
        if (dst_path[i] == '/') {
            last_slash = i;
        }
    }

    if (last_slash == -1) {
        strcpy(target_name, dst_path);
    } else {
        char path_only[64];
        
        strncpy(path_only, dst_path, last_slash);
        path_only[last_slash] = '\0';
        strcpy(target_name, dst_path + last_slash + 1);

        if (strcmp(path_only, "..") == 0) {
            if (cwd == FS_NODE_P_IDX_ROOT) {
                printString("mv: cannot move to parent of root\n");
                return;
            }
            new_parent_index = __fs_nodes.nodes[cwd].parent_index;
        } else if (path_only[0] == '\0' && last_slash == 0) {
            new_parent_index = FS_NODE_P_IDX_ROOT;
        } else {
            byte current_search_parent = cwd;
            if (path_only[0] == '/') {
                current_search_parent = FS_NODE_P_IDX_ROOT;
                if (strlen(path_only) > 1) {
                    strcpy(path_only, path_only + 1);
                } else {
                    new_parent_index = FS_NODE_P_IDX_ROOT;
                    goto check_target_name_exists;
                }
            }

            char temp_path_segment[MAX_FILENAME];
            int seg_start = 0;
            for (j = 0; j <= strlen(path_only); j++) {
                if (path_only[j] == '/' || path_only[j] == '\0') {
                    if (j - seg_start > 0) {
                        strncpy(temp_path_segment, path_only + seg_start, j - seg_start);
                        temp_path_segment[j - seg_start] = '\0';

                        int found_dir = 0;
                        for (i = 0; i < FS_MAX_NODE; i++) {
                            if (__fs_nodes.nodes[i].parent_index == current_search_parent &&
                                strcmp(__fs_nodes.nodes[i].node_name, temp_path_segment) == 0 &&
                                __fs_nodes.nodes[i].data_index == FS_NODE_D_DIR) {
                                current_search_parent = i;
                                found_dir = 1;
                                break;
                            }
                        }
                        if (!found_dir) {
                            printString("mv: Destination directory not found in path\n");
                            return;
                        }
                    }
                    seg_start = j + 1;
                }
            }
            new_parent_index = current_search_parent;
        }
    }

check_target_name_exists:;

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (__fs_nodes.nodes[i].parent_index == new_parent_index && strcmp(__fs_nodes.nodes[i].node_name, target_name) == 0) {
            printString("mv: Destination file/directory already exists\n");
            return;
        }
    }

    __fs_nodes.nodes[src_node_idx].parent_index = new_parent_index;
    strcpy(__fs_nodes.nodes[src_node_idx].node_name, target_name);
    
    writeSector(&__fs_nodes.nodes[0], FS_NODE_SECTOR_NUMBER);
    writeSector(&__fs_nodes.nodes[FS_MAX_NODE / FS_NODE_SECTOR_COUNT], FS_NODE_SECTOR_NUMBER + 1);

    printString("File/directory moved successfully.\n");
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
    struct file_metadata src_meta;
    struct file_metadata dst_meta;
    enum fs_return status;
    byte new_parent_index = cwd;
    char target_name[MAX_FILENAME];
    int i;

    // Parsing dst
    int dst_len = strlen(dst);
    int last_slash = -1;
    for (i = 0; i < dst_len; i++) {
        if (dst[i] == '/') last_slash = i;
    }

    if (last_slash == -1) {
        strcpy(target_name, dst);
    } else {
        char path_only[64];
        strncpy(path_only, dst, last_slash);
        path_only[last_slash] = '\0';
        strcpy(target_name, dst + last_slash + 1);

        if (strcmp(path_only, "..") == 0) {
            if (cwd == FS_NODE_P_IDX_ROOT) {
                printString("cp: cannot copy to parent of root\n");
                return;
            }
            new_parent_index = __fs_nodes.nodes[cwd].parent_index;
        } else if (strcmp(path_only, "") == 0) {
            new_parent_index = FS_NODE_P_IDX_ROOT;
        } else {
            for (i = 0; i < FS_MAX_NODE; i++) {
                if (__fs_nodes.nodes[i].parent_index == cwd &&
                    strcmp(__fs_nodes.nodes[i].node_name, path_only) == 0 &&
                    __fs_nodes.nodes[i].data_index == FS_NODE_D_DIR) {
                    new_parent_index = i;
                    break;
                }
            }
            if (i == FS_MAX_NODE) {
                printString("cp: Destination directory not found\n");
                return;
            }
        }
    }

    // Baca file sumber
    strcpy(src_meta.node_name, src);
    src_meta.parent_index = cwd;
    fsRead(&src_meta, &status);

    if (status != FS_R_SUCCESS) {
        if (status == FS_R_TYPE_IS_DIRECTORY) {
            printString("cp: cannot copy a directory\n");
        } else {
            printString("cp: Source file not found\n");
        }
        return;
    }

    // Siapkan metadata tujuan
    dst_meta.parent_index = new_parent_index;
    dst_meta.filesize = src_meta.filesize;
    strcpy(dst_meta.node_name, target_name);
    memcpy(dst_meta.buffer, src_meta.buffer, src_meta.filesize);

    fsWrite(&dst_meta, &status);

    if (status == FS_W_SUCCESS) {
        printString("File copied successfully.\n");
    } else if (status == FS_W_NODE_ALREADY_EXISTS) {
        printString("cp: Destination file already exists\n");
    } else if (status == FS_W_NO_FREE_NODE || status == FS_W_NO_FREE_DATA || status == FS_W_NOT_ENOUGH_SPACE) {
        printString("cp: Not enough space or free nodes/data items\n");
    } else {
        printString("cp: Unknown error during copy\n");
    }
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
    struct file_metadata meta;
    enum fs_return status;
    int i;

    meta.parent_index = cwd;
    strcpy(meta.node_name, filename);
    fsRead(&meta, &status);

    if (status == FS_R_SUCCESS) {
        for (i = 0; i < meta.filesize; i++) {
            interrupt(0x10, 0x0E00 + meta.buffer[i], 0, 0, 0);
        }
        printString("\n");
    } else if (status == FS_R_TYPE_IS_DIRECTORY) {
        printString("cat: Is a directory\n");
    } else {
        printString("cat: File not found\n");
    }
}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct file_metadata meta;
    enum fs_return status;

    meta.parent_index = cwd;
    meta.filesize = 0;
    strcpy(meta.node_name, dirname);
    fsWrite(&meta, &status);

    if (status == FS_W_SUCCESS) {
        printString("Directory created successfully.\n");
    } else if (status == FS_W_NODE_ALREADY_EXISTS) {
        printString("mkdir: Directory already exists\n");
    } else if (status == FS_W_NO_FREE_NODE) {
        printString("mkdir: No free node available\n");
    } else {
        printString("mkdir: Failed to create directory\n");
    }
}