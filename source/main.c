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
	bool disabled;
	bool rainbow;
	char* text;
	int textOffsetX;
	int textOffsetY;
	float textScale;
	C2D_Font* font;
} UIButton;

typedef struct
{
    int rainbowDelay;
    int rainbowDelayDefault;
} AppState;

static C2D_SpriteSheet spriteSheet;
AppState mainState;
C2D_ImageTint rainbowTint;
C2D_ImageTint shadowTint;
C2D_ImageTint disabledTint;

void drawText(int x, int y, int z, float scale, u32 color, char* text, int flags, C2D_Font font) {
    C2D_Text drawText;
    C2D_TextBuf dynamicBuffer = C2D_TextBufNew(MAX_TEXT_SIZE);
    C2D_TextBufClear(dynamicBuffer);
    C2D_TextFontParse( &drawText, font, dynamicBuffer, text );
    C2D_TextOptimize( &drawText );

    C2D_DrawText( &drawText, flags, x, y, z, scale, scale, color, TOP_SCREEN_WIDTH - 9.0 );

    C2D_TextBufDelete(dynamicBuffer);
}

u32 hueToRGB(float hue) {
    float r = fabsf(hue * 6.0f - 3.0f) - 1.0f;
    float g = 2.0f - fabsf(hue * 6.0f - 2.0f);
    float b = 2.0f - fabsf(hue * 6.0f - 4.0f);

    r = fminf(fmaxf(r, 0.0f), 1.0f);
    g = fminf(fmaxf(g, 0.0f), 1.0f);
    b = fminf(fmaxf(b, 0.0f), 1.0f);

    return C2D_Color32((u8)(r * 255), (u8)(g * 255), (u8)(b * 255), 0xFF);
}

void updateRainbowTint(C2D_ImageTint* tint, AppState* appState) {
    static float hue = -1.0f;

    // Initialize the hue randomly on the first call
    if (hue < 0.0f) {
        srand((unsigned int)time(NULL)); // Seed the random generator
        hue = (float)rand() / RAND_MAX; // Random value between 0.0 and 1.0
    }

    if (appState->rainbowDelay > 0) {
        appState->rainbowDelay--;
        return;
    }
    appState->rainbowDelay = appState->rainbowDelayDefault;

    hue += 0.01f;
    if (hue >= 1.0f) hue -= 1.0f;

    u32 color = hueToRGB(hue);

    for (int i = 0; i < 4; i++) {
        tint->corners[i].color = color;
        tint->corners[i].blend = 1.0f;
    }
}

void drawButton(UIButton* button) {
    int distance = 4;
    bool needShadow = 1;

    if(button->pressed == 0 ^ button->disabled == 0) { // Raise button for rendering to indicate pressability
        needShadow = 0;
    }

    if(needShadow) {
        C2D_DrawSpriteTinted(&button->sprite, &shadowTint);
        button->sprite.params.pos.x -= distance;
        button->sprite.params.pos.y -= distance;
    }

    if(button->rainbow != 0 && button->disabled == 0)
    {
        C2D_DrawSpriteTinted(&button->sprite, &rainbowTint);
    } else {
        C2D_DrawSprite(&button->sprite);
    }

    drawText(button->sprite.params.pos.x + button->textOffsetX, button->sprite.params.pos.y + button->textOffsetY, 0, button->textScale, C2D_Color32(0xFB, 0xFC, 0xFC, 0xFF), button->text, C2D_WithColor | C2D_WordWrap, *button->font);

    if(button->disabled == 1)
    {
        C2D_DrawSpriteTinted(&button->sprite, &disabledTint);
    }

    // Reset positioning if applicable
    if(needShadow) {
        button->sprite.params.pos.x += distance;
        button->sprite.params.pos.y += distance;
    }

}

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

    // State initialization
    mainState.rainbowDelayDefault = 5;
    mainState.rainbowDelay = 5;

    // Load graphics
	spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
	if (!spriteSheet) svcBreak(USERBREAK_PANIC);

	UIButton buttonMain;
	buttonMain.pressed = 0;
	buttonMain.disabled = 1;
	buttonMain.rainbow = 0;
	buttonMain.text = "CONNECT!";
	C2D_SpriteFromSheet(&buttonMain.sprite, spriteSheet, 0);
	buttonMain.sprite.params.pos.x = 38;
	buttonMain.sprite.params.pos.y = 28;
	buttonMain.textOffsetX = 20;
	buttonMain.textOffsetY = 0;
	buttonMain.textScale = 2.2;


	UIButton buttonEdit;
	buttonEdit.pressed = 0;
	buttonEdit.disabled = 0;
	buttonEdit.rainbow = 0;
	buttonEdit.text = "Edit";
	C2D_SpriteFromSheet(&buttonEdit.sprite, spriteSheet, 1);
	buttonEdit.sprite.params.pos.x = 38;
	buttonEdit.sprite.params.pos.y = 128;
	buttonEdit.textOffsetX = 10;
	buttonEdit.textOffsetY = 0;
	buttonEdit.textScale = 2;


    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	touchPosition touch;

	C2D_Font font = C2D_FontLoad("romfs:/ffbold.bcfnt");
	buttonMain.font = &font;
    buttonEdit.font = &font;
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
        drawButton(&buttonMain);
        drawButton(&buttonEdit);
        updateRainbowTint(&rainbowTint, &mainState);

		C3D_FrameEnd(0);
	}
	C2D_FontFree(font);
	exit(0);
}
