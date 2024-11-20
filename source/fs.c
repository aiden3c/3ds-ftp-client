#include "fs.h"
#include <stdlib.h>


FS_Archive sdmcArchive;

void initSD() {
    FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
}

char* utfToAscii(const u16* utf16String) {
    // Calculate the length of the UTF-16 string
    int utf16Length = 0;
    while (utf16String[utf16Length] != 0) {
        utf16Length++;
    }

    // Allocate memory for the ASCII string
    char* asciiString = (char*)malloc(utf16Length + 1);
    if (asciiString == NULL) {
        return NULL; // Return NULL on allocation failure
    }

    // Convert UTF-16 to ASCII
    for (int i = 0; i < utf16Length; i++) {
        // Check if the UTF-16 character is in the ASCII range
        if (utf16String[i] <= 0x7F) {
            asciiString[i] = (char)utf16String[i];
        } else {
            // Replace non-ASCII characters with a placeholder
            asciiString[i] = '?';
        }
    }
    asciiString[utf16Length] = '\0'; // Null-terminate the ASCII string

    return asciiString;
}

char** listSD(char* pathooh, int* count) {
    FS_DirectoryEntry entry;
    const char *path = "/";
    Handle dirHandle;
    FS_Path dirPath = fsMakePath(PATH_ASCII, path);

    FSUSER_OpenDirectory(&dirHandle, sdmcArchive, dirPath);
    u32 entriesRead;
    static char* names[1024];
    for (int i = 0;;i++){

            //Reset entries var
            entriesRead=0;

            //Read the next item in the directory
            FSDIR_Read(dirHandle, &entriesRead, 1, (FS_DirectoryEntry*)&entry);

            names[i] = utfToAscii(entry.name);
            //If there is a next item
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
