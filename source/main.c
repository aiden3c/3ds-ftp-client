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

#include <citro2d.h>

#include <3ds.h>

#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240

#define MAX_TEXT_SIZE 1024 * 16
#define MAX_SPRITES 256 //arbitrary

typedef struct
{
	C2D_Sprite sprite;
	bool pressed;
} UIButton;

static C2D_SpriteSheet spriteSheet;


void drawText(int x, int y, int z, float scale, u32 color, char* text, int flags, C2D_Font font) {
    C2D_Text drawText;
    C2D_TextBuf dynamicBuffer = C2D_TextBufNew(MAX_TEXT_SIZE);
    C2D_TextBufClear(dynamicBuffer);
    C2D_TextFontParse( &drawText, font, dynamicBuffer, text );
    C2D_TextOptimize( &drawText );

    C2D_DrawText( &drawText, flags, x, y, z, scale, scale, color, TOP_SCREEN_WIDTH - 9.0 );

    C2D_TextBufDelete(dynamicBuffer);
}

int main(int argc, char **argv)
{
    romfsInit();
    cfguInit();
    gfxInitDefault();
    atexit(gfxExit);

    // Load graphics
	spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
	if (!spriteSheet) svcBreak(USERBREAK_PANIC);

	UIButton buttonMain;
	buttonMain.pressed = 0;
	C2D_SpriteFromSheet(&buttonMain.sprite, spriteSheet, 0);
	buttonMain.sprite.params.pos.x = 38;
	buttonMain.sprite.params.pos.y = 20;


    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	touchPosition touch;

	C2D_Font font = C2D_FontLoad("romfs:/ffbold.bcfnt");
	u32 backgroundColor = C2D_Color32(0xF1, 0xEA, 0xA7, 0xFF);
	u32 textColor  = C2D_Color32(0xFB, 0xFC, 0xFC, 0xFF);

	while (aptMainLoop())
	{
        // Pre-processing for frame
        hidScanInput();
        hidTouchRead( &touch );

        u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break; // break in order to return to hbmenu


		// Top Screen
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, backgroundColor);
        C2D_SceneBegin(top);
        char randomData[100];
        snprintf(randomData, sizeof(randomData), "(%d, %d)", touch.px, touch.py);
        drawText(6, 6, 1, 1, textColor, randomData, C2D_WithColor | C2D_WordWrap, font);


        // Bottom Screen
        C2D_TargetClear(bottom, backgroundColor);
        C2D_SceneBegin(bottom);
        C2D_DrawSprite(&buttonMain.sprite);

		C3D_FrameEnd(0);
	}
	C2D_FontFree(font);
	exit(0);
}

void failExit(const char *fmt, ...) {
    va_list ap;

    printf("\x1b[31;1m"); // Set color to red
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\x1b[0m"); // Reset color
    printf("\nPress B to exit\n");

    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();

        u32 kDown = hidKeysDown();
        if (kDown & KEY_B) exit(0);
    }
}
