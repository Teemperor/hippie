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
typedef int (*orig_stat_f_type)(int ver, const char *pathname, struct stat *buf);
typedef int (*orig_lstat_f_type)(int ver, const char *pathname, struct stat *buf);
typedef int (*orig_access_f_type)(const char *pathname, int mode);
typedef char* (*orig_getcwd_f_type)(const char *buf, size_t size);
typedef int (*orig_mkdir_f_type)(const char *pathname, mode_t mode);
typedef int (*orig_symlink_f_type)(const char *target, const char *linkpath);
typedef int (*orig_rename_f_type)(const char *oldpath, const char *newpath);
typedef int (*orig_unlink_f_type)(const char *pathname);
 

static int startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

#define IF_ECXLUDE_PATH if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname) )
    // || startsWith("/usr/lib/", pathname) || startsWith("/usr/share/", pathname))

#define FILL_CHROOT_PATH(chrootPath, pathname) \
    if (pathname[0] != '/') { \
        strcpy(chrootPath, pathname); \
    } else { \
        strcpy(chrootPath, getenv(CHROOT_PATH)); \
        strncat(chrootPath, pathname, MAX_PATH_LEN); \
    }
    
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

int __xstat(int ver, const char *pathname, struct stat *buf, ...)
{
    orig_stat_f_type orig;
    orig = (orig_stat_f_type)dlsym(RTLD_NEXT, "__xstat");

    IF_ECXLUDE_PATH {
        return orig(ver, pathname, buf);
    }
    
    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, pathname);
    return orig(ver, chrootPath, buf);
}

int __lxstat(int ver, const char *pathname, struct stat *buf)
{
    orig_lstat_f_type orig;
    orig = (orig_lstat_f_type)dlsym(RTLD_NEXT, "__lxstat");

    IF_ECXLUDE_PATH {
        return orig(ver, pathname, buf);
    }
    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, pathname);
    return orig(ver, chrootPath, buf);
}

char *getcwd(char *buf, size_t size)
{
    orig_getcwd_f_type orig;
    orig = (orig_getcwd_f_type)dlsym(RTLD_NEXT, "getcwd");

    strncpy(buf, getenv(CHROOT_PATH), size);
    
    return orig(buf + strlen(getenv(CHROOT_PATH)), size - strlen(getenv(CHROOT_PATH)));
}

int mkdir(const char *pathname, mode_t mode)
{
    orig_mkdir_f_type orig;
    orig = (orig_mkdir_f_type)dlsym(RTLD_NEXT, "mkdir");

    IF_ECXLUDE_PATH {
        return orig(pathname, mode);
    }
    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, pathname);
    return orig(chrootPath, mode);
}

int symlink(const char *target, const char *linkpath)
{
    orig_symlink_f_type orig;
    orig = (orig_symlink_f_type)dlsym(RTLD_NEXT, "symlink");

    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, target);
    char chrootPath2 [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath2, linkpath);
    return orig(chrootPath, chrootPath2);
}

int rename(const char *oldpath, const char *newpath)
{
    orig_symlink_f_type orig;
    orig = (orig_symlink_f_type)dlsym(RTLD_NEXT, "rename");

    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, oldpath);
    char chrootPath2 [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath2, newpath);
    return orig(chrootPath, chrootPath2);
}

int remove(const char *pathname)
{
    orig_unlink_f_type orig;
    orig = (orig_unlink_f_type)dlsym(RTLD_NEXT, "remove");
    
    IF_ECXLUDE_PATH {
        return orig(pathname);
    }
    char chrootPath [MAX_PATH_LEN];
    FILL_CHROOT_PATH(chrootPath, pathname);
    return orig(chrootPath);
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
