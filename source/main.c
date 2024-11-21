#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>


#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>

#include <curl/curl.h>
#include <math.h>

#include <citro2d.h>

#include <3ds.h>

#include <archive.h>
#include <archive_entry.h>

#include "ui.h"
#include "fs.h"

AppState mainState;

void initTint()
{
    u32 shadowColor = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
    u32 disabledColor = C2D_Color32(0x00, 0x00, 0x00, 0x99);

    for (int i = 0; i < 4; i++) {
        shadowTint.corners[i].color = shadowColor;
        shadowTint.corners[i].blend = 1.0f;

        disabledTint.corners[i].color = disabledColor;
        disabledTint.corners[i].blend = 1.0f;
    }
}

void addDummyEntry(AppState *mainState)
{
    int index = mainState->addressBook.size;
    // Set name
    strncpy(mainState->addressBook.data[index].name, "Test", sizeof(mainState->addressBook.data[index].name) - 1);
    mainState->addressBook.data[index].name[sizeof(mainState->addressBook.data[index].name) - 1] = '\0';

    // Set address
    strncpy(mainState->addressBook.data[index].address, "192.168.0.110", sizeof(mainState->addressBook.data[index].address) - 1);
    mainState->addressBook.data[index].address[sizeof(mainState->addressBook.data[index].address) - 1] = '\0';

    // Set port
    strncpy(mainState->addressBook.data[index].port, "5000", sizeof(mainState->addressBook.data[index].port) - 1);
    mainState->addressBook.data[index].port[sizeof(mainState->addressBook.data[index].port) - 1] = '\0';
}

int main(int argc, char **argv)
{
    romfsInit();
    cfguInit();
    gfxInitDefault();
    initTint();
    atexit(gfxExit);

    initSD();

    int buttonCount;
    mainState.buttons = initButtons(&buttonCount);

    mainState.colors[0] = C2D_Color32(0xF1, 0xEA, 0xA7, 0xFF); // Background
    mainState.colors[1] = C2D_Color32(0xC4, 0xBC, 0x6A, 0xFF); // Background Dark
    mainState.colors[2] = C2D_Color32(0xFB, 0xFC, 0xFC, 0xFF); // Light Text (Iced)
    mainState.colors[3] = C2D_Color32(0x64, 0x00, 0x75, 0xFF); // Dark Text (Plum?)


    // State initialization
    mainState.rainbowDelayDefault = 5;
    mainState.rainbowDelay = 5;
    mainState.scene = ROOT;
    mainState.backgroundColorTop = mainState.colors[0];
    mainState.backgroundColorBottom = mainState.colors[0];

    //mainState.message = saveAddressBook(&mainState.addressBook);
    // Load saved data
    mainState.fileList = listSD("/", &mainState.fileCount);
    readAddressBook(&mainState.addressBook);
    mainState.message = mainState.addressBook.size;

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	touchPosition touch;

	C2D_Font font = C2D_FontLoad("romfs:/ffbold.bcfnt");
	for (int i = 0; i < buttonCount; i++) {
	   mainState.buttons[i].font = &font;
	}

	while (aptMainLoop())
	{
        // Pre-processing for frame
        hidScanInput();
        hidTouchRead( &touch );

        // All of this runs the button callbacks if they were selected when released
        if(!(touch.px == 0 && touch.py == 0)) {
            mainState.pressedCoords = touch;
        }
        for (int i = 0; i < buttonCount; i++) {
            if(mainState.buttons[i].sprite.params.pos.x < mainState.pressedCoords.px && mainState.buttons[i].sprite.params.pos.x + mainState.buttons[i].sprite.params.pos.w > mainState.pressedCoords.px && mainState.buttons[i].sprite.params.pos.y < mainState.pressedCoords.py && mainState.buttons[i].sprite.params.pos.y + mainState.buttons[i].sprite.params.pos.h > mainState.pressedCoords.py) {
                    mainState.buttons[i].pressed = 1;
            } else {
                mainState.buttons[i].pressed = 0;
            }
        }
        if(touch.px == 0 && touch.py == 0) {
            for (int i = 0; i < buttonCount; i++) {
                if(mainState.buttons[i].pressed && mainState.buttons[i].disabled == 0 && mainState.buttons[i].scene == mainState.scene) {
                    mainState.buttons[i].callback(&mainState, mainState.pressedCoords);
                    mainState.pressedCoords = touch;
                    break;
                }
            }
        }


        // Keyboard inputs
        u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break; // break in order to return to hbmenu
        if (kDown & KEY_A)
        {
            mainState.scene = 0;
            mainState.backgroundColorBottom = mainState.colors[0];
        }
        if (kDown & KEY_B)
        {
            addDummyEntry(&mainState);
            saveAddressBook(&mainState.addressBook);
            mainState.message = mainState.addressBook.size;
        }


		// Top Screen
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, mainState.backgroundColorTop);
        C2D_SceneBegin(top);
        char randomData[100];
        snprintf(randomData, sizeof(randomData), "%d\n%d\n%d", mainState.currentAddress, mainState.addressBook.size, mainState.addressBook.size - (mainState.addressBookPage * 2));
        drawText(0, 0, 1, 1, mainState.colors[3], randomData, C2D_WithColor | C2D_WordWrap, font);
        //for (int i = 0; i < mainState.fileCount; i++) {
        //    drawText(6, (i + 10) * 6, 1, 1, mainState.colors[2], mainState.fileList[i], C2D_WithColor | C2D_WordWrap, font);
        //}


        // Bottom Screen
        C2D_TargetClear(bottom, mainState.backgroundColorBottom);
        C2D_SceneBegin(bottom);
        for (int i = 0; i < buttonCount; i++) {
            if(mainState.scene == mainState.buttons[i].scene) {
                drawButton(&mainState.buttons[i]);
            }
        }
        if(mainState.scene == BOOK)
        {
            char subtitle[135];
            UIButton* addressButton;
            if((2 * mainState.addressBookPage) < mainState.addressBook.size) {
                addressButton = &mainState.buttons[3];
                addressButton->text = mainState.addressBook.data[(2 * mainState.addressBookPage)].name;
                snprintf(subtitle, sizeof(subtitle), "%s:%s", mainState.addressBook.data[(2 * mainState.addressBookPage)].address, mainState.addressBook.data[(2 * mainState.addressBookPage)].port);
                addressButton->subtext = subtitle;
            }
            else {
                addressButton = &mainState.buttons[3];
                addressButton->text = "New Entry";
                addressButton->subtext = "Add a new entry.";
            }

            if((2 * mainState.addressBookPage) + 1 < mainState.addressBook.size) {
                addressButton = &mainState.buttons[4];
                addressButton->text = mainState.addressBook.data[(2 * mainState.addressBookPage) + 1].name;
                snprintf(subtitle, sizeof(subtitle), "%s:%s", mainState.addressBook.data[(2 * mainState.addressBookPage) + 1].address, mainState.addressBook.data[(2 * mainState.addressBookPage) + 1].port);
                addressButton->subtext = subtitle;
            }
            else {
                addressButton = &mainState.buttons[4];
                addressButton->text = "New Entry";
                addressButton->subtext = "Add a new entry.";
            }


            char page[4];
            volatile int page_size = sizeof(page);
            snprintf(page, page_size, "%d", mainState.addressBookPage + 1);
            drawText(155, 206, 1, 1, mainState.colors[3], page, C2D_WithColor | C2D_WordWrap, font);
        }

        // Ooh pretty colors
        updateRainbowTint(&rainbowTint, &mainState);

		C3D_FrameEnd(0);
	}
	C2D_FontFree(font);
	exitSD();
	exit(0);
}
