#include "ls_tree.h"

#include <getopt.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "compression.h"
#include "debug_helpers.h"
#include "git_dir_helpers.h"
#include "git_obj_helpers.h"

#define GIT_OBJ_HEADER_SIZE 64

bool name_only_opt = false;

static bool try_resolve_ls_tree_opts(const int argc, char **argv)
{
    opterr = 0;
    int opt;

    const struct option long_opts[] = {
        { "name-only", no_argument, nullptr, 'n' },
        { nullptr, 0, nullptr, 0 }
    };

    while ((opt = getopt_long(argc, argv, "", long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'n':
                name_only_opt = true;
                break;
            case '?':
                validate(false, "Invalid switch: '%c'\n", optopt);
            default:
                validate(false, "Unrecognized option: %c\n", optopt);
        }
    }

    return true;

error:
    return false;
}

static bool is_expected_obj_type(const char *obj_content, const char *expected, const size_t type_def_len)
{
    for (size_t i = 0; i < type_def_len; i++)
    {
        if (obj_content[i] != expected[i])
            return false;
    }

    return true;
}

struct git_tree_node
{
    char *mode;
    char *name;
    unsigned char *hash;
};

static void destroy_git_tree_node(struct git_tree_node *node)
{
    if (!node) return;

    if (node->mode) free(node->mode);
    if (node->name) free(node->name);
    if (node->hash) free(node->hash);

    free(node);
}

static size_t try_set_node(struct git_tree_node *node, const char *obj_content, const size_t start)
{
    size_t curr_pos = start;
    size_t elem_start = start;

    while (obj_content[curr_pos] != ' ') curr_pos++;

    size_t elem_len = curr_pos - elem_start;
    node->mode = malloc(elem_len + 1);
    validate(node->mode, "Failed to allocate memory.");

    memcpy(node->mode, &obj_content[elem_start], elem_len);
    node->mode[elem_len] = '\0';
    elem_start = ++curr_pos;

    while (obj_content[curr_pos] != '\0') curr_pos++;

    elem_len = curr_pos - elem_start;
    node->name = malloc(elem_len + 1);
    validate(node->name, "Failed to allocate memory.");

    memcpy(node->name, &obj_content[elem_start], elem_len);
    node->name[elem_len] = '\0';
    elem_start = ++curr_pos;

    node->hash = malloc(elem_len);
    validate(node->hash, "Failed to allocate memory.");

    elem_len = 20;
    memcpy(node->hash, &obj_content[elem_start], elem_len);

    curr_pos += 20;

    return curr_pos;

error:
    return 0;
}

static void print_tree_node_name_only(const struct git_tree_node *node)
{
    if (!node) return;

    printf("%s", node->name);
    putchar('\n');
}

static void print_tree_node_full(const struct git_tree_node *node)
{
    if (!node) return;

    char hash_hex[40];
    hash_bytes_to_hex(hash_hex, node->hash);

    char *inflated_buffer = nullptr;

    (void)get_object_content(hash_hex, &inflated_buffer);
    validate(inflated_buffer, "Failed to obtain object content.");

    char obj_type[16];
    get_object_type(obj_type, inflated_buffer);

    const size_t leading_zeros = 6 - strlen(node->mode);
    for (size_t i = 0; i < leading_zeros; i++) putchar('0');

    printf("%s", node->mode);
    putchar(' ');

    printf("%s", obj_type);
    putchar(' ');

    printf("%s", hash_hex);
    putchar(' ');

    printf("%s", node->name);
    putchar('\n');

    free(inflated_buffer);

    return;

error:
    if (inflated_buffer) free(inflated_buffer);
}

static void print_tree_content(const char *inflated_buffer, const size_t inflated_buffer_size, const bool name_only)
{
    size_t curr_pos = get_header_size(inflated_buffer);
    curr_pos++;

    struct git_tree_node *node;
    while (curr_pos < inflated_buffer_size)
    {
        node = malloc(sizeof(struct git_tree_node));
        validate(node, "Failed to allocate memory.");

        curr_pos = try_set_node(node, inflated_buffer, curr_pos);
        validate(curr_pos, "Failed to read git tree node.");

        if (name_only)
        {
            print_tree_node_name_only(node);
        }
        else
        {
            print_tree_node_full(node);
        }

        destroy_git_tree_node(node);
    }

    return;

error:
    if (node) destroy_git_tree_node(node);
}

int ls_tree(const int argc, char *argv[])
{
    validate(try_resolve_ls_tree_opts(argc, argv), "Failed to resolve options.");

    const char *tree_hash = argv[argc - 1];
    char *inflated_buffer = nullptr;

    const size_t inflated_buffer_size = get_object_content(tree_hash, &inflated_buffer);
    validate(inflated_buffer, "Failed to obtain object content.");

    const char *expected = "tree";
    validate(is_expected_obj_type(inflated_buffer, expected, 4), "Expected %s object type.", expected);

    print_tree_content(inflated_buffer, inflated_buffer_size, name_only_opt);

    free(inflated_buffer);

    return 0;

error:
    if (inflated_buffer) free(inflated_buffer);

    return 1;
}
