#include "../inc/functions.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>

extern pid_t        g_worker_pids[8];
extern int          g_num_workers;
extern int          g_pipes[8][2];
extern volatile sig_atomic_t g_sigusr1_count;
extern volatile sig_atomic_t g_worker_matches[8];
extern volatile sig_atomic_t g_worker_files[8];


int fill_subdirs(const char *root, char subdirs[][4096], int max)
{
    DIR *dir = opendir(root);
    if (!dir) return -1;

    struct dirent *entry;
    struct stat st;
    char full_path[4096];
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", root, entry->d_name);
        if (lstat(full_path, &st) == -1) continue;

        if (S_ISDIR(st.st_mode)) {
            strncpy(subdirs[count], full_path, 4095);
            subdirs[count][4095] = '\0';
            count++;
        }
    }
    closedir(dir);
    return count;
}

int check_num_subdirs(int num_subdirs, t_args *args)
{
    if (num_subdirs == 0) {
        printf("Notice: no subdirectories found; parent will search root directly.\n");
        int dummy = 0;
        search_directory(args->root_dir, args, &dummy);
        return -1;
    }
    if (num_subdirs < args->num_workers) {
        printf("Notice: only %d subdirectories found; using %d workers instead of %d.\n",
               num_subdirs, num_subdirs, args->num_workers);
        args->num_workers = num_subdirs;
    }
    return 1;
}

int create_workers(t_args *args)
{
    int num_subdirs = root_subdirs(args->root_dir);

    if (check_num_subdirs(num_subdirs, args) == -1)
        return 0;

    char subdirs[num_subdirs][4096];
    fill_subdirs(args->root_dir, subdirs, num_subdirs);

    g_num_workers = args->num_workers;

    setup_sigint_handler();
    setup_sigchld_handler();

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = sigusr1_handler;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    for (size_t i = 0; i < args->num_workers; i++)
    {
        pipe(g_pipes[i]);
        pipe(g_match_pipes[i]);
        g_worker_pids[i] = fork();

        if (g_worker_pids[i] == -1)
        {
            write(STDERR_FILENO, "ERROR: fork failed\n", 19);
            return -1;
        }

        if (g_worker_pids[i] == 0) // CHILD
        {
            struct sigaction sa_ign;
            sa_ign.sa_handler = SIG_IGN;
            sigemptyset(&sa_ign.sa_mask);
            sa_ign.sa_flags = 0;
            sigaction(SIGINT, &sa_ign, NULL);

            close(g_pipes[i][0]);
            close(g_match_pipes[i][0]);

            // Diğer worker'ların pipe'larını kapat
            for (size_t k = 0; k < args->num_workers; k++)
            {
                if (k != i)
                {
                    close(g_pipes[k][0]);
                    close(g_pipes[k][1]);
                    close(g_match_pipes[k][0]);
                    close(g_match_pipes[k][1]);
                }
            }

            setup_sigterm_handler();
            set_parent_pid(getppid());
            set_worker_index(i);

            int match_count   = 0;
            int files_scanned = 0;

            for (size_t j = i; j < num_subdirs; j += args->num_workers)
                match_count += search_directory(subdirs[j], args, &files_scanned);

            close(g_match_pipes[i][1]);

            int results[2] = {match_count, files_scanned};
            write(g_pipes[i][1], results, sizeof(results));
            close(g_pipes[i][1]);

            notify_parent();
            exit(match_count % 256);
        }

        // Parent: yazma uçlarını kapat
        close(g_pipes[i][1]);
        close(g_match_pipes[i][1]);
    }

    // PARENT: pipe'tan oku, match'leri topla
    t_match all_matches[1024];
    int     total_match_count = 0;

    for (size_t i = 0; i < g_num_workers; i++)
    {
        // Önce results pipe'tan oku
        int results[2] = {0, 0};
        read(g_pipes[i][0], results, sizeof(results));
        close(g_pipes[i][0]);

        g_worker_matches[i] = results[0];
        g_worker_files[i]   = results[1];

        // Match pipe'tan tüm match'leri oku
        t_match m;
        while (read(g_match_pipes[i][0], &m, sizeof(t_match)) == sizeof(t_match))
        {
            if (total_match_count < 1024)
                all_matches[total_match_count++] = m;
        }
        close(g_match_pipes[i][0]);

        waitpid(g_worker_pids[i], NULL, 0);
    }

    print_tree(args->root_dir, all_matches, total_match_count);
    print_summary();
    return 0;
}

int main(int argc, char **argv)
{
    t_args args;
    memset(&args, 0, sizeof(t_args));
    parse_arguments(argc, argv, &args);
    check_directory(args.root_dir);

    if (create_workers(&args) == -1)
        return 1;

    return 0;
}