#include "commit_tree.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "debug_helpers.h"
#include "git_obj_helpers.h"

static bool try_resolve_commit_tree_opts(const int argc, char *argv[], commit_info *commit_opts)
{
    validate(commit_opts, "Called with null commit_opts.");

    opterr = 0;

    int opt;
    while ((opt = getopt(argc, argv, "p:m:")) != -1)
    {
        switch (opt)
        {
            case 'p':
                commit_opts->parent_sha = malloc(SHA_HEX_LENGTH);
                validate(commit_opts->parent_sha, "Failed to allocate memory.");
                strncpy(commit_opts->parent_sha, optarg, SHA_HEX_LENGTH);
                break;
            case 'm':
                const size_t commit_message_len = strlen(optarg);
                commit_opts->message = malloc(commit_message_len + 1);
                validate(commit_opts->message, "Failed to allocate memory.");
                strncpy(commit_opts->message, optarg, commit_message_len);
                break;
            case '?':
                validate(false, "Invalid switch: '%c'\n", optopt);
            default:
                validate(false, "Unrecognized option: '%c'\n", optopt);
        }
    }

    validate(commit_opts->message, "No commit message provided.");

    return true;

error:
    return false;
}

int commit_tree(const int argc, char *argv[])
{
    commit_info commit_info;
    init_commit_tree_info(&commit_info);

    const auto user_name = "git.user";
    const auto user_email = "git.user@email.com";

    commit_info.author_name = strdup(user_name);
    commit_info.author_email = strdup(user_email);

    commit_info.committer_name = strdup(user_name);
    commit_info.committer_email = strdup(user_email);

    const time_t current_time = time(nullptr);
    validate(current_time != -1, "Failed to get current time.");

    commit_info.author_date = malloc(32);
    commit_info.committer_date = malloc(32);

    sprintf(commit_info.author_date, "%ld", current_time);
    sprintf(commit_info.committer_date, "%ld", current_time);

    const struct tm *local_time = localtime(&current_time);

    commit_info.author_timezone = malloc(8);
    commit_info.commiter_timezone = malloc(8);

    const char offset_sign = local_time->tm_gmtoff < 0 ? '-' : '+';
    const long int offset_hours = local_time->tm_gmtoff / 3600;
    const long int offset_minutes = local_time->tm_gmtoff % 3600 / 60;

    sprintf(
        commit_info.author_timezone,
        "%c%02ld%02ld",
        offset_sign,
        offset_hours,
        offset_minutes);

    sprintf(
        commit_info.commiter_timezone,
        "%c%02ld%02ld",
        offset_sign,
        offset_hours,
        offset_minutes);

    commit_info.tree_sha = strndup(argv[2], SHA_HEX_LENGTH);

    bool opt_result = try_resolve_commit_tree_opts(argc, argv, &commit_info);
    validate(opt_result, "Failed to resolve options.");

    //
    printf("%s\n", commit_info.tree_sha);
    printf("%s\n", commit_info.parent_sha);
    printf("%s\n", commit_info.author_name);
    printf("%s\n", commit_info.author_email);
    printf("%s\n", commit_info.author_date);
    printf("%s\n", commit_info.author_timezone);
    printf("%s\n", commit_info.committer_name);
    printf("%s\n", commit_info.committer_email);
    printf("%s\n", commit_info.committer_date);
    printf("%s\n", commit_info.commiter_timezone);
    printf("%s\n", commit_info.message);
    //

    char hash_hex[SHA_HEX_LENGTH];
    char *commit_hash = write_commit_object(&commit_info, nullptr);

    validate(commit_hash, "Failed to write tree.");

    printf("%s", hash_hex);

    destroy_commit_tree_info(&commit_info);

    return 0;

error:
    return 1;
}
