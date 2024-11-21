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

int main(int argc, char **argv)
{
    romfsInit();
    cfguInit();
    gfxInitDefault();
    initTint();
    atexit(gfxExit);

    initSD();
    mainState.fileSystemHandle = fsGetSessionHandle();

    int buttonCount;
    UIButton* buttons = initButtons(&buttonCount);

    mainState.colors[0] = C2D_Color32(0xF1, 0xEA, 0xA7, 0xFF); // Background
    mainState.colors[1] = C2D_Color32(0xC4, 0xBC, 0x6A, 0xFF); // Background Dark
    mainState.colors[2] = C2D_Color32(0xFB, 0xFC, 0xFC, 0xFF); // Text (Iced)


    // State initialization
    mainState.rainbowDelayDefault = 5;
    mainState.rainbowDelay = 5;
    mainState.scene = ROOT;
    mainState.backgroundColorTop = mainState.colors[0];
    mainState.backgroundColorBottom = mainState.colors[0];

    mainState.fileList = listSD("/", &mainState.fileCount);

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	touchPosition touch;

	C2D_Font font = C2D_FontLoad("romfs:/ffbold.bcfnt");
	for (int i = 0; i < buttonCount; i++) {
	   buttons[i].font = &font;
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
            if(buttons[i].sprite.params.pos.x < mainState.pressedCoords.px && buttons[i].sprite.params.pos.x + buttons[i].sprite.params.pos.w > mainState.pressedCoords.px && buttons[i].sprite.params.pos.y < mainState.pressedCoords.py && buttons[i].sprite.params.pos.y + buttons[i].sprite.params.pos.h > mainState.pressedCoords.py) {
                    buttons[i].pressed = 1;
            } else {
                buttons[i].pressed = 0;
            }
        }
        if(touch.px == 0 && touch.py == 0) {
            for (int i = 0; i < buttonCount; i++) {
                if(buttons[i].pressed && buttons[i].disabled == 0) {
                    buttons[i].callback(&mainState, touch);
                    mainState.pressedCoords = touch;
                }
            }
        }


        // Keyboard inputs
        u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break; // break in order to return to hbmenu
        if (kDown & KEY_A) {
            mainState.scene = 0;
            mainState.backgroundColorBottom = mainState.colors[0];
        }


		// Top Screen
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, mainState.backgroundColorTop);
        C2D_SceneBegin(top);
        mainState.message = createFile(sdmcArchive, fsMakePath(PATH_ASCII, "/3ds/ftp-client/addressbook.bin"), (u32)sizeof(addressBook));
        char randomData[100];
        snprintf(randomData, sizeof(randomData), "%x", mainState.message);
        drawText(6, 6, 1, 1, mainState.colors[2], randomData, C2D_WithColor | C2D_WordWrap, font);
        //for (int i = 0; i < mainState.fileCount; i++) {
        //    drawText(6, (i + 10) * 6, 1, 1, mainState.colors[2], mainState.fileList[i], C2D_WithColor | C2D_WordWrap, font);
        //}


        // Bottom Screen
        C2D_TargetClear(bottom, mainState.backgroundColorBottom);
        C2D_SceneBegin(bottom);
        for (int i = 0; i < buttonCount; i++) {
            if(mainState.scene == buttons[i].scene) {
                drawButton(&buttons[i]);
            }
        }

        // Ooh pretty colors
        updateRainbowTint(&rainbowTint, &mainState);

		C3D_FrameEnd(0);
	}
	C2D_FontFree(font);
	exitSD();
	exit(0);
}
