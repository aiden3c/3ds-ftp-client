#include "fs.h"

FS_Archive sdmcArchive;
AddressBook addressBook;

int countBookEntries(const AddressBook* book) {
    int count = 0;
    for (int i = 0; i < 128; ++i) {
        if (book->data[i].name[0] != '\0') {
            ++count;
        }
    }
    return count;
}

Result saveAddressBook(AddressBook *book)
{
    Result res;
    Handle addressHandle;
    u32 bytesWritten;

    res = FSUSER_OpenFile(&addressHandle, sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/ftp-client/addressbook.bin"), FS_OPEN_WRITE, 0);
    res = FSFILE_Write(addressHandle, &bytesWritten, 0, book, sizeof(AddressBook), FS_WRITE_FLUSH);
    FSFILE_Close(addressHandle);

    book->size = countBookEntries(book);
    return res;
}

Result readAddressBook(AddressBook *book)
{
    u32 bytesRead;

    Result res;
    Handle addressHandle;
    res = FSUSER_OpenFile(&addressHandle, sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/ftp-client/addressbook.bin"), FS_OPEN_READ, 0);
    res = FSFILE_Read(addressHandle, &bytesRead, 0, book, sizeof(AddressBook));
    if (R_FAILED(res)) {
        return res;
    } else if (bytesRead != sizeof(AddressBook)) {
        return 0x2b16; // 2big (or small actually)
    }

    FSFILE_Close(addressHandle);

    book->size = countBookEntries(book);
    return res;
}


Result initSD()
{

    // Make sure we have our own folder
    if(R_SUCCEEDED(FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))){
        FSUSER_CreateDirectory(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/ftp-client"), 0);
    } else {
        exit(0); // just kill the program if no SD card for now
    }

    // Make sure we have an address book saved
    Result res;
    Handle addressHandle;
    AddressBook tmp;
    res = FSUSER_OpenFile(&addressHandle, sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/ftp-client/addressbook.bin"), FS_OPEN_READ, 0);
    if(R_FAILED(res)) {
        if(R_SUMMARY(res) == RS_NOTFOUND) {
            FSUSER_CreateFile(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/ftp-client/addressbook.bin"), 0, (u64)sizeof(AddressBook));
            saveAddressBook(&tmp);
        }
    }
    FSFILE_Close(addressHandle);
    return res;
}

void exitSD()
{
    FSUSER_CloseArchive(sdmcArchive);
}

char* utfToAscii(const u16* utf16String)
{
    int utf16Length = 0;
    while (utf16String[utf16Length] != 0) {
        utf16Length++;
    }
    char* asciiString = (char*)malloc(utf16Length + 1);
    if (asciiString == NULL) {
        return NULL;
    }
    for (int i = 0; i < utf16Length; i++)
    {
        if (utf16String[i] <= 0x7F) {
            asciiString[i] = (char)utf16String[i];
        } else {
            asciiString[i] = '?';
        }
    }
    asciiString[utf16Length] = '\0';
    return asciiString;
}

char** listSD(char* path, int* count)
{
    FS_DirectoryEntry entry;
    Handle dirHandle;
    FS_Path dirPath = fsMakePath(PATH_ASCII, path);

    FSUSER_OpenDirectory(&dirHandle, sdmcArchive, dirPath);
    u32 entriesRead;
    static char* names[1024];
    for (int i = 0;;i++)
    {

            entriesRead=0;

            FSDIR_Read(dirHandle, &entriesRead, 1, (FS_DirectoryEntry*)&entry);

            names[i] = utfToAscii(entry.name);
            if (entriesRead){}else
            {
                *count = i;
                break;
            }
    }

    //Close handles
    FSDIR_Close(dirHandle);
    return names;
}
