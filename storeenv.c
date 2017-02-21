
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
typedef int (*orig_stat_f_type)(const char *pathname, struct stat *buf);
typedef int (*orig_lstat_f_type)(const char *pathname, struct stat *buf);
typedef int (*orig_access_f_type)(const char *pathname, int mode);

static void store_str(const char* content) {
    assert(getenv(STORAGE));
    FILE *f = fopen(getenv(STORAGE), "a");
    assert(f != 0);
    fprintf(f, content);
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

static int isClangCC1() {
    FILE *file = fopen("/proc/self/cmdline", "r");
    int c;
    int index = 0;
    int lastWas0 = 1;

    if (file == 0) {
        fprintf(stderr, "Oh dear, something went wrong with read()! %s\n", strerror(errno));
        fprintf(stderr, "fopen(/proc/self/cmdline, 'r') failed\n");
        exit(1);
    }

    while ((c = fgetc(file)) != EOF)
    {
        if (c == '\0') {
            if (lastWas0) {
                fclose(file);
                return 0;
            }
            lastWas0 = 1;
        } else {
            lastWas0 = 0;
        }
        switch(index) {
        case 0:
            if (c == '-') {
                index++;
            }
            break;
        case 1:
            if (c == 'c') {
                index++;
            } else {
                index = 0;
            }
            break;
        case 2:
            if (c == 'c') {
                index++;
            } else {
                index = 0;
            }
            break;
        case 3:
            if (c == '1') {
                return 1;
                fclose(file);
            } else {
                index = 0;
            }
            break;
        default:
            break;
        }
    }
    return 0;
}

static void saveCmdLine() {
    if (access ("cmdline", F_OK ) != -1) {
    } else {
        FILE *outfile = fopen("cmdline", "a");
        assert(outfile != 0);
        
        if (outfile == 0) {
            fprintf(stderr, "fopen(cmdline, 'a') failed\n");
            exit(1);
        }
        
        FILE *infile = fopen("/proc/self/cmdline", "r");
        int c;
        
        if (infile == 0) {
            fprintf(stderr, "fopen(/proc/self/cmdline, 'r') failed\n");
            exit(1);
        }

        while ((c = fgetc(infile)) != EOF)
        {
            if (c == 0)
                fprintf(outfile, " ");
            else
                fprintf(outfile, "%c", c);
        }
        fprintf(outfile, "\n");
        
        fclose(infile);
        fclose(outfile);
    }
}

static int isClang() {
    char exe[1024];
    int ret;
    
    ret = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (ret ==-1) {
        fprintf(stderr, "readlink(/proc/self/exe, ...) failed\n");
        exit(1);
    }
    exe[ret] = 0;
    if (   exe[ret-1] > '0' && exe[ret-1] <= '9' &&
           exe[ret-2] == '.' &&
           exe[ret-3] > '0' && exe[ret-3] <= '9' &&
           exe[ret-4] == '-' &&
           exe[ret-5] == 'g' &&
           exe[ret-6] == 'n' &&
           exe[ret-7] == 'a' &&
           exe[ret-8] == 'l' &&
           exe[ret-9] == 'c') {
        if (isClangCC1()) {
            saveCmdLine();
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

int open(const char *pathname, int flags, mode_t mode) {
    orig_open_f_type orig;
    orig = (orig_open_f_type)dlsym(RTLD_NEXT, "open");
    
    if (strcmp(pathname, "cmdline") == 0) {
        return orig(pathname, flags, mode);
    }
    if (strcmp(pathname, getenv(STORAGE)) == 0) {
        return orig(pathname, flags, mode);
    }
    
    if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname)) {
        return orig(pathname, flags, mode);
    }
    if (!isClang()) {
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



int stat(const char *pathname, struct stat *buf) {
    orig_stat_f_type orig;
    orig = (orig_stat_f_type)dlsym(RTLD_NEXT, "stat");
    if (strcmp(pathname, "cmdline") == 0) {
        return orig(pathname, buf);
    }
    if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname)) {
        return orig(pathname, buf);
    }
    
    if (getenv(STORAGE)) {
        if (!isClang()) {
            return orig(pathname, buf);
        }
        store_str("stat ");
        store_pathname(pathname);
    } else {
        fprintf(stderr, "INVALID\n");
        exit(1);
    }
}


int lstat(const char *pathname, struct stat *buf) {
    orig_lstat_f_type orig;
    orig = (orig_lstat_f_type)dlsym(RTLD_NEXT, "lstat");
    if (strcmp(pathname, "cmdline") == 0) {
        return orig(pathname, buf);
    }
    if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname)) {
        return orig(pathname, buf);
    }
    
    if (getenv(STORAGE)) {
        if (!isClang()) {
            return orig(pathname, buf);
        }
        store_str("lstat ");
        store_pathname(pathname);
        return orig(pathname, buf);
    } else {
        fprintf(stderr, "INVALID\n");
        exit(1);
    }
}


int access(const char *pathname, int mode) {
    orig_access_f_type orig;
    orig = (orig_access_f_type)dlsym(RTLD_NEXT, "access");
    if (strcmp(pathname, "cmdline") == 0) {
        return orig(pathname, mode);
    }
    if (startsWith("/proc/", pathname) || startsWith("/dev/", pathname)) {
        return orig(pathname, mode);
    }
    
    if (getenv(STORAGE)) {
        if (!isClang()) {
            return orig(pathname, mode);
        }
        store_str("access ");
        store_pathname(pathname);
        return orig(pathname, mode);
    } else {
        fprintf(stderr, "INVALID\n");
        exit(1);
    }
}

