#pragma once

#include "main.h"

typedef MAP CTile;
typedef BASE CCity;

#pragma pack(push, 1)
struct CFile {
    int pvFileBuffer;
    int hFile;
    int hFileMapping;
    int iFileSize;
};

struct CMemAllocator {
    int iFlags;
    int pszStart;
    char *pszPosition;
    int iSize;
    int iSpaceLeft;
};

struct CLabelAllocator {
    CMemAllocator oMemAllocator;
    int fAllocated;
};

struct CHitBox {
    RECT rTest;
    int iHitBoxID;
    int iTag;
};

struct CHitBoxList {
    CHitBox *paHitBoxes;
    int iArraySize;
    int iCount;
};

struct CPalette {
    tagPALETTEENTRY aColors[256];
};

struct CListItem {
    int field_0;
    void *pItem;
    int p4Alloced;
    CListItem *pPrev;
    CListItem *pNext;
    int field_14;
    int field_18;
};

struct CFont {
    int field_0;
    int iFlags;
    HGDIOBJ hgdiobjFont;
    int iLineHeight;
    int iHeight;
    int iInternalLeading;
    int iAscent;
    int iDescent;
    int field_20;
    char *pszFileName;
};

struct CTimer {
    int iFlags;
    int iResult;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int iElapse;
    int fFired;
    int iResolution;
    int field_24;
};

struct _PcxHeader {
    BYTE Identifier;
    BYTE Version;
    BYTE Encoding;
    BYTE BitsPerPixel;
    WORD XStart;
    WORD YStart;
    WORD XEnd;
    WORD YEnd;
    WORD HorzRes;
    WORD VertRes;
    BYTE Palette[48];
    BYTE Reserved1;
    BYTE NumBitPlanes;
    WORD BytesPerLine;
    WORD PaletteType;
    WORD HorzScreenSize;
    WORD VertScreenSize;
    BYTE Reserved2[54];
};

struct CWinBase;

struct CCanvas {
    void *vtblCPCX;
    CWinBase *poOwner;
    __int32 (__cdecl *pfcnScrollText)(char *pszText, __int32 x, int y, int iCharsToScroll);
    DWORD dwordC;
    DWORD dword10;
    DWORD dword14;
    DWORD dword18;
    DWORD iFlags;
    void *pstRect[12];
    char *pcSurfaceBits;
    char *pcDIBBits;
    void **ppddsSurface;
    void **ppddcClipper;
    HDC hdcSurface;
    HDC hdcCompatible;
    DWORD iHdcSurfaceRefCounter;
    DWORD iDDSSurfaceRefCount;
    DWORD hrgnMainRegion;
    HBITMAP hbmBitmapOld;
    HBITMAP hbmBitmap;
    BITMAPINFO stBitMapInfo;
    int dib[256];
    DWORD lPitch;
    int iHyperlinkCount;
    CHitBoxList oHitBoxList;
    void *apHeapMem[20];
    DWORD dword50C;
    DWORD dword510;
    DWORD dword514;
    DWORD dword518;
    DWORD iParseCharType;
    DWORD dword520;
    DWORD dword524;
    DWORD dword528;
    CFont *font1;
    CFont *font2;
    CFont *font3;
    CFont *font4;
    DWORD colorVal1;
    DWORD color2Val1;
    DWORD color3Val1;
    DWORD colorhyperVal1;
    DWORD colorVal2;
    DWORD color2Val2;
    DWORD color3Val2;
    DWORD colorhyperVal2;
    DWORD colorVal3;
    DWORD color2Val3;
    DWORD color3Val3;
    DWORD colorhyperVal3;
    DWORD colorVal4;
    DWORD color2Val4;
    DWORD color3Val4;
    DWORD colorhyperVal4;
    DWORD fHasPalette;
    int cPaletteOffset;
    CPalette *pPalette;
};

struct CSprite {
    void *ppszFileName;
    char *pcBits;
    BYTE cTransparentIndex;
    BYTE f9[3];
    DWORD iSpriteWidth2;
    DWORD iSpriteWidth;
    int iSpriteHeight;
    int iWidth;
    int iHeight;
    int iLeftOffset;
    int iTopOffset;
    int fObj1Exists;
};

struct CImage {
    char *pcBits;
    DWORD iWidth;
    DWORD iHeight;
    DWORD dwordC;
    DWORD dword10;
    DWORD iWidthLess1;
    DWORD dword18;
    DWORD dword1C;
    DWORD iHeightLess1;
    DWORD dword24;
    DWORD iHeightLess1_2;
    DWORD dword2C;
    DWORD dword30;
    DWORD dword34;
    DWORD dword38;
    DWORD iWidthLess1_2;
    DWORD dword40;
    DWORD iWidthLess1_3;
    DWORD iCenterY;
    DWORD iWidthLess1_4;
    DWORD iHeightLess1_3;
    DWORD iCenterX;
    DWORD iHeightLess1_4;
    DWORD dword5C;
    DWORD iHeightLess1_5;
    DWORD dword64;
    DWORD iCenterY_2;
    DWORD fNotMalloced;
};

struct CMainWnd {
    HWND hwndScreen;
    void **ppddDirectDraw;
    int dword_9C6B20;
    void **ppdsSurface;
    RECT *prScreenRect;
};

struct CClass0A {
    void *vtbl;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    int field_48;
    int field_4C;
    int field_50;
    int field_54;
    int field_58;
    int field_5C;
    int field_60;
    int field_64;
    int field_68;
    int field_6C;
    int field_70;
    int field_74;
    int field_78;
    int field_7C;
    int field_80;
    int field_84;
    int field_88;
    int field_8C;
    int field_90;
    int field_94;
};

struct CList {
    int vtbl;
    CListItem *pFirst;
    CListItem *pCurrent;
    int iCount;
    int iCurrent;
    CMemAllocator *poMemAllocator;
};

struct CWinBuffed;

struct CWinBase {
    CClass0A oClass0A;
    int iFlags;
    int iSomeFlag;
    int field_A0;
    int field_A4;
    CWinBase *poWinBase;
    int iVertScaleDenom;
    int iVertScaleNum;
    CCanvas *pCanvas1;
    CCanvas *pCanvas2;
    CCanvas *pCanvas3;
    CCanvas *pCanvas4;
    CWinBase *poParent;
    CList oChildList;
    char *pszCaption;
    int field_E4;
    int field_E8;
    int field_EC;
    int field_F0;
    int iBackbufferBackgroundColor;
    int field_F8;
    int pCanvasBackground;
    int cBackgroundColor;
    int field_104;
    int field_108;
    int field_10C;
    int field_110;
    int field_114;
    int iBorderSize;
    int fHasBorder;
    int iBorderColor1;
    int iBorderColor2;
    int field_128;
    CCanvas *pCanvasBackBuffer;
    int field_130;
    int iMinPosLeft;
    int iMinPosTop;
    RECT rRect1;
    RECT rRect2;
    int field_15C;
    int field_160;
    int field_164;
    int field_168;
    int iCaptionSize;
    int iNCSize;
    int field_174;
    int field_178;
    int field_17C;
    int field_180;
    int field_184;
    int field_188;
    int field_18C;
    int field_190;
    int field_194;
    int field_198;
    int field_19C;
    int iSomeFlag2;
    CWinBase *apoChildren[150];
    int iChildCount;
    int field_400;
    int field_404;
    int field_408;
    int field_40C;
    int field_410;
    int field_414;
    int field_418;
    int field_41C;
    int field_420;
    int field_424;
    int field_428;
    int field_42C;
    int field_430;
    int field_434;
    int field_438;
    CWinBuffed *poWinBuffed1;
    CWinBuffed *poWinBuffed2;
};

struct CWinBuffed {
    CWinBase oWinBase;
    CCanvas oCanvas;
    int field_9CC;
    int field_9D0;
    int field_9D4;
    int field_9D8;
    int field_9DC;
    int field_9E0;
    int field_9E4;
    int field_9E8;
    int field_9EC;
    int field_9F0;
    int field_9F4;
    int field_9F8;
    int field_9FC;
    int field_A00;
    int field_A04;
    CCanvas *poCanvas;
    int field_A0C;
    int field_A10;
};

struct CWinFonted;

struct CClass3ArrayItem {
    int field_0;
    int pszCaption;
    int iFlags;
    int field_C;
    CWinFonted *poWinFonted;
};

struct CWinFonted {
    CWinBuffed oWinBuffed;
    int field_A14;
    int iArrayCount;
    int field_A1C;
    int field_A20;
    int iHitBoxTagClicked;
    int field_A28;
    CHitBoxList oHitBoxList;
    CClass3ArrayItem astArray[15];
};

struct MapVtbl {
    int field_0;
    CWinBuffed *iOffsetofoClass2;
};

struct TTilePos {
    char field_0;
    char field_1;
    char field_2;
    char field_3;
    char field_4;
    char field_5;
    char field_6;
    char field_7;
    char field_8;
    char field_9;
    char field_A;
    char field_B;
};

struct CMainArrayItem {
    int field_0;
    int field_4;
    int field_8[150];
};

struct CClass3B {
    CWinBuffed oWinBuffed;
    int field_A14;
    int field_A18;
    CTimer oTimer1;
    int field_A44;
    int field_A48;
    CTimer oTimer2;
    int field_A74;
    int field_A78;
    int field_A7C;
    int field_A80;
    int field_A84;
    int field_A88;
    int field_A8C;
    int field_A90;
    int field_A94;
    int field_A98;
    int field_A9C;
    int field_AA0;
    int field_AA4;
    int field_AA8;
    int field_AAC;
    int field_AB0;
    int field_AB4;
    int field_AB8;
};

struct CMap {
    MapVtbl *vtbl;
    TTilePos *paTilePos;
    int field_8;
    CMainArrayItem aoMainArrayItem1[4];
    CMainArrayItem aoMainArrayItem2[196];
    CMainArrayItem oUnknown;
    int iZoomX2;
    int iWhatToDrawFlags;
    int field_1DD74;
    int field_1DD78;
    int field_1DD7C;
    int field_1DD80;
    int field_1DD84;
    int field_1DD88;
    int field_1DD8C;
    int field_1DD90;
    int field_1DD94;
    int iZoomFactor;
    int iTileX;
    int iTileY;
    int iMapTileLeft;
    int iMapTileTop;
    int iMapPixelLeft;
    int iMapPixelTop;
    int iPixelsPerTileX;
    int iPixelsPerHalfTileX;
    int iPixelsPerTileY;
    int iPixelsPerHalfTileY_2;
    int iPixelsPerHalfTileY;
    int iZoomX12_2;
    int iMapTilesOddX;
    int iMapTilesOddY;
    int iMapTilesEvenX;
    int iMapTilesEvenY;
    int iZoomX12;
    int iZoomX4;
    int field_1DDE4;
    int field_1DDE8;
    int field_1DDEC;
    int field_1DDF0;
    int field_1DDF4;
    int field_1DDF8;
    int field_1DDFC;
    int field_1DE00;
    int field_1DE04;
    int field_1DE08;
    int field_1DE0C;
    int field_1DE10;
    int field_1DE14;
    int field_1DE18;
    int field_1DE1C;
    int field_1DE20;
    int field_1DE24;
    CCanvas oCanvas1;
    CCanvas oCanvas2;
    CCanvas oCanvas3;
    int iLastZoomFactor;
    int iLastZoomX2;
    CFont oFont1;
    int iFont1Height;
    CFont oFont2;
    int iFont4Height;
    CFont oFont3;
    int iFont3Height;
    int field_1EF4C;
    int field_1EF50;
    CClass3B oaClass3B[4];
    int drawOnlyCursor;
    int field_21A48;
    int field_21A4C;
    int field_21A50;
    int field_21A54;
    int field_21A58;
    int field_21A5C;
    int field_21A60;
    int field_21A64;
};

struct CMenuItem {
    int pszCaption;
    int pszHotKey;
    int iMenuID;
    int iFlag;
    int field_10;
};

struct CMenu {
    CWinBuffed field_0;
    int field_A14;
    CMenuItem aMenuItems[64];
    int field_F18;
    int field_F1C;
    int iMenuItemCount;
    int iWidth;
    int field_F28;
    int iVisibleItemCount;
    int field_F30;
    int field_F34;
    int field_F38;
    int field_F3C;
};

struct CMainMenuItem {
    int iMenuItemID;
    int pszCaption;
    int iFlags;
    int pszHotKey;
    CMenu *poSubMenu;
};

typedef int(__cdecl *MENU_HANDLER_CB_F)(int iMenuItemId);

struct CMainMenu {
    CWinBuffed oWinBuffed;
    MENU_HANDLER_CB_F pMenuHandlerCB;
    int iBaseMenuItemCount;
    int field_A1C;
    int field_A20;
    int iHitBoxTagClicked;
    int field_A28;
    CHitBoxList oHitBoxList;
    CMainMenuItem aMainMenuItems[15];
};

struct CMain {
    CMap oMap;
    CWinBuffed oWinBuffed;
    CCanvas oCanvas1;
    CTimer oTimer;
    CMainMenu oMainMenu;
    CCanvas oCanvas2;
    CSprite oSprite1;
    CSprite aoSprite2[3];
    int field_23BC8;
    int field_23BCC;
    int field_23BD0;
    int cOwner;
    int fUnitNotViewMode;
    int iUnit;
    int field_23BE0;
    int field_23BE4;
    int field_23BE8;
    int field_23BEC;
    int field_23BF0;
    int field_23BF4;
    int field_23BF8;
    int field_23BFC;
    int field_23C00;
    int field_23C04;
    int iCursorPositionCurrent;
    int field_23C0C;
    int aiCursorPositionsX[32];
    int aiCursorPositionsY[32];
    int field_23D10;
    int field_23D14;
    int field_23D18;
    int field_23D1C;
    int field_23D20;
    int field_23D24;
    CSprite oSprite3;
    CSprite oSPrite4;
    int field_23D80;
    int field_23D84;
    int field_23D88;
    int field_23D8C;
    CWinBuffed oClass2_2;
};

struct CClass5 {
    CMap oMap;
    int field_21A68;
    CWinBuffed oClass2;
};

struct CKeyFileIndex {
    char szFileName[256];
    char *pszSections;
    int field_104;
    __int32 iSectionCount;
    int field_10C;
    int field_110;
    int field_114;
};

struct CKeyFile {
    char szFileName[80];
    char szSomeString[256];
    char *pszValues;
    FILE *pFile;
    char *pszFileBuffer;
    int pszNextValue;
};

struct CInfoWin {
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
    int field_30[1332];
    int field_1500;
    int field_1504;
    int field_1508;
    int field_150C;
    int field_1510;
    int field_1514;
    int field_1518;
    int field_151C;
    int field_1520;
    int field_1524;
    int field_1528;
    int field_152C;
    CFont oFontNorm;
    CFont oFontItal;
    CFont oFontBold;
    int iTileInfoTab;
    int iFontHeight;
    int field_15B0;
    int iTileX;
    int iTileY;
    int iUnitIndex;
    int iCityIndex;
    int field_15C4;
    int field_15C8;
    int field_15CC;
    int field_15D0;
    int field_15D4;
    int fInOnClick;
    int field_15DC;
    CHitBoxList oHitBoxList;
};
#pragma pack(pop)

static_assert(sizeof(CHitBox) == 24, "");
static_assert(sizeof(CHitBoxList) == 12, "");
static_assert(sizeof(CFont) == 40, "");
static_assert(sizeof(CTimer) == 40, "");
static_assert(sizeof(CSprite) == 44, "");
static_assert(sizeof(CMainArrayItem) == 608, "");
static_assert(sizeof(CWinBase) == 1092, "");
static_assert(sizeof(CCanvas) == 1416, "");
static_assert(sizeof(CMainMenu) == 2916, "");
static_assert(sizeof(CWinBuffed) == 2580, "");
static_assert(sizeof(CMap) == 137832, "");
static_assert(sizeof(CMain) == 149412, "");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

ATOM WINAPI ModRegisterClassA(WNDCLASS* pstWndClass);
int WINAPI ModGetSystemMetrics(int nIndex);
int __cdecl blink_timer();
int __thiscall zoom_process(CMain* This);
int __thiscall draw_map(CMain* This, int iOwner, int fUnitsOnly);

#pragma GCC diagnostic pop



