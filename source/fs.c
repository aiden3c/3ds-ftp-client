#include "fs.h"
#include <stdlib.h>


FS_Archive sdmcArchive;
AddressBook addressBook;

Result createFile(FS_Archive archive, FS_Path path, u64 fileSize) {
    return FSUSER_CreateFile(archive, path, 0, fileSize);
}

void initSD() {
    Result res;
    if(R_SUCCEEDED(FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))){
        res = FSUSER_CreateDirectory(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/ftp-client"), 0);
        //if((u32)res == 0xC82044BE) // exists
        //    res = 0;
    } else {
        exit(0); // just kill the program if no SD card for now
    }

    Handle addressHandle;
    res = FSUSER_OpenFile(&addressHandle, ARCHIVE_SDMC, fsMakePath(PATH_ASCII, "/3ds/ftp-client/addressbook.bin"), FS_OPEN_READ, 0);
    if(R_FAILED(res)) {
        if(R_SUMMARY(res) == RS_NOTFOUND) {
            res = createFile(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/ftp-client/addressbook.bin"), (u32)sizeof(AddressBook));
            if (R_FAILED(res)) {
                exit(0);
            }
        }
    }

}

void exitSD() {
    FSUSER_CloseArchive(sdmcArchive);
}

char* utfToAscii(const u16* utf16String) {
    int utf16Length = 0;
    while (utf16String[utf16Length] != 0) {
        utf16Length++;
    }
    char* asciiString = (char*)malloc(utf16Length + 1);
    if (asciiString == NULL) {
        return NULL;
    }
    for (int i = 0; i < utf16Length; i++) {
        if (utf16String[i] <= 0x7F) {
            asciiString[i] = (char)utf16String[i];
        } else {
            asciiString[i] = '?';
        }
    }
    asciiString[utf16Length] = '\0';
    return asciiString;
}

char** listSD(char* path, int* count) {
    FS_DirectoryEntry entry;
    Handle dirHandle;
    FS_Path dirPath = fsMakePath(PATH_ASCII, path);

    FSUSER_OpenDirectory(&dirHandle, sdmcArchive, dirPath);
    u32 entriesRead;
    static char* names[1024];
    for (int i = 0;;i++){

            entriesRead=0;

            FSDIR_Read(dirHandle, &entriesRead, 1, (FS_DirectoryEntry*)&entry);

            names[i] = utfToAscii(entry.name);
            if (entriesRead){}else {
                *count = i;
                break;
            }
    }
    //Close handles and archives.
    FSDIR_Close(dirHandle);
    svcCloseHandle(dirHandle);
    FSUSER_CloseArchive(sdmcArchive);
    return names;
}
