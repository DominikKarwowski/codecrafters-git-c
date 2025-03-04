#ifndef CAT_FILE_H
#define CAT_FILE_H

struct object_path
{
    char subdir[3];
    char name[19];
};

int cat_file(int argc, char *argv[]);

#endif //CAT_FILE_H
