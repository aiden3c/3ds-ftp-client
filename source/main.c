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
#define MAX_TEXT_SIZE 1024 * 16

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
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();
		hidTouchRead( &touch );

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, backgroundColor);
        C2D_SceneBegin(top);

        char randomData[100];
        snprintf(randomData, sizeof(randomData), "(%d, %d)", touch.px, touch.py);
        drawText(6, 6, 1, 1, textColor, randomData, C2D_WithColor | C2D_WordWrap, font);

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break; // break in order to return to hbmenu


		C3D_FrameEnd(0);
	}
	C2D_FontFree(font);
	exit(0);
}
