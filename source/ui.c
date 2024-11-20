#include "ui.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240

#define MAX_TEXT_SIZE 1024 * 16
#define MAX_SPRITES 256 //arbitrary

C2D_ImageTint rainbowTint;
C2D_ImageTint shadowTint;
C2D_ImageTint disabledTint;
C2D_SpriteSheet spriteSheet;

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

UIButton* initButtons(int* buttonCount) {
   	spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
	if (!spriteSheet) svcBreak(USERBREAK_PANIC);

    int count = 3; // Adjust this as needed
    UIButton* buttons = malloc(sizeof(UIButton) * count);

    buttons[0].pressed = 0;
    buttons[0].disabled = 1;
    buttons[0].rainbow = 0;
    buttons[0].text = "CONNECT!";
    C2D_SpriteFromSheet(&buttons[0].sprite, spriteSheet, 0);
    buttons[0].sprite.params.pos.x = 38;
    buttons[0].sprite.params.pos.y = 28;
    buttons[0].textOffsetX = 20;
    buttons[0].textOffsetY = 0;
    buttons[0].textScale = 2.2;
    buttons[0].callback = addressConnectCallback;
    buttons[0].scene = 0;

    // Initialize buttonEdit
    buttons[1].pressed = 0;
    buttons[1].disabled = 0;
    buttons[1].rainbow = 0;
    buttons[1].text = "Edit";
    C2D_SpriteFromSheet(&buttons[1].sprite, spriteSheet, 1);
    buttons[1].sprite.params.pos.x = 42;
    buttons[1].sprite.params.pos.y = 128;
    buttons[1].textOffsetX = 10;
    buttons[1].textOffsetY = 0;
    buttons[1].textScale = 2;
    buttons[1].callback = buttonEditCallback;
    buttons[1].scene = 0;

    // Initialize buttonAddressBook
    buttons[2].pressed = 0;
    buttons[2].disabled = 0;
    buttons[2].rainbow = 0;
    buttons[2].text = "Book";
    C2D_SpriteFromSheet(&buttons[2].sprite, spriteSheet, 1);
    buttons[2].sprite.params.pos.x = 175;
    buttons[2].sprite.params.pos.y = 128;
    buttons[2].textOffsetX = 10;
    buttons[2].textOffsetY = 0;
    buttons[2].textScale = 2;
    buttons[2].callback = addressBookCallback;
    buttons[2].scene = 0;

    *buttonCount = count;
    return buttons;
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

void drawText(int x, int y, int z, float scale, u32 color, char* text, int flags, C2D_Font font) {
    C2D_Text drawText;
    C2D_TextBuf dynamicBuffer = C2D_TextBufNew(MAX_TEXT_SIZE);
    C2D_TextBufClear(dynamicBuffer);
    C2D_TextFontParse( &drawText, font, dynamicBuffer, text );
    C2D_TextOptimize( &drawText );

    C2D_DrawText( &drawText, flags, x, y, z, scale, scale, color, TOP_SCREEN_WIDTH - 9.0 );

    C2D_TextBufDelete(dynamicBuffer);
}

void drawButton(UIButton* button) {
    int distance = 4;
    bool needShadow = 1;

    if(button->pressed) {
        needShadow = 0;
    } else if (button->disabled) {
        needShadow = 0;
    }

    if(needShadow) {
        C2D_DrawSpriteTinted(&button->sprite, &shadowTint);
        button->sprite.params.pos.x -= distance;
        button->sprite.params.pos.y -= distance;
    }

    if(button->rainbow != 0 && button->disabled == 0) {
        C2D_DrawSpriteTinted(&button->sprite, &rainbowTint);
    } else {
        C2D_DrawSprite(&button->sprite);
    }

    C2D_Text drawText;
    C2D_TextBuf textBuffer = C2D_TextBufNew(MAX_TEXT_SIZE);
    C2D_TextBufClear(textBuffer);
    C2D_TextFontParse(&drawText, *button->font, textBuffer, button->text);
    C2D_TextOptimize(&drawText);
    C2D_DrawText(&drawText, C2D_WithColor,
                 button->sprite.params.pos.x + button->textOffsetX,
                 button->sprite.params.pos.y + button->textOffsetY,
                 0, button->textScale, button->textScale, C2D_Color32(0xFB, 0xFC, 0xFC, 0xFF));
    C2D_TextBufDelete(textBuffer);

    if(button->disabled) {
        C2D_DrawSpriteTinted(&button->sprite, &disabledTint);
    }

    if(needShadow) {
        button->sprite.params.pos.x += distance;
        button->sprite.params.pos.y += distance;
    }
}
