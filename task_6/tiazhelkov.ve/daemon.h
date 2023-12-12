
#include <linux/limits.h>
int daemon_main(char* config_file);
void set_pid_file(char* filename);

struct Daemon {
    int sleep_seconds;
    int pid;
    char dump_dir[PATH_MAX];

    int inotify_fd;
    int inotify_size;
    int inotify_capacity;
    int* inotify_wds;
    char* config_file;
};

void monitor_dir(struct Daemon*, char* cwd, int fd);
void file_deleted  ();
void file_modified (char* cwd, char* dump_dir, char* filename);
void file_created (char* cwd, char* dump_dir, char* filename);

void add_watcher_recursive (const char* wd, struct Daemon* cfg);

