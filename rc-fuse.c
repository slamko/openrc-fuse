#include <fcntl.h>
#include <stddef.h>
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <rc.h>
#include <fuse.h>

static RC_STRINGLIST *FRC_SERVICES;

#define ETC_INIT_D "/etc/init.d"
#define ETC_INIT_D_LEN sizeof(ETC_INIT_D)

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

        for (char *service = NULL; lookup_services(&service, &lookup_entry) == 0;) {   
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
    char *fname = NULL;

    for (token = strtok_r(context, delim, &sstate);
         (token);) {
        fname = token;
        token = strtok_r(NULL, delim, &sstate);
    }

    *service_name = strdup(fname);
    free(context);
    return !*service_name;
}

static int lookup_service_check(RC_STRINGLIST *service_list, const char *service_name) {
    struct rc_string *lookup_entry = service_list->tqh_first;
        
    for (char *service = NULL; lookup_services(&service, &lookup_entry) == 0;) {   
        printf("diff: %s : %s\n", service_name, service);
        if (strcmp(service_name, service) == 0)
            return 0;
    }
    
    return 1;
}

static int etc_init_append(char **buf, const char *append, size_t append_len, size_t buf_len) {
    if (buf == NULL || append == NULL || *buf == NULL)
        return 1;

    size_t plen = ETC_INIT_D_LEN + append_len;

    if (plen > buf_len)
        return 1;
    
    strcpy(*buf, ETC_INIT_D);
    strncat(*buf, append, buf_len);
    return !*buf;
}

int frc_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *finfo) {
    if (strcmp(path, "/") == 0) {
        return -1;
    }

    char *service = NULL;
    if (parse_service_name(path, &service)) {
        return -1;
    }

    if (lookup_service_check(FRC_SERVICES, service)){
        return -1;
    }
/*
    pid_t status_proc = fork();
    int statp[2];
    pipe(statp);
    
    if (status_proc == 0) {
        size_t sname_len = strlen(service);
        char *service_path = calloc(ETC_INIT_D_LEN + sname_len, sizeof(*service_path));
        
        close(statp[0]);
        dup2(STDOUT_FILENO, statp[1]);
        
        if (etc_init_append(&service_path, service, sname_len, ETC_INIT_D_LEN + sname_len)) {
            exit(1);
        }
        
        execl(service_path, "status", NULL);
        exit(1);
    } else if (status_proc == -1) {
        return -1;
    }

    close(statp[1]);
    read(statp[0], buffer, size);
    close(statp[0]);
*/
    memset(buffer, '\0', size);
    RC_SERVICE service_stat = rc_service_state(service);
    strncpy(buffer, (service_stat == RC_SERVICE_STARTED ? "STARTED\n" : "BROKEN\n"), size);
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

