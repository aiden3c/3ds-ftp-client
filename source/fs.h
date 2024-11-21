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
    char port[6]; // Port size + delimeter
};

struct AddressBook {
    AddressBookEntry data[128]; // arbitrary
};

// Function delcarations
Result initSD(void);
void exitSD(void);
char** listSD(char* path, int* length);
Result createFile(FS_Archive archive, FS_Path path, u64 fileSize);
Result readAddressBook(AddressBook *book);
Result saveAddressBook(AddressBook *book);
size_t countBookEntries(const AddressBook* book);

#endif // FS_H
