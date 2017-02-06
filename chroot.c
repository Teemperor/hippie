#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#define MAX_PATH_LEN 4096
#define CHROOT_PATH "GP_CHROOT_PATH"

struct stat;

typedef int (*orig_open_f_type)(const char *pathname, int flags);
typedef int (*orig_stat_f_type)(const char *pathname, struct stat *buf);
typedef int (*orig_lstat_f_type)(const char *pathname, struct stat *buf);
typedef int (*orig_access_f_type)(const char *pathname, int mode);
 
int open(const char *pathname, int flags, ...)
{
    orig_open_f_type orig;
    orig = (orig_open_f_type)dlsym(RTLD_NEXT, "open");

    char chrootPath [MAX_PATH_LEN];
    strcpy(chrootPath, getenv(CHROOT_PATH));
    strncat(chrootPath, pathname, MAX_PATH_LEN);

    return orig(chrootPath, flags);
}

int stat(const char *pathname, struct stat *buf, ...)
{
    orig_stat_f_type orig;
    orig = (orig_stat_f_type)dlsym(RTLD_NEXT, "stat");

    char chrootPath [MAX_PATH_LEN];
    strcpy(chrootPath, getenv(CHROOT_PATH));
    strncat(chrootPath, pathname, MAX_PATH_LEN);

    return orig(chrootPath, buf);
}


int lstat(const char *pathname, struct stat *buf, ...)
{
    orig_lstat_f_type orig;
    orig = (orig_lstat_f_type)dlsym(RTLD_NEXT, "lstat");

    char chrootPath [MAX_PATH_LEN];
    strcpy(chrootPath, getenv(CHROOT_PATH));
    strncat(chrootPath, pathname, MAX_PATH_LEN);

    return orig(chrootPath, buf);
}


int access(const char *pathname, int mode, ...)
{
    orig_access_f_type orig;
    orig = (orig_access_f_type)dlsym(RTLD_NEXT, "access");

    char chrootPath [MAX_PATH_LEN];
    strcpy(chrootPath, getenv(CHROOT_PATH));
    strncat(chrootPath, pathname, MAX_PATH_LEN);

    return orig(chrootPath, mode);
}
