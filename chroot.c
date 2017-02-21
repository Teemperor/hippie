#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define MAX_PATH_LEN 4096
#define CHROOT_PATH "GP_CHROOT_PATH"

struct stat;

typedef int (*orig_open_f_type)(const char *pathname, int flags, mode_t mode);
typedef int (*orig_stat_f_type)(const char *pathname, struct stat *buf);
typedef int (*orig_lstat_f_type)(const char *pathname, struct stat *buf);
typedef int (*orig_access_f_type)(const char *pathname, int mode);
 

static int startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

#define IF_ECXLUDE_PATH if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname) \
     || startsWith("/usr/lib/", pathname) || startsWith("/usr/share/", pathname))

#define FILL_CHROOT_PATH(chrootPath, pathname) \
    if (pathname[0] != '/') { \
        if (getcwd(chrootPath, MAX_PATH_LEN) == 0) { \
            assert(0 && "getcwd failed"); \
        } \
        strncat(chrootPath, "////", MAX_PATH_LEN); \
    } else { \
        strcpy(chrootPath, getenv(CHROOT_PATH)); \
    } \
    strncat(chrootPath, pathname, MAX_PATH_LEN);
    
int open(const char *pathname, int flags, mode_t mode)
{
    assert(getenv(CHROOT_PATH));

    orig_open_f_type orig;
    orig = (orig_open_f_type)dlsym(RTLD_NEXT, "open");
    
    IF_ECXLUDE_PATH {
        return orig(pathname, flags, mode);
    }
    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, pathname);
    return orig(chrootPath, flags, mode);
}

int stat(const char *pathname, struct stat *buf, ...)
{
    orig_stat_f_type orig;
    orig = (orig_stat_f_type)dlsym(RTLD_NEXT, "stat");

    IF_ECXLUDE_PATH {
        return orig(pathname, buf);
    }
    
    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, pathname);
    return orig(chrootPath, buf);
}


int lstat(const char *pathname, struct stat *buf, ...)
{
    orig_lstat_f_type orig;
    orig = (orig_lstat_f_type)dlsym(RTLD_NEXT, "lstat");

    IF_ECXLUDE_PATH {
        return orig(pathname, buf);
    }
    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, pathname);
    return orig(chrootPath, buf);
}


int access(const char *pathname, int mode)
{
    orig_access_f_type orig;
    orig = (orig_access_f_type)dlsym(RTLD_NEXT, "access");

    IF_ECXLUDE_PATH {
        return orig(pathname, mode);
    }
    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, pathname);
    return orig(chrootPath, mode);
}
