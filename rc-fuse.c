#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <rc.h>
#define FUSE_USE_VERSION 26
#include <fuse.h>

static RC_STRINGLIST *FRC_SERVICES;

int frc_getattr(const char *path, struct stat *st) {
    st->st_uid = getuid(); 
	st->st_gid = getgid(); 
	st->st_atime = time(NULL); 
	st->st_mtime = time(NULL); 
	
	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	
	return 0;
}

int frc_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *finfo) {
    if (strcmp(path, "/") == 0) {
        printf("\nhi%p\n", (void *)FRC_SERVICES);
        for (struct rc_string *service = FRC_SERVICES->tqh_first;
             service;
             service = service->entries.tqe_next) {
            filler(buffer, service->value, NULL, 0);
        }
    }
    return 0;
}

struct fuse_operations frc_ops = {
    .getattr = frc_getattr,
    .readdir = frc_readdir, 
};

int main(int argc, char **argv) {
    FRC_SERVICES = rc_services_in_state(RC_SERVICE_STARTED);
    return fuse_main(argc, argv, &frc_ops, NULL);
}

