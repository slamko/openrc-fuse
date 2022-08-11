#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <rc.h>
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

static int lookup_services(char **service_name, struct rc_string **entry) {
    struct rc_string *service = (entry ? *entry : FRC_SERVICES->tqh_first);
    if (service) {
        *service_name = service->value;
        *entry = service->entries.tqe_next;
        return 0;
    }
    
    return 1;
}

int frc_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *finfo) {
    if (strcmp(path, "/") == 0) {
        struct rc_string *lookup_entry = FRC_SERVICES->tqh_first;

        for (char *service = NULL; lookup_services(&service, &lookup_entry);) {   
            filler(buffer, service, NULL, 0);
        }

        return 0;
    }
    return 1;
}

static int parse_service_name(const char *path, char **service_name) {
    char delim[] = "/";
    char *token = NULL;
    char *context = strdup(path);
    char *sstate = NULL;

    for (token = strtok_r(context, delim, &sstate);
         (token);) {
        *service_name = token;
        token = strtok_r(NULL, delim, &sstate);
    }

    free(context);
    return !*service_name;
}

int frc_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *finfo) {
    if (strcmp(path, "/") == 0) {
        return -1;
    }

    char *service = NULL;
    if (parse_service_name(path, &service)) {
        return -1;
    }

    
    
    return size;
}

struct fuse_operations frc_ops = {
    .getattr = frc_getattr,
    .readdir = frc_readdir,
    .read = frc_read,
};

int main(int argc, char **argv) {
    FRC_SERVICES = rc_services_in_state(RC_SERVICE_STARTED);
    return fuse_main(argc, argv, &frc_ops, NULL);
}

