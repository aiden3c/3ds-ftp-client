#ifndef FS_H
#define FS_H

#include <citro2d.h>

typedef struct AddressBookEntry AddressBookEntry;
typedef struct AddressBook AddressBook;

extern FS_Archive sdmcArchive;
extern AddressBook addressBook;

struct File {
    char* path;
    int directory;
};

struct AddressBookEntry {
    char name[64]; // arbitrary
    char address[128]; // arbitrary
    char port[5];
};

struct AddressBook {
    AddressBookEntry data[128]; // arbitrary
};

// Function delcarations
void initSD(void);
void exitSD(void);
char** listSD(char* path, int* length);
Result createFile(FS_Archive archive, FS_Path path, u64 fileSize);

#endif // FS_H
