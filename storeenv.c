
#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>


#define MAX_PATH_LEN 4096
#define STORAGE "GP_STORAGE"

struct stat;

typedef int (*orig_open_f_type)(const char *pathname, int flags, mode_t mode);
typedef int (*orig_stat_f_type)(int ver, const char *pathname, struct stat *buf);
typedef int (*orig_lstat_f_type)(int ver, const char *pathname, struct stat *buf);
typedef int (*orig_access_f_type)(const char *pathname, int mode);

static void store_str(const char* content) {
    assert(getenv(STORAGE));
    FILE *f = fopen(getenv(STORAGE), "a");
    assert(f != 0);
    fprintf(f, "%s", content);
    fclose(f);
}

static int startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

static void store_pathname(const char* pathname) {
    char cwd[MAX_PATH_LEN];
    if (getcwd(cwd, MAX_PATH_LEN))
        assert("getcwd failed");
    if (pathname[0] != '/') {
        store_str(cwd);
        store_str("/");
    }
    store_str(pathname);
    store_str("\n");
}

int open(const char *pathname, int flags, mode_t mode) {
    orig_open_f_type orig;
    orig = (orig_open_f_type)dlsym(RTLD_NEXT, "open");
    
    if (strcmp(pathname, getenv(STORAGE)) == 0) {
        return orig(pathname, flags, mode);
    }
    
    if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname)) {
        return orig(pathname, flags, mode);
    }
    
    if (getenv(STORAGE)) {
        store_str("open ");
        store_pathname(pathname);
        return orig(pathname, flags, mode);
    } else {
        fprintf(stderr, "INVALID\n");
        exit(1);
    }
}

#ifdef __linux__ 
int __xstat(int ver, const char *pathname, struct stat *buf) {
#else
int xstat(int ver, const char *pathname, struct stat *buf) {
#endif
    orig_stat_f_type orig;
    orig = (orig_stat_f_type)dlsym(RTLD_NEXT, "__xstat");

    if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname)) {
        return orig(ver, pathname, buf);
    }
    
    if (getenv(STORAGE)) {
        store_str("stat ");
        store_pathname(pathname);
        return orig(ver, pathname, buf);
    } else {
        fprintf(stderr, "INVALID\n");
        exit(1);
    }
}


#ifdef __linux__ 
int __xlstat(int ver, const char *pathname, struct stat *buf) {
#else
int xlstat(int ver, const char *pathname, struct stat *buf) {
#endif
    orig_lstat_f_type orig;
    orig = (orig_lstat_f_type)dlsym(RTLD_NEXT, "__xlstat");

    if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname)) {
        return orig(ver, pathname, buf);
    }
    
    if (getenv(STORAGE)) {
        store_str("lstat ");
        store_pathname(pathname);
        return orig(ver, pathname, buf);
    } else {
        fprintf(stderr, "INVALID\n");
        exit(1);
    }
}


int access(const char *pathname, int mode) {
    orig_access_f_type orig;
    orig = (orig_access_f_type)dlsym(RTLD_NEXT, "access");

    if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname)) {
        return orig(pathname, mode);
    }
    
    if (getenv(STORAGE)) {
        store_str("access ");
        store_pathname(pathname);
        return orig(pathname, mode);
    } else {
        fprintf(stderr, "INVALID\n");
        exit(1);
    }
}

