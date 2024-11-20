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


#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240

#define MAX_TEXT_SIZE 1024 * 16
#define MAX_SPRITES 256 //arbitrary

FS_Archive ArchiveSD;

enum Scene {
    ROOT,
    EDIT,
    BOOK
};

typedef struct
{
    int rainbowDelay;
    int rainbowDelayDefault;
    enum Scene scene;
    touchPosition pressedCoords;
    u32 colors[128];
    u32 backgroundColorBottom;
    u32 backgroundColorTop;

    u32 *fileSystemHandle;
} AppState;

typedef void (*ButtonCallback)(AppState*, touchPosition);

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
    ButtonCallback callback;
    enum Scene scene;
} UIButton;

static C2D_SpriteSheet spriteSheet;
AppState mainState;
C2D_ImageTint rainbowTint;
C2D_ImageTint shadowTint;
C2D_ImageTint disabledTint;

void addressBookCallback(AppState* mainState, touchPosition _) {
    mainState->backgroundColorBottom = mainState->colors[1];
    mainState->scene = BOOK;
}

void addressConnectCallback(AppState* mainState, touchPosition _) {
    exit(0);
}

void buttonEditCallback(AppState* mainState, touchPosition _) {
    mainState->backgroundColorBottom = mainState->colors[1];
    mainState->scene = EDIT;
}

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
        srand((unsigned int)time(NULL));
        hue = (float)rand() / RAND_MAX;
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

    if(button->pressed) { // Raise button for rendering to indicate pressability
        needShadow = 0;
    } else if (button->disabled) {
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

int initSD()
{
    Result res;
    if(R_FAILED(res = FSUSER_OpenArchive(&ArchiveSD, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) return res;

    return 0;
}

int main(int argc, char **argv)
{
    romfsInit();
    cfguInit();
    gfxInitDefault();
    initTint();
    atexit(gfxExit);


    fsInit();
    mainState.fileSystemHandle = fsGetSessionHandle();
    

    mainState.colors[0] = C2D_Color32(0xF1, 0xEA, 0xA7, 0xFF); // Background
    mainState.colors[1] = C2D_Color32(0xC4, 0xBC, 0x6A, 0xFF); // Background Dark
    mainState.colors[2] = C2D_Color32(0xFB, 0xFC, 0xFC, 0xFF); // Text (Iced)


    // State initialization
    mainState.rainbowDelayDefault = 5;
    mainState.rainbowDelay = 5;
    mainState.scene = ROOT;
    mainState.backgroundColorTop = mainState.colors[0];
    mainState.backgroundColorBottom = mainState.colors[0];

    // Don't worry I'm moving this and other things to another file to import
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
    buttonMain.callback = addressConnectCallback;
    buttonMain.scene = 0;

	UIButton buttonEdit;
	buttonEdit.pressed = 0;
	buttonEdit.disabled = 0;
	buttonEdit.rainbow = 0;
	buttonEdit.text = "Edit";
	C2D_SpriteFromSheet(&buttonEdit.sprite, spriteSheet, 1);
	buttonEdit.sprite.params.pos.x = 42;
	buttonEdit.sprite.params.pos.y = 128;
	buttonEdit.textOffsetX = 10;
	buttonEdit.textOffsetY = 0;
	buttonEdit.textScale = 2;
    buttonEdit.callback = buttonEditCallback;
    buttonEdit.scene = 0;

    UIButton buttonAddressBook;
	buttonAddressBook.pressed = 0;
	buttonAddressBook.disabled = 0;
	buttonAddressBook.rainbow = 0;
	buttonAddressBook.text = "Book";
	C2D_SpriteFromSheet(&buttonAddressBook.sprite, spriteSheet, 1);
	buttonAddressBook.sprite.params.pos.x = 175;
	buttonAddressBook.sprite.params.pos.y = 128;
	buttonAddressBook.textOffsetX = 10;
	buttonAddressBook.textOffsetY = 0;
	buttonAddressBook.textScale = 2;
    buttonAddressBook.callback = addressBookCallback;
    buttonAddressBook.scene = 0;


    UIButton* buttons[25]; //arbitrary
    buttons[0] = &buttonMain;
    buttons[1] = &buttonEdit;
    buttons[2] = &buttonAddressBook;
    int buttonCount = 3;

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	touchPosition touch;

	C2D_Font font = C2D_FontLoad("romfs:/ffbold.bcfnt");
	buttonMain.font = &font;
    buttonEdit.font = &font;
    buttonAddressBook.font = &font;
    

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
            if(buttons[i]->sprite.params.pos.x < mainState.pressedCoords.px && buttons[i]->sprite.params.pos.x + buttons[i]->sprite.params.pos.w > mainState.pressedCoords.px && buttons[i]->sprite.params.pos.y < mainState.pressedCoords.py && buttons[i]->sprite.params.pos.y + buttons[i]->sprite.params.pos.h > mainState.pressedCoords.py) {
                    buttons[i]->pressed = 1;
            } else {
                buttons[i]->pressed = 0;
            }
        }
        if(touch.px == 0 && touch.py == 0) {
            for (int i = 0; i < buttonCount; i++) {
                if(buttons[i]->pressed && buttons[i]->disabled == 0) {
                    buttons[i]->callback(&mainState, touch);
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
        char randomData[100];
        snprintf(randomData, sizeof(randomData), "(%d, %d)\n(%d, %d)\n%d", touch.px, touch.py, mainState.pressedCoords.px, mainState.pressedCoords.py, mainState.fileSystemHandle);
        drawText(6, 6, 1, 1, mainState.colors[2], randomData, C2D_WithColor | C2D_WordWrap, font);


        // Bottom Screen
        C2D_TargetClear(bottom, mainState.backgroundColorBottom);
        C2D_SceneBegin(bottom);
        for (int i = 0; i < buttonCount; i++) {
            if(mainState.scene == buttons[i]->scene) {
                drawButton(buttons[i]);
            }
        }

        // Ooh pretty colors
        updateRainbowTint(&rainbowTint, &mainState);

		C3D_FrameEnd(0);
	}
	C2D_FontFree(font);
	exit(0);
}
