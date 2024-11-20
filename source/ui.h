#ifndef UI_BUTTONS_H
#define UI_BUTTONS_H

#include <3ds.h>
#include <citro2d.h>

// Forward declare AppState and UIButton
typedef struct AppState AppState;
typedef struct UIButton UIButton;

// Enums
enum Scene {
    ROOT,
    EDIT,
    BOOK
};

// AppState definition
struct AppState {
    int rainbowDelay;
    int rainbowDelayDefault;
    enum Scene scene;
    touchPosition pressedCoords;
    u32 colors[128];
    u32 backgroundColorBottom;
    u32 backgroundColorTop;
    u32 *fileSystemHandle;
    char **nameList;
    int fileCount;
};

// UIButton definition
typedef void (*ButtonCallback)(AppState*, touchPosition);

struct UIButton {
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
};

// External variables
extern C2D_ImageTint rainbowTint;
extern C2D_ImageTint shadowTint;
extern C2D_ImageTint disabledTint;

// Function declarations
void updateRainbowTint(C2D_ImageTint* tint, AppState* appState);
void drawButton(UIButton* button);
void drawText(int x, int y, int z, float scale, u32 color, char* text, int flags, C2D_Font font);
UIButton* initButtons(int* buttonCount);

// Callback functions
void addressBookCallback(AppState* mainState, touchPosition touch);
void addressConnectCallback(AppState* mainState, touchPosition touch);
void buttonEditCallback(AppState* mainState, touchPosition touch);

#endif // UI_BUTTONS_H
