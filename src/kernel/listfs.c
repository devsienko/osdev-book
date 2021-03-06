#include "stdlib.h"
#include "listfs.h"
#include "memory_manager.h"

uint32 first_file_sector_number = -1;

void init_list_fs (uint64 sector_number) { 
    first_file_sector_number = (uint32)sector_number;
}

listfs_file_header* get_file_info (uint32 sector_number) {
    listfs_file_header *first_file_header = (listfs_file_header*)flpydsk_read_sector(sector_number);
    return first_file_header;
}

listfs_file_header* get_first_file_info () {
    return get_file_info(first_file_sector_number);
}