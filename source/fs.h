#ifndef FS_H
#define FS_H

#include <citro2d.h>

struct File {
    char* path;
    int directory;
};

// Function delcarations
void initSD(void);
char** listSD(char* path, int* length);

#endif // FS_H
