#include <bits/types/siginfo_t.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include <assert.h>

#include "daemon.h"

#ifdef DEBUG
#define Dprintf(...) printf(__VA_ARGS__)
#else
#define Dprintf(...) ;
#endif

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))
static int CHANGE_CFG = 0;

void sig_handler(int sig, siginfo_t* info, void* context) {
    CHANGE_CFG = 1;
}

void daemon_init (struct Daemon* daemon, char* config_file) {
    daemon->inotify_fd = inotify_init();
    if (daemon->inotify_fd == -1) {
        perror("inotify_init");
        exit(0);
    }

    FILE* fileptr = fopen(config_file, "r");
    perror("Config");
    assert(fileptr != NULL);
    fscanf(fileptr, "%d %d %s", &daemon->pid, &daemon->sleep_seconds, daemon->dump_dir);
    fclose(fileptr);

    char cmd[PATH_MAX];
    sprintf(cmd, "mkdir %s", daemon->dump_dir);
    system(cmd);

    daemon->config_file = config_file;
    daemon->inotify_capacity = 100;
    daemon->inotify_size = 0;
    daemon->inotify_wds = (int*) calloc (daemon->inotify_capacity, sizeof(int));
}

void change_cfg (struct Daemon* daemon) {
    FILE* fileptr = fopen(daemon->config_file, "r");
    assert(fileptr != NULL);
    fscanf(fileptr, "%d %d %s", &daemon->pid, &daemon->sleep_seconds, daemon->dump_dir);
    fclose(fileptr);
    
}

int daemon_main(char* config_file) {
    struct Daemon daemon = {};
    daemon_init (&daemon, config_file);
    printf("Started\n");

    char proc_path[256];
    sprintf(proc_path, "/proc/%d", daemon.pid);

    // Check if the process exists
    if (access(proc_path, F_OK) == -1) {
        printf("Process with PID %d does not exist.\n", daemon.pid);
        return EXIT_FAILURE;
    }

    // Obtain the working directory of the process from /proc
    char cwd[PATH_MAX];
    sprintf(proc_path, "/proc/%d/cwd", daemon.pid);
    Dprintf("proc_path: %s\n", proc_path);

    ssize_t len = readlink(proc_path, cwd, sizeof(cwd) - 1);
    if (len == -1) {
        printf("Error reading process working directory.\n");
        return EXIT_FAILURE;
    }
    cwd[len] = '\0';

    add_watcher_recursive(cwd, &daemon);

    monitor_dir(&daemon, cwd, daemon.inotify_fd);
}

void add_watcher_recursive (const char* wd, struct Daemon* daemon) {
    DIR* dir = opendir(wd);

    if (dir) {
        int inotify_wd = inotify_add_watch(daemon->inotify_fd, wd, IN_MODIFY | IN_CREATE | IN_DELETE);

        daemon->inotify_wds[daemon->inotify_size] = inotify_wd;
        daemon->inotify_size++;

        char path[PATH_MAX];
        char* end_ptr = path;
        struct dirent* e;
        struct stat info;
        strcpy(path, wd);
        end_ptr += strlen(wd);
        sprintf(end_ptr, "/");
        end_ptr += 1;

        while((e = readdir(dir)) != NULL) {
            strcpy(end_ptr, e->d_name);

            printf("%s\n", path);
            if (!stat(path, &info)) {
                if (S_ISDIR(info.st_mode)) {
                    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
                        continue;
                    }
                    add_watcher_recursive(path, daemon);
                }
            }
        }
    }

    return;
}

void monitor_dir(struct Daemon* daemon, char* cwd, int fd) { // Main loop, mustn't return 
    Dprintf("HERE\n");
    while (1) {

        if (CHANGE_CFG) {
            change_cfg(daemon);
            CHANGE_CFG = 0;
        }

        sleep(daemon->sleep_seconds);

        char buf[BUF_LEN];
        int len = read(fd, buf, BUF_LEN);
        int i = 0;
        while (i < len) {

            struct inotify_event* ev = (struct inotify_event*) &buf[i];

            if (ev->mask & IN_CREATE) {
                printf ("FILE %s is created\n", ev->name);
                file_created (cwd, daemon->dump_dir, ev->name);
            }

            if (ev->mask & IN_MODIFY) {
                printf ("FILE %s is modified\n", ev->name);
                file_modified (cwd, daemon->dump_dir, ev->name);
            }

            if (ev->mask & IN_DELETE) 
                printf ("FILE %s is deleted\n", ev->name);

            i += EVENT_SIZE + ev->len;
        }
    }

}
void file_created (char* cwd, char* dump_dir, char* filename) {
    char buf[PATH_MAX];
    char filepath[PATH_MAX];
    sprintf(filepath, "%s/find_file", dump_dir);
    
    FILE* find_file = fopen(filepath, "w");
    fclose(find_file);
    chdir(cwd);
    sprintf(buf, "find -name %s > %s", filename, filepath);
    system(buf);

    find_file = fopen(filepath, "r");
    fscanf(find_file, "%s", filepath);
    fclose(find_file);

    sprintf(buf, "mkdir %s/%s", dump_dir, filename); 
    Dprintf("mkdir %s/%s", dump_dir, filename); 
    system(buf);

    sprintf(buf, "cp %s %s/%s", filepath, dump_dir, filename); 
    Dprintf("cp %s %s/%s", filepath, dump_dir, filename); 
    system(buf);
}

void file_modified (char* cwd, char* dump_dir, char* filename) {
    char buf[PATH_MAX];
    char filepath[PATH_MAX];

    sprintf(filepath, "%s/find_file", dump_dir);
    
    FILE* find_file = fopen(filepath, "w");
    fclose(find_file);
    chdir(cwd);
    sprintf(buf, "find -name %s > %s", filename, filepath);
    system(buf);

    find_file = fopen(filepath, "r");
    fscanf(find_file, "%s", filepath);
    fclose(find_file);

    time_t t = time(NULL);

    struct tm tm = *localtime(&t);
    char diffFilename[100];
    sprintf(diffFilename, "%s_%d-%02d-%02d_%02d-%02d-%02d.diff", filename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    sprintf (buf, "diff %s %s/%s/%s > %s/%s/%s", filepath, dump_dir, filename, filename, dump_dir, filename, diffFilename);
    system(buf);

    sprintf(buf, "cp %s %s/%s", filepath, dump_dir, filename); 
    Dprintf("cp %s %s/%s", filepath, dump_dir, filename); 
    system(buf);

}

void set_pid_file(char* filename) {
    FILE* pid_file;

    pid_file = fopen(filename, "w+");
    if (pid_file)
    {
        fprintf(pid_file, "%u", getpid());
        fclose(pid_file);
    }
}

