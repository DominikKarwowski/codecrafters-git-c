#include "commit_tree.h"

#include <stdlib.h>
#include <unistd.h>

#include "debug_helpers.h"


struct commit_tree_opts
{
    char parent_sha[40];
    char *message;
};

static void destroy_commit_tree_opts(struct commit_tree_opts *commit_opts)
{
    if (!commit_opts) return;

    if (commit_opts->message) free(commit_opts->message);
    free(commit_opts);
}

static bool try_resolve_commit_tree_opts(const int argc, char *argv[], struct commit_tree_opts *commit_opts)
{
    opterr = 0;

    int opt;
    while ((opt = getopt(argc, argv, "p:m:")) != -1)
    {
        switch (opt)
        {
            case 'p':
                strncpy(commit_opts->parent_sha, optarg, 40);
                break;
            case 'm':
                const size_t commit_message_len = strlen(optarg);
                commit_opts->message = malloc(commit_message_len + 1);
                strncpy(commit_opts->message, optarg, commit_message_len);
                break;
            case '?':
                validate(false, "Invalid switch: '%c'\n", optopt);
            default:
                validate(false, "Unrecognized option: '%c'\n", optopt);
        }
    }

    return true;

error:
    return false;
}

int commit_tree(const int argc, char *argv[])
{
    const auto user_name = "git.user";
    const auto user_email = "git.user@email.com";

    char* tree_sha = argv[2];

    struct commit_tree_opts *commit_opts = malloc(sizeof(struct commit_tree_opts));

    bool opt_result = try_resolve_commit_tree_opts(argc, argv, commit_opts);
    validate(opt_result, "Failed to resolve options.");

    printf("user_name: %s\n", user_name);
    printf("user_email: %s\n", user_email);
    printf("tree_sha: %s\n", tree_sha);
    printf("parent_sha: %s\n", commit_opts->parent_sha);
    printf("commit message: %s\n", commit_opts->message);

    destroy_commit_tree_opts(commit_opts);

    return 0;

error:
    return 1;
}
