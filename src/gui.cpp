
#include "gui.h"

const int32_t MainWinHandle = (int32_t)(&MapWin->oMainWin.oWinBase.field_4); // 0x939444

char label_pop_size[StrBufLen] = "Pop: %d / %d / %d / %d";
char label_pop_boom[StrBufLen] = "Population Boom";
char label_nerve_staple[StrBufLen] = "Nerve Staple: %d turns";
char label_captured_base[StrBufLen] = "Captured Base: %d turns";
char label_stockpile_energy[StrBufLen] = "Stockpile: %d per turn";
char label_sat_nutrient[StrBufLen] = "N +%d";
char label_sat_mineral[StrBufLen] = "M +%d";
char label_sat_energy[StrBufLen] = "E +%d";
char label_eco_damage[StrBufLen] = "";
char label_base_surplus[StrBufLen] = "";
char label_unit_reactor[4][StrBufLen] = {};

static int minimal_cost = 0;
static int base_zoom_factor = -14;

struct ConsoleState {
    const int ScrollMin = 1;
    const int ScrollMax = 20;
    const int ListScrollDelta = 1;
    POINT ScreenSize;
    bool MouseOverTileInfo = true;
    bool Scrolling = false;
    bool RightButtonDown = false;
    bool ScrollDragging = false;
    double ScrollOffsetX = 0.0;
    double ScrollOffsetY = 0.0;
    POINT ScrollDragPos = {0, 0};
} CState;

/*
The following lists contain definitions copied directly from PRACX (e.g. functions with _F suffix).
These are mostly provided for reference and using them should be avoided because the names should be
converted to the actual names reversed from the SMACX binary (add F prefix for function prototypes).
*/

typedef int(__stdcall *START_F)(HINSTANCE, HINSTANCE, LPSTR, int);
typedef int(__thiscall *CCANVAS_CREATE_F)(Buffer* This);
typedef int(__stdcall *WNDPROC_F)(HWND, int, WPARAM, LPARAM);
typedef int(__thiscall *CMAIN_ZOOMPROCESSING_F)(Console* This);
typedef int(__stdcall *PROC_ZOOM_KEY_F)(int iZoomType, int iZero);
typedef int(__thiscall *CMAIN_TILETOPT_F)(Console* This, int iTileX, int iTileY, long* piX, long* piY);
typedef int(__thiscall *CMAIN_MOVEMAP_F)(Console* This, int iXPos, int iYPos, int a4);
typedef int(__thiscall *CMAIN_REDRAWMAP_F)(Console* This, int a2);
typedef int(__thiscall *CMAIN_DRAWMAP_F)(Console* This, int iOwner, int fUnitsOnly);
typedef int(__thiscall *CMAIN_PTTOTILE_F)(Console* This, POINT p, long* piTileX, long* piTileY);
typedef int(__thiscall *CINFOWIN_DRAWTILEINFO_F)(CInfoWin* This);
typedef int(__cdecl *PAINTHANDLER_F)(RECT *prRect, int a2);
typedef int(__cdecl *PAINTMAIN_F)(RECT *pRect);
typedef int(__thiscall *CSPRITE_FROMCANVASRECTTRANS_F)(Sprite* This, Buffer *poCanvas,
    int iTransparentIndex, int iLeft, int iTop, int iWidth, int iHeight, int a8);
typedef int(__thiscall *CCANVAS_DESTROY4_F)(Buffer* This);
typedef int (__thiscall *CSPRITE_STRETCHCOPYTOCANVAS_F)
    (Sprite* This, Buffer *poCanvasDest, int cTransparentIndex, int iLeft, int iTop);
typedef int(__thiscall *CSPRITE_STRETCHCOPYTOCANVAS1_F)
    (Sprite* This, Buffer *poCanvasDest, int cTransparentIndex, int iLeft, int iTop, int iDestScale, int iSourceScale);
typedef int (__thiscall *CSPRITE_STRETCHDRAWTOCANVAS2_F)
    (Sprite* This, Buffer *poCanvas, int a1, int a2, int a3, int a4, int a7, int a8);
typedef int(__thiscall *CWINBASE_ISVISIBLE_F)(Win* This);
typedef int(__thiscall *CTIMER_STARTTIMER_F)(Time* This, int a2, int a3, int iDelay, int iElapse, int uResolution);
typedef int(__thiscall *CTIMER_STOPTIMER_F)(Time* This);
typedef int(__thiscall *DRAWCITYMAP_F)(Win* This, int a2);
typedef int(__cdecl *GETFOODCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithFarm);
typedef int(__cdecl *GETPRODCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithMine);
typedef int(__cdecl *GETENERGYCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithSolar);
typedef int(__thiscall *IMAGEFROMCANVAS_F)(CImage* This, Buffer *poCanvasSource, int iLeft, int iTop, int iWidth, int iHeight, int a7);
typedef int(__cdecl *GETELEVATION_F)(int iTileX, int iTileY);
typedef int(__thiscall *CIMAGE_COPYTOCANVAS2_F)(CImage* This, Buffer *poCanvasDest, int x, int y, int a5, int a6, int a7);
typedef int(__thiscall *CMAINMENU_ADDSUBMENU_F)(Menu* This, int iMenuID, int iMenuItemID, char *lpString);
typedef int(__thiscall *CMAINMENU_ADDBASEMENU_F)(Menu* This, int iMenuItemID, const char *pszCaption, int a4);
typedef int(__thiscall *CMAINMENU_ADDSEPARATOR_F)(Menu* This, int iMenuItemID, int iSubMenuItemID);
typedef int(__thiscall *CMAINMENU_UPDATEVISIBLE_F)(Menu* This, int a2);
typedef int(__thiscall *CMAINMENU_RENAMEMENUITEM_F)(Menu* This, int iMenuItemID, int iSubMenuItemID, const char *pszCaption);
typedef long(__thiscall *CMAP_GETCORNERYOFFSET_F)(Console* This, int iTileX, int iTileY, int iCorner);

START_F                        pfncWinMain =                    (START_F                       )0x45F950;
HDC*                           phDC =                           (HDC*                          )0x9B7B2C;
HWND*                          phWnd =                          (HWND*                         )0x9B7B28;
HPALETTE*                      phPallete =                      (HPALETTE*                     )0x9B8178;
HINSTANCE*                     phInstance =                     (HINSTANCE*                    )0x9B7B14;
WNDPROC_F                      pfncWinProc =                    (WNDPROC_F                     )0x5F0650;
CMAIN_ZOOMPROCESSING_F         pfncZoomProcessing =             (CMAIN_ZOOMPROCESSING_F        )0x462980;
Console*                       pMain =                          (Console*                      )0x9156B0;
int*                           piMaxTileX =                     (int*                          )0x949870;
int*                           piMaxTileY =                     (int*                          )0x949874;
PROC_ZOOM_KEY_F                pfncProcZoomKey =                (PROC_ZOOM_KEY_F               )0x5150D0;
CMAIN_TILETOPT_F               pfncTileToPoint =                (CMAIN_TILETOPT_F              )0x462F00;
RECT*                          prVisibleTiles =                 (RECT*                         )0x7D3C28;
int*                           piMapFlags =                     (int*                          )0x94988C;
CMAIN_MOVEMAP_F                pfncMoveMap =                    (CMAIN_MOVEMAP_F               )0x46B1F0;
CMAIN_REDRAWMAP_F              pfncRedrawMap =                  (CMAIN_REDRAWMAP_F             )0x46A550;
CMAIN_DRAWMAP_F                pfncDrawMap =                    (CMAIN_DRAWMAP_F               )0x469CA0;
CMAIN_PTTOTILE_F               pfncPtToTile =                   (CMAIN_PTTOTILE_F              )0x463040;
CInfoWin*                      pInfoWin =                       (CInfoWin*                     )0x8C5568;
CINFOWIN_DRAWTILEINFO_F        pfncDrawTileInfo =               (CINFOWIN_DRAWTILEINFO_F       )0x4B8890; // Fixed
Console**                      ppMain =                         (Console**                     )0x7D3C3C;
PAINTHANDLER_F                 pfncPaintHandler =               (PAINTHANDLER_F                )0x5F7320;
PAINTMAIN_F                    pfncPaintMain =                  (PAINTMAIN_F                   )0x5EFD20;
CSPRITE_FROMCANVASRECTTRANS_F  pfncSpriteFromCanvasRectTrans =  (CSPRITE_FROMCANVASRECTTRANS_F )0x5E39A0;
Buffer*                        poLoadingCanvas =                (Buffer*                       )0x798668;
CCANVAS_DESTROY4_F             pfncCanvasDestroy4 =             (CCANVAS_DESTROY4_F            )0x5D7470;
CSPRITE_STRETCHCOPYTOCANVAS_F  pfncSpriteStretchCopyToCanvas =  (CSPRITE_STRETCHCOPYTOCANVAS_F )0x5E4B9A;
CSPRITE_STRETCHCOPYTOCANVAS1_F pfncSpriteStretchCopyToCanvas1 = (CSPRITE_STRETCHCOPYTOCANVAS1_F)0x5E4B4A;
CSPRITE_STRETCHDRAWTOCANVAS2_F pfncSpriteStretchDrawToCanvas2 = (CSPRITE_STRETCHDRAWTOCANVAS2_F)0x5E3E00;
Sprite*                        pSprResourceIcons =              (Sprite*                       )0x7A72A0;
Win*                           pCityWindow =                    (Win*                          )0x6A7628;
Win*                           pAnotherWindow =                 (Win*                          )0x8A6270;
Win*                           pAnotherWindow2 =                (Win*                          )0x8C6E68;
int*                           pfGameNotStarted =               (int*                          )0x68F21C;
CWINBASE_ISVISIBLE_F           pfncWinIsVisible =               (CWINBASE_ISVISIBLE_F          )0x5F7E90;
CTIMER_STARTTIMER_F            pfncStartTimer =                 (CTIMER_STARTTIMER_F           )0x616350;
CTIMER_STOPTIMER_F             pfncStopTimer =                  (CTIMER_STOPTIMER_F            )0x616780;
DRAWCITYMAP_F                  pfncDrawCityMap =                (DRAWCITYMAP_F                 )0x40F0F0;
int*                           piZoomNum =                      (int*                          )0x691E6C;
int*                           piZoomDenom =                    (int*                          )0x691E70;
int*                           piSourceScale =                  (int*                          )0x696D1C;
int*                           piDestScale =                    (int*                          )0x696D18;
int*                           piResourceExtra =                (int*                          )0x90E998;
int*                           piTilesPerRow =                  (int*                          )0x68FAF0;
CTile**                        paTiles =                        (CTile**                       )0x94A30C;
GETFOODCOUNT_F                 pfncGetFoodCount =               (GETFOODCOUNT_F                )0x4E6E50;
GETPRODCOUNT_F                 pfncGetProdCount =               (GETPRODCOUNT_F                )0x4E7310;
GETENERGYCOUNT_F               pfncGetEnergyCount =             (GETENERGYCOUNT_F              )0x4E7750;
IMAGEFROMCANVAS_F              pfncImageFromCanvas =            (IMAGEFROMCANVAS_F             )0x619710;
GETELEVATION_F                 pfncGetElevation =               (GETELEVATION_F                )0x5919C0;
CIMAGE_COPYTOCANVAS2_F         pfncImageCopytoCanvas2 =         (CIMAGE_COPYTOCANVAS2_F        )0x6233C0;
CMAINMENU_ADDSUBMENU_F         pfncMainMenuAddSubMenu =         (CMAINMENU_ADDSUBMENU_F        )0x5FB100;
CMAINMENU_ADDBASEMENU_F        pfncMainMenuAddBaseMenu =        (CMAINMENU_ADDBASEMENU_F       )0x5FAEF0;
CMAINMENU_ADDSEPARATOR_F       pfncMainMenuAddSeparator =       (CMAINMENU_ADDSEPARATOR_F      )0x5FB160;
CMAINMENU_UPDATEVISIBLE_F      pfncMainMenuUpdateVisible =      (CMAINMENU_UPDATEVISIBLE_F     )0x460DD0;
CMAINMENU_RENAMEMENUITEM_F     pfncMainMenuRenameMenuItem =     (CMAINMENU_RENAMEMENUITEM_F    )0x5FB700;
CMAP_GETCORNERYOFFSET_F        pfncMapGetCornerYOffset =        (CMAP_GETCORNERYOFFSET_F       )0x46FE70;

// End of PRACX definitions


int __thiscall Win_is_visible(Win* This) {
    bool value = (This->iSomeFlag & WIN_VISIBLE)
        && (!This->poParent || Win_is_visible(This->poParent));
    return value;
}

/*
Returns true only when the world map is visible and has focus
and other large modal windows are not blocking it.
Other modal windows with the exception of BaseWin are already
covered by checking Win_get_key_window condition.
*/
static GameWinState current_window() {
    if (!*GameHalted) {
        int state = Win_get_key_window();
        if (state == MainWinHandle) {
            return Win_is_visible(BaseWin) ? GW_Base : GW_World;
        } else if (state == (int)DesignWin) {
            return GW_Design;
        }
    }
    return GW_None;
}

static bool win_has_focus() {
    return GetFocus() == *phWnd;
}

static bool alt_key_down() {
    return GetAsyncKeyState(VK_MENU) < 0;
}

static bool ctrl_key_down() {
    return GetAsyncKeyState(VK_CONTROL) < 0;
}

static void base_resource_zoom(bool zoom_in) {
    base_zoom_factor = clamp(base_zoom_factor + (zoom_in ? 2 : -2), -14, 0);
    GraphicWin_redraw(BaseWin);
}

void mouse_over_tile(POINT* p) {
    static POINT ptLastTile = {0, 0};
    POINT ptTile;

    if (CState.MouseOverTileInfo
    && !MapWin->fUnitNotViewMode
    && p->x >= 0 && p->x < CState.ScreenSize.x
    && p->y >= 0 && p->y < (CState.ScreenSize.y - ConsoleHeight)
    && MapWin_pixel_to_tile(MapWin, p->x, p->y, &ptTile.x, &ptTile.y) == 0
    && memcmp(&ptTile, &ptLastTile, sizeof(POINT)) != 0) {

        pInfoWin->iTileX = ptTile.x;
        pInfoWin->iTileY = ptTile.y;
        StatusWin_on_redraw((Win*)pInfoWin);
        memcpy(&ptLastTile, &ptTile, sizeof(POINT));
    }
}

ULONGLONG get_ms_count() {
    static LONGLONG llFrequency = 0;
    static ULONGLONG ullLast = 0;
    static ULONGLONG ullHigh = 0;
    ULONGLONG ullRet;
    if (llFrequency == 0 && !QueryPerformanceFrequency((LARGE_INTEGER*)&llFrequency)) {
        llFrequency = -1LL;
    }
    if (llFrequency > 0) {
        QueryPerformanceCounter((LARGE_INTEGER*)&ullRet);
        ullRet *= 1000;
        ullRet /= (ULONGLONG)llFrequency;
    } else if (llFrequency < 0) {
        ullRet = GetTickCount();
        if (ullRet < ullLast) {
            ullHigh += 0x100000000ULL;
        }
        ullLast = ullRet;
        ullRet += ullHigh;
    }
    return ullRet;
}

bool do_scroll(double x, double y) {
    bool fScrolled = false;
    int mx = *MapAreaX;
    int my = *MapAreaY;
    int i;
    int d;
    if (x && MapWin->iMapTilesEvenX + MapWin->iMapTilesOddX < mx) {
        if (x < 0 && (!map_is_flat() || MapWin->iMapTileLeft > 0)) {
            i = (int)CState.ScrollOffsetX;
            CState.ScrollOffsetX -= x;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetX);
            while (CState.ScrollOffsetX >= MapWin->iPixelsPerTileX) {
                CState.ScrollOffsetX -= MapWin->iPixelsPerTileX;
                MapWin->iTileX -= 2;
                if (MapWin->iTileX < 0) {
                    if (map_is_flat()) {
                        MapWin->iTileX = 0;
                        MapWin->iTileY &= ~1;
                        CState.ScrollOffsetX = 0;
                    } else {
                        MapWin->iTileX += mx;
                    }
                }
            }
        } else if (x < 0 && map_is_flat()) {
            fScrolled = true;
            CState.ScrollOffsetX = 0;
        }
        if (x > 0 &&
                (!map_is_flat() ||
                 MapWin->iMapTileLeft +
                 MapWin->iMapTilesEvenX +
                 MapWin->iMapTilesOddX <= mx)) {
            i = (int)CState.ScrollOffsetX;
            CState.ScrollOffsetX -= x;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetX);
            while (CState.ScrollOffsetX <= -MapWin->iPixelsPerTileX) {
                CState.ScrollOffsetX += MapWin->iPixelsPerTileX;
                MapWin->iTileX += 2;
                if (MapWin->iTileX > mx) {
                    if (map_is_flat()) {
                        MapWin->iTileX = mx;
                        MapWin->iTileY &= ~1;
                        CState.ScrollOffsetX = 0;
                    } else {
                        MapWin->iTileX -= mx;
                    }
                }
            }
        } else if (x > 0 && map_is_flat()) {
            fScrolled = true;
            CState.ScrollOffsetX = 0;
        }
    }
    if (y && MapWin->iMapTilesEvenY + MapWin->iMapTilesOddY < my) {
        int iMinTileY = MapWin->iMapTilesOddY - 2;
        int iMaxTileY = my + 4 - MapWin->iMapTilesOddY;
        while (MapWin->iTileY < iMinTileY) {
            MapWin->iTileY += 2;
        }
        while (MapWin->iTileY > iMaxTileY) {
            MapWin->iTileY -= 2;
        }
        d = (MapWin->iTileY - iMinTileY) * MapWin->iPixelsPerHalfTileY - (int)CState.ScrollOffsetY;
        if (y < 0 && d > 0 ) {
            if (y < -d)
                y = -d;
            i = (int)CState.ScrollOffsetY;
            CState.ScrollOffsetY -= y;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetY);
            while (CState.ScrollOffsetY >= MapWin->iPixelsPerTileY && MapWin->iTileY - 2 >= iMinTileY) {
                CState.ScrollOffsetY -= MapWin->iPixelsPerTileY;
                MapWin->iTileY -= 2;
            }
        }
        d = (iMaxTileY - MapWin->iTileY + 1) * MapWin->iPixelsPerHalfTileY + (int)CState.ScrollOffsetY;
        if (y > 0 && d > 0) {
            if (y > d)
                y = d;
            i = (int)CState.ScrollOffsetY;
            CState.ScrollOffsetY -= y;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetY);
            while (CState.ScrollOffsetY <= -MapWin->iPixelsPerTileY && MapWin->iTileY + 2 <= iMaxTileY) {
                CState.ScrollOffsetY += MapWin->iPixelsPerTileY;
                MapWin->iTileY += 2;
            }
        }
    }
    if (fScrolled) {
        MapWin_draw_map(MapWin, 0);
        Win_update_screen(NULL, 0);
        Win_flip(NULL);
        ValidateRect(*phWnd, NULL);
    }
    return fScrolled;
}

void check_scroll() {
    POINT p;
    if (CState.Scrolling || (!GetCursorPos(&p) && !(CState.ScrollDragging && CState.RightButtonDown))) {
        return;
    }
    CState.Scrolling = true;
    int w = CState.ScreenSize.x;
    int h = CState.ScreenSize.y;
    static ULONGLONG ullDeactiveTimer = 0;
    ULONGLONG ullOldTickCount = 0;
    ULONGLONG ullNewTickCount = 0;
    BOOL fScrolled;
    BOOL fScrolledAtAll = false;
    BOOL fLeftButtonDown = (GetAsyncKeyState(VK_LBUTTON) < 0);
    int iScrollArea = conf.scroll_area * CState.ScreenSize.x / 1024;

    if (CState.RightButtonDown && GetAsyncKeyState(VK_RBUTTON) < 0) {
        if (labs((long)hypot((double)(p.x-CState.ScrollDragPos.x), (double)(p.y-CState.ScrollDragPos.y))) > 2.5) {
            CState.ScrollDragging = true;
            SetCursor(LoadCursor(0, IDC_HAND));
        }
    }
    CState.ScrollOffsetX = MapWin->iMapPixelLeft;
    CState.ScrollOffsetY = MapWin->iMapPixelTop;
    ullNewTickCount = get_ms_count();
    ullOldTickCount = ullNewTickCount;
//    debug("scroll_check %d %d %d\n", CState.Scrolling, (int)CState.ScrollDragPos.x, (int)CState.ScrollDragPos.y);
    do {
        double dTPS = -1;
        double dx = 0;
        double dy = 0;
        fScrolled = false;
        if (CState.ScrollDragging && CState.RightButtonDown) {
            fScrolled = true;
            dx = CState.ScrollDragPos.x - p.x;
            dy = CState.ScrollDragPos.y - p.y;
            memcpy(&CState.ScrollDragPos, &p, sizeof(POINT));

        } else if (ullNewTickCount - ullDeactiveTimer > 100 && !CState.ScrollDragging) {
            double dMin = (double)CState.ScrollMin;
            double dMax = (double)CState.ScrollMax;
            double dArea = (double)iScrollArea;
            if (p.x <= iScrollArea && p.x >= 0) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)p.x) / dArea * (dMax - dMin);
                dx = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)MapWin->iPixelsPerTileX / -1000.0;

            } else if ((w - p.x) <= iScrollArea && w >= p.x) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)(w - p.x)) / dArea * (dMax - dMin);
                dx = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)MapWin->iPixelsPerTileX / 1000.0;
            }
            if (p.y <= iScrollArea && p.y >= 0) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)p.y) / dArea * (dMax - dMin);
                dy = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)MapWin->iPixelsPerTileY / -1000.0;

            } else if (h - p.y <= iScrollArea && h >= p.y &&
            // These extra conditions will stop movement when the mouse is over the bottom middle console.
            (p.x <= (CState.ScreenSize.x - ConsoleWidth) / 2 ||
             p.x >= (CState.ScreenSize.x - ConsoleWidth) / 2 + ConsoleWidth ||
             h - p.y <= 8 * CState.ScreenSize.y / 768)) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)(h - p.y)) / dArea * (dMax - dMin);
                dy = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)MapWin->iPixelsPerTileY / 1000.0;
            }
        }
        if (fScrolled) {
            ullOldTickCount = ullNewTickCount;
            if (do_scroll(dx, dy)) {
                fScrolledAtAll = true;
            } else {
                Sleep(0);
            }
            if (DEBUG) {
                Sleep(5);
            }
            ullNewTickCount = get_ms_count();
            if (CState.RightButtonDown) {
                CState.RightButtonDown = (GetAsyncKeyState(VK_RBUTTON) < 0);
            }
            debug("scroll_move  x=%d y=%d scx=%.4f scy=%.4f dx=%.4f dy=%.4f dTPS=%.4f\n",
                (int)p.x, (int)p.y, CState.ScrollOffsetX, CState.ScrollOffsetY, dx, dy, dTPS);
        }

    } while (fScrolled && (GetCursorPos(&p) || (CState.ScrollDragging && CState.RightButtonDown)));

    if (fScrolledAtAll) {
        MapWin->drawOnlyCursor = 1;
        MapWin_set_center(MapWin, MapWin->iTileX, MapWin->iTileY, 1);
        MapWin->drawOnlyCursor = 0;
        for (int i = 1; i < 8; i++) {
            if (ppMain[i] && ppMain[i]->iDrawToggleA &&
            (!fLeftButtonDown || ppMain[i]->field_1DD80) &&
            ppMain[i]->iMapTilesOddX + ppMain[i]->iMapTilesEvenX < *MapAreaX) {
                MapWin_set_center(ppMain[i], MapWin->iTileX, MapWin->iTileY, 1);
            }
        }
        if (CState.ScrollDragging) {
            ullDeactiveTimer = ullNewTickCount;
        }
    }
    mouse_over_tile(&p);
    CState.Scrolling = false;
    flushlog();
}

int __thiscall mod_gen_map(Console* This, int iOwner, int fUnitsOnly) {

    if (This == MapWin) {
        // Save these values to restore them later
        int iMapPixelLeft = This->iMapPixelLeft;
        int iMapPixelTop = This->iMapPixelTop;
        int iMapTileLeft = This->iMapTileLeft;
        int iMapTileTop = This->iMapTileTop;
        int iMapTilesOddX = This->iMapTilesOddX;
        int iMapTilesOddY = This->iMapTilesOddY;
        int iMapTilesEvenX = This->iMapTilesEvenX;
        int iMapTilesEvenY = This->iMapTilesEvenY;
        // These are just aliased to save typing and are not modified
        int mx = *MapAreaX;
        int my = *MapAreaY;

        if (iMapTilesOddX + iMapTilesEvenX < mx && !map_is_flat()) {
            if (iMapPixelLeft > 0) {
                This->iMapPixelLeft -= This->iPixelsPerTileX;
                This->iMapTileLeft -= 2;
                This->iMapTilesEvenX++;
                This->iMapTilesOddX++;
                if (This->iMapTileLeft < 0)
                    This->iMapTileLeft += mx;
            } else if (iMapPixelLeft < 0 ) {
                This->iMapTilesEvenX++;
                This->iMapTilesOddX++;
            }
        }
        if (iMapTilesOddY + iMapTilesEvenY < my) {
            if (iMapPixelTop > 0) {
                This->iMapPixelTop -= This->iPixelsPerTileY;
                This->iMapTileTop -= 2;
                This->iMapTilesEvenY++;
                This->iMapTilesOddY++;
            } else if (iMapPixelTop < 0) {
                This->iMapTilesEvenY++;
                This->iMapTilesOddY++;
            }
        }
        MapWin_gen_map(This, iOwner, fUnitsOnly);
        // Restore This's original values
        This->iMapPixelLeft = iMapPixelLeft;
        This->iMapPixelTop = iMapPixelTop;
        This->iMapTileLeft = iMapTileLeft;
        This->iMapTileTop = iMapTileTop;
        This->iMapTilesOddX = iMapTilesOddX;
        This->iMapTilesOddY = iMapTilesOddY;
        This->iMapTilesEvenX = iMapTilesEvenX;
        This->iMapTilesEvenY = iMapTilesEvenY;
    } else {
        MapWin_gen_map(This, iOwner, fUnitsOnly);
    }
    return 0;
}

int __thiscall mod_calc_dim(Console* This) {
    static POINT ptOldTile = {-1, -1};
    POINT ptOldCenter;
    POINT ptNewCenter;
    POINT ptNewTile;
    POINT ptScale;
    int iOldZoom;
    int dx, dy;
    bool fx, fy;
//    int w = ((GraphicWin*)((int)This + This->vtbl[1]))->oCanvas.stBitMapInfo.bmiHeader.biWidth;
//    int h = -((GraphicWin*)((int)This + This->vtbl[1]))->oCanvas.stBitMapInfo.bmiHeader.biHeight;

    if (This == MapWin) {
        iOldZoom = This->iLastZoomFactor;
        ptNewTile.x = This->iTileX;
        ptNewTile.y = This->iTileY;
        fx = (ptNewTile.x == ptOldTile.x);
        fy = (ptNewTile.y == ptOldTile.y);
        memcpy(&ptOldTile, &ptNewTile, sizeof(POINT));
        MapWin_tile_to_pixel(This, ptNewTile.x, ptNewTile.y, &ptOldCenter.x, &ptOldCenter.y);
        MapWin_calculate_dimensions(This);

        if (CState.Scrolling) {
            This->iMapPixelLeft = (int)CState.ScrollOffsetX;
            This->iMapPixelTop = (int)CState.ScrollOffsetY;
        } else if (iOldZoom != -9999) {
            ptScale.x = This->iPixelsPerTileX;
            ptScale.y = This->iPixelsPerTileY;
            MapWin_tile_to_pixel(This, ptNewTile.x, ptNewTile.y, &ptNewCenter.x, &ptNewCenter.y);
            dx = ptOldCenter.x - ptNewCenter.x;
            dy = ptOldCenter.y - ptNewCenter.y;
            if (!This->iMapPixelLeft && fx && dx > -ptScale.x * 2 && dx < ptScale.x * 2) {
                This->iMapPixelLeft = dx;
            }
            if (!This->iMapPixelTop && fy && dy > -ptScale.y * 2 && dy < ptScale.y * 2
            && (dy + *ScreenHeight) / ptScale.y < (*MapAreaY - This->iMapTileTop) / 2) {
                This->iMapPixelTop = dy;
            }
        }
    } else {
        MapWin_calculate_dimensions(This);
    }
    return 0;
}

int __cdecl mod_blink_timer() {
    if (!*GameHalted && !VehBattleState[1]) {
        if (Win_is_visible(BaseWin)) {
            return TutWin_draw_arrow(TutWin);
        }
        if (Win_is_visible(SocialWin)) {
            return TutWin_draw_arrow(TutWin);
        }
        PlanWin_blink(PlanWin);
        StringBox_clip_ids(StringBox, 150);

        if ((!MapWin->field_23BE8 && (!*MultiplayerActive || !(*GameState & STATE_UNK_2))) || *ControlTurnA) {
            MapWin->field_23BFC = 0;
            return 0;
        }
        if (MapWin->field_23BE4 || (*MultiplayerActive && *GameState & STATE_UNK_2)) {
            MapWin->field_23BF8 = !MapWin->field_23BF8;
            draw_cursor();
            bool mouse_btn_down = GetAsyncKeyState(VK_LBUTTON) < 0;

            if ((int)*phInstance == GetWindowLongA(GetFocus(), GWL_HINSTANCE) && !Win_is_visible(TutWin)) {
                if (MapWin->field_23D80 != -1
                && MapWin->field_23D84 != -1
                && MapWin->field_23D88 != -1
                && MapWin->field_23D8C != -1) {
                    if (*GamePreferences & PREF_BSC_MOUSE_EDGE_SCROLL_VIEW || mouse_btn_down || *WinModalState != 0) {
                        CState.ScreenSize.x = *ScreenWidth;
                        CState.ScreenSize.y = *ScreenHeight;
                        check_scroll();
                    }
                }
            }
        }
    }
    return 0;
}

LRESULT WINAPI ModWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const bool debug_cmd = DEBUG && !*GameHalted && msg == WM_CHAR;
    static int delta_accum = 0;
    POINT p;
    MAP* sq;

    if (msg == WM_ACTIVATEAPP && conf.auto_minimise && !conf.reduced_mode) {
        if (!LOWORD(wParam)) {
            //If window has just become inactive e.g. ALT+TAB
            //wParam is 0 if the window has become inactive.
            set_minimised(true);
        } else {
            set_minimised(false);
        }
        return WinProc(hwnd, msg, wParam, lParam);

    } else if (msg == WM_MOVIEOVER && !conf.reduced_mode) {
        conf.playing_movie = false;
        set_video_mode(1);

    } else if (msg == WM_KEYDOWN && LOWORD(wParam) == VK_RETURN
    && GetAsyncKeyState(VK_MENU) < 0 && !conf.reduced_mode) {
        if (conf.video_mode == VM_Custom) {
            set_windowed(true);
        } else if (conf.video_mode == VM_Window) {
            set_windowed(false);
        }

    } else if (msg == WM_WINDOWED && !conf.reduced_mode) {
        RECT window_rect;
        WINDOWPLACEMENT wp;
        memset(&wp, 0, sizeof(wp));
        wp.length = sizeof(wp);
        GetWindowPlacement(*phWnd, &wp);
        wp.flags = 0;
        wp.showCmd = SW_SHOWNORMAL;
        SetRect(&window_rect, 0, 0, conf.window_width, conf.window_height);
        wp.rcNormalPosition = window_rect;
        SetWindowPlacement(*phWnd, &wp);
        SetWindowPos(*phWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);

    } else if (msg == WM_MOUSEWHEEL && win_has_focus()) {
        int wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam) + delta_accum;
        delta_accum = wheel_delta % WHEEL_DELTA;
        wheel_delta /= WHEEL_DELTA;
        bool zoom_in = (wheel_delta >= 0);
        wheel_delta = abs(wheel_delta);
        GameWinState state = current_window();

        if (state == GW_World) {
            int zoom_type = (zoom_in ? 515 : 516);
            for (int i = 0; i < wheel_delta; i++) {
                if (MapWin->iZoomFactor > -8 || zoom_in) {
                    Console_zoom(MapWin, zoom_type, 0);
                }
            }
        } else if (state == GW_Base && conf.render_high_detail) {
            base_resource_zoom(zoom_in);
        } else {
            int key;
            if (state == GW_Design) {
                key = (zoom_in ? VK_LEFT : VK_RIGHT);
            } else {
                key = (zoom_in ? VK_UP : VK_DOWN);
            }
            wheel_delta *= CState.ListScrollDelta;
            for (int i = 0; i < wheel_delta; i++) {
                PostMessage(hwnd, WM_KEYDOWN, key, 0);
                PostMessage(hwnd, WM_KEYUP, key, 0);
            }
            return 0;
        }

    } else if (msg == WM_KEYDOWN && (wParam == VK_UP || wParam == VK_DOWN)
    && conf.render_high_detail && ctrl_key_down() && current_window() == GW_Base) {
        base_resource_zoom(wParam == VK_UP);

    } else if (msg == WM_KEYDOWN && (wParam == VK_LEFT || wParam == VK_RIGHT)
    && ctrl_key_down() && current_window() == GW_Base) {
        int32_t value = ((BaseWindow*)BaseWin)->oRender.iResWindowTab;
        if (wParam == VK_LEFT) {
            value = (value + 1) % 3;
        } else {
            value = (value + 2) % 3;
        }
        ((BaseWindow*)BaseWin)->oRender.iResWindowTab = value;
        GraphicWin_redraw(BaseWin);

    } else if (conf.smooth_scrolling && msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) {
        if (current_window() != GW_World || !win_has_focus()) {
            CState.RightButtonDown = false;
            CState.ScrollDragging = false;
            return WinProc(hwnd, msg, wParam, lParam);

        } else if (msg == WM_RBUTTONDOWN) {
            CState.RightButtonDown = true;
            GetCursorPos(&p);
            memcpy(&CState.ScrollDragPos, &p, sizeof(POINT));

        } else if (msg == WM_RBUTTONUP) {
            CState.RightButtonDown = false;
            if (CState.ScrollDragging) {
                CState.ScrollDragging = false;
                SetCursor(LoadCursor(0, IDC_ARROW));
            } else {
                WinProc(hwnd, WM_RBUTTONDOWN, wParam | MK_RBUTTON, lParam);
                return WinProc(hwnd, WM_RBUTTONUP, wParam, lParam);
            }
        } else if (CState.RightButtonDown) {
            check_scroll();
        } else {
            return WinProc(hwnd, msg, wParam, lParam);
        }

    } else if (conf.smooth_scrolling && msg == WM_CHAR && wParam == 'r' && alt_key_down()) {
        CState.MouseOverTileInfo = !CState.MouseOverTileInfo;

    } else if (!conf.reduced_mode && msg == WM_CHAR && wParam == 't' && alt_key_down()) {
        show_mod_menu();

    } else if (conf.reduced_mode && msg == WM_CHAR && wParam == 'h' && alt_key_down()) {
        show_mod_menu();

    } else if (msg == WM_CHAR && wParam == 'o' && alt_key_down() && !*GameHalted
    && *GameState & STATE_SCENARIO_EDITOR && *GameState & STATE_OMNISCIENT_VIEW) {
        uint32_t seed = ThinkerVars->map_random_value;
        int value = pop_ask_number("modmenu", "MAPGEN", seed, 0);
        if (!value) { // OK button pressed
            console_world_generate(ParseNumTable[0]);
        }

    } else if (DEBUG && msg == WM_CHAR && wParam == 'd' && alt_key_down()) {
        conf.debug_mode = !conf.debug_mode;
        if (conf.debug_mode) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                Faction& f = Factions[i];
                if (!f.base_count) {
                    memset(f.goals, 0, sizeof(f.goals));
                    memset(f.sites, 0, sizeof(f.sites));
                }
            }
            *GameState |= STATE_DEBUG_MODE;
            *GamePreferences |= PREF_ADV_FAST_BATTLE_RESOLUTION;
        } else {
            *GameState &= ~STATE_DEBUG_MODE;
        }
        if (!*GameHalted) {
            MapWin_draw_map(MapWin, 0);
            InvalidateRect(hwnd, NULL, false);
            parse_says(0, MOD_VERSION, -1, -1);
            parse_says(1, (conf.debug_mode ?
                "Debug mode enabled." : "Debug mode disabled."), -1, -1);
            popp("modmenu", "GENERIC", 0, 0, 0);
        }

    } else if (DEBUG && msg == WM_CHAR && wParam == 'm' && alt_key_down()) {
        conf.debug_verbose = !conf.debug_verbose;
        parse_says(0, MOD_VERSION, -1, -1);
        parse_says(1, (conf.debug_verbose ?
            "Verbose mode enabled." : "Verbose mode disabled."), -1, -1);
        popp("modmenu", "GENERIC", 0, 0, 0);

    } else if (debug_cmd && wParam == 'b' && alt_key_down() && Win_is_visible(BaseWin)) {
        conf.base_psych = !conf.base_psych;
        base_compute(1);
        BaseWin_on_redraw(BaseWin);

    } else if (debug_cmd && wParam == 'c' && alt_key_down()
    && *GameState & STATE_SCENARIO_EDITOR && *GameState & STATE_OMNISCIENT_VIEW
    && (sq = mapsq(MapWin->iTileX, MapWin->iTileY)) && sq->landmarks) {
        uint32_t prev_state = MapWin->iWhatToDrawFlags;
        MapWin->iWhatToDrawFlags |= MAPWIN_DRAW_GOALS;
        refresh_overlay(code_at);
        int value = pop_ask_number("modmenu", "MAPGEN", sq->art_ref_id, 0);
        if (!value) { // OK button pressed
            sq->art_ref_id = ParseNumTable[0];
        }
        refresh_overlay(clear_overlay);
        MapWin->iWhatToDrawFlags = prev_state;
        draw_map(1);

    } else if (debug_cmd && wParam == 'y' && alt_key_down()) {
        static int draw_diplo = 0;
        draw_diplo = !draw_diplo;
        if (draw_diplo) {
            MapWin->iWhatToDrawFlags |= MAPWIN_DRAW_DIPLO_STATE;
            *GameState |= STATE_DEBUG_MODE;
        } else {
            MapWin->iWhatToDrawFlags &= ~MAPWIN_DRAW_DIPLO_STATE;
        }
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (debug_cmd && wParam == 'v' && alt_key_down()) {
        MapWin->iWhatToDrawFlags |= MAPWIN_DRAW_GOALS;
        refresh_overlay(clear_overlay);
        static int ts_type = 0;
        int i = 0;
        TileSearch ts;
        ts_type = (ts_type+1) % (MaxTileSearchType+1);
        ts.init(MapWin->iTileX, MapWin->iTileY, ts_type, 0);
        while (ts.get_next() != NULL) {
            mapdata[{ts.rx, ts.ry}].overlay = ++i;
        }
        mapdata[{MapWin->iTileX, MapWin->iTileY}].overlay = -ts_type;
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (debug_cmd && wParam == 'f' && alt_key_down()
    && (sq = mapsq(MapWin->iTileX, MapWin->iTileY))) {
        MapWin->iWhatToDrawFlags |= MAPWIN_DRAW_GOALS;
        if (sq && sq->is_owned()) {
            move_upkeep(sq->owner, UM_Visual);
            MapWin_draw_map(MapWin, 0);
            InvalidateRect(hwnd, NULL, false);
        }

    } else if (debug_cmd && wParam == 'x' && alt_key_down()) {
        MapWin->iWhatToDrawFlags |= MAPWIN_DRAW_GOALS;
        static int px = 0, py = 0;
        int x = MapWin->iTileX, y = MapWin->iTileY;
        int unit = is_ocean(mapsq(x, y)) ? BSC_UNITY_FOIL : BSC_UNITY_ROVER;
        path_distance(px, py, x, y, unit, 1);
        px=x;
        py=y;
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (debug_cmd && wParam == 'z' && alt_key_down()) {
        int x = MapWin->iTileX, y = MapWin->iTileY;
        int base_id;
        if ((base_id = base_at(x, y)) >= 0) {
            print_base(base_id);
        }
        print_map(x, y);
        for (int k=0; k < *VehCount; k++) {
            VEH* veh = &Vehicles[k];
            if (veh->x == x && veh->y == y) {
                Vehicles[k].state |= VSTATE_UNK_40000;
                Vehicles[k].state &= ~VSTATE_UNK_2000;
                print_veh(k);
            }
        }
        flushlog();

    } else {
        return WinProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int __cdecl mod_Win_init_class(const char* lpWindowName)
{
    int value = Win_init_class(lpWindowName);
    set_video_mode(0);
    return value;
}

void __cdecl mod_amovie_project(const char* name)
{
    conf.playing_movie = true;
    amovie_project(name);
    if (*phWnd) {
        PostMessage(*phWnd, WM_MOVIEOVER, 0, 0);
    }
}

void restore_video_mode()
{
    ChangeDisplaySettings(NULL, 0);
}

void set_video_mode(bool reset_window)
{
    if (conf.video_mode != VM_Window
    && (conf.window_width != conf.screen_width || conf.window_height != conf.screen_height)) {
        DEVMODE dv = {};
        dv.dmPelsWidth = conf.window_width;
        dv.dmPelsHeight = conf.window_height;
        dv.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        dv.dmSize = sizeof(dv);
        ChangeDisplaySettings(&dv, CDS_FULLSCREEN);
    }
    else if (conf.video_mode == VM_Window && reset_window && *phWnd) {
        // Restore window layout after movie playback has ended
        restore_video_mode();
        SetWindowLong(*phWnd, GWL_STYLE, AC_WS_WINDOWED);
        PostMessage(*phWnd, WM_WINDOWED, 0, 0);
    }
}

void set_minimised(bool minimise)
{
    if (conf.minimised != minimise && *phWnd) {
        conf.minimised = minimise;
        if (minimise) {
            restore_video_mode();
            ShowWindow(*phWnd, SW_MINIMIZE);
        } else {
            set_video_mode(0);
            ShowWindow(*phWnd, SW_RESTORE);
        }
    }
}

void set_windowed(bool windowed)
{
    if (!conf.playing_movie && *phWnd) {
        if (conf.video_mode == VM_Custom && windowed) {
            conf.video_mode = VM_Window;
            restore_video_mode();
            SetWindowLong(*phWnd, GWL_STYLE, AC_WS_WINDOWED);
            PostMessage(*phWnd, WM_WINDOWED, 0, 0);
        }
        else if (conf.video_mode == VM_Window && !windowed) {
            conf.video_mode = VM_Custom;
            set_video_mode(0);
            SetWindowLong(*phWnd, GWL_STYLE, AC_WS_FULLSCREEN);
            SetWindowPos(*phWnd, HWND_TOPMOST, 0, 0, conf.window_height, conf.window_width,
                         SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
            ShowWindow(*phWnd, SW_RESTORE);
        }
    }
}

/*
Render custom debug overlays with original and additional goals.
*/
void __thiscall MapWin_gen_overlays(Console* This, int x, int y)
{
    Buffer* Canvas = (Buffer*)&This->oMainWin.oCanvas.poOwner;
    RECT rt;
    if (*GameState & STATE_OMNISCIENT_VIEW && This->iWhatToDrawFlags & MAPWIN_DRAW_GOALS)
    {
        MapWin_tile_to_pixel(This, x, y, &rt.left, &rt.top);
        rt.right = rt.left + This->iPixelsPerTileX;
        rt.bottom = rt.top + This->iPixelsPerHalfTileX;

        char buf[20] = {};
        bool found = false;
        int color = 255;
        int value = mapdata[{x, y}].overlay;

        for (int faction = 1; faction < MaxPlayerNum && !found; faction++) {
            Faction& f = Factions[faction];
            MFaction& m = MFactions[faction];
            if (!f.base_count) {
                continue;
            }
            for (int i = 0; i < MaxGoalsNum && !found; i++) {
                Goal& goal = Factions[faction].goals[i];
                if (goal.x == x && goal.y == y && goal.priority > 0
                && goal.type != AI_GOAL_UNUSED ) {
                    found = true;
                    buf[0] = m.filename[0];
                    switch (goal.type) {
                        case AI_GOAL_ATTACK:
                            buf[1] = 'a';
                            color = ColorRed;
                            break;
                        case AI_GOAL_DEFEND:
                            buf[1] = 'd';
                            color = ColorYellow;
                            break;
                        case AI_GOAL_SCOUT:
                            buf[1] = 's';
                            color = ColorMagenta;
                            break;
                        case AI_GOAL_UNK_1:
                            buf[1] = 'n';
                            color = ColorBlue;
                            break;
                        case AI_GOAL_COLONIZE:
                            buf[1] = 'c';
                            color = ColorCyan;
                            break;
                        case AI_GOAL_TERRAFORM_LAND:
                            buf[1] = 'f';
                            color = ColorGreen;
                            break;
                        case AI_GOAL_UNK_4:
                            buf[1] = '^';
                            break;
                        case AI_GOAL_RAISE_LAND:
                            buf[1] = 'r';
                            break;
                        default:
                            buf[1] = (goal.type < Thinker_Goal_ID_First ? '*' : 'g');
                            break;
                    }
                    _itoa(goal.priority, &buf[2], 10);
                }
            }
        }
        if (!found && value != 0) {
            color = (value >= 0 ? ColorWhite : ColorYellow);
            _itoa(value, buf, 10);
        }
        if (found || value) {
            Buffer_set_text_color(Canvas, color, 0, 1, 1);
            Buffer_set_font(Canvas, &This->oFont2, 0, 0, 0);
            Buffer_write_cent_l3(Canvas, buf, &rt, 20);
        }
    }
}

void __cdecl mod_turn_timer()
{
    /*
    Timer calls this function every 500ms.
    Used for multiplayer related screen updates in turn_timer().
    */
    static uint32_t iter = 0;
    static uint32_t prev_time = 0;
    turn_timer();
    if (++iter & 1) {
        return;
    }
    if (*GameHalted && !*MapRandomSeed) {
        ThinkerVars->game_time_spent = 0;
        prev_time = 0;
    } else {
        uint32_t now = GetTickCount();
        if (prev_time && now - prev_time > 0) {
            ThinkerVars->game_time_spent += now - prev_time;
        }
        prev_time = now;
    }
}

void popup_homepage()
{
    ShellExecute(NULL, "open", "https://github.com/induktio/thinker", NULL, NULL, SW_SHOWNORMAL);
}

void show_mod_stats()
{
    int total_pop = 0,
        total_minerals = 0,
        total_energy = 0,
        faction_pop = 0,
        faction_units = 0,
        faction_minerals = 0,
        faction_energy = 0;

    Faction* f = &Factions[MapWin->cOwner];
    for (int i = 0; i < *BaseCount; ++i) {
        BASE* b = &Bases[i];
        int mindiv = (has_project(FAC_SPACE_ELEVATOR, b->faction_id)
             && (b->item() == -FAC_ORBITAL_DEFENSE_POD
             || b->item() == -FAC_NESSUS_MINING_STATION
             || b->item() == -FAC_ORBITAL_POWER_TRANS
             || b->item() == -FAC_SKY_HYDRO_LAB) ? 2 : 1);
        if (b->faction_id == MapWin->cOwner) {
            faction_pop += b->pop_size;
            faction_minerals += b->mineral_intake_2 / mindiv;
            faction_energy += b->energy_intake_2;
        }
        total_pop += b->pop_size;
        total_minerals += b->mineral_intake_2 / mindiv;
        total_energy += b->energy_intake_2;
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehicles[i];
        if (v->faction_id == MapWin->cOwner) {
            faction_units++;
        }
    }
    ParseNumTable[0] = *BaseCount;
    ParseNumTable[1] = *VehCount;
    ParseNumTable[2] = total_pop;
    ParseNumTable[3] = total_minerals;
    ParseNumTable[4] = total_energy;
    ParseNumTable[5] = f->base_count;
    ParseNumTable[6] = faction_units;
    ParseNumTable[7] = f->pop_total;
    ParseNumTable[8] = faction_minerals;
    ParseNumTable[9] = faction_energy;
    popp("modmenu", "STATS", 0, "markbm_sm.pcx", 0);
}

int show_mod_config()
{
    enum {
        MapGen = 1,
        MapContinents = 2,
        MapLandmarks = 4,
        MapPolarCaps = 8,
        MapMirrorX = 16,
        MapMirrorY = 32,
        AutoBases = 64,
        AutoUnits = 128,
        FormerReplace = 256,
        MapBaseInfo = 512,
        TreatyPopup = 1024,
        AutoMinimise = 2048,
    };
    *DialogChoices = 0
        | (conf.new_world_builder ? MapGen : 0)
        | (conf.world_continents ? MapContinents : 0)
        | (conf.modified_landmarks ? MapLandmarks : 0)
        | (conf.world_polar_caps ? MapPolarCaps: 0)
        | (conf.world_mirror_x ? MapMirrorX : 0)
        | (conf.world_mirror_y ? MapMirrorY : 0)
        | (conf.manage_player_bases ? AutoBases : 0)
        | (conf.manage_player_units ? AutoUnits : 0)
        | (conf.warn_on_former_replace ? FormerReplace : 0)
        | (conf.render_base_info ? MapBaseInfo : 0)
        | (conf.foreign_treaty_popup ? TreatyPopup : 0)
        | (conf.auto_minimise ? AutoMinimise : 0);

    // Return value is equal to choices bitfield if OK pressed, -1 otherwise.
    int value = X_pop("modmenu", "OPTIONS", -1, 0, PopDialogCheckbox|PopDialogBtnCancel, 0);
    if (value < 0) {
        return 0;
    }
    conf.new_world_builder = !!(value & MapGen);
    WritePrivateProfileStringA(ModAppName, "new_world_builder",
        (conf.new_world_builder ? "1" : "0"), ModIniFile);

    conf.modified_landmarks = !!(value & MapLandmarks);
    WritePrivateProfileStringA(ModAppName, "modified_landmarks",
        (conf.modified_landmarks ? "1" : "0"), ModIniFile);

    conf.world_continents = !!(value & MapContinents);
    WritePrivateProfileStringA(ModAppName, "world_continents",
        (conf.world_continents ? "1" : "0"), ModIniFile);

    conf.world_polar_caps = !!(value & MapPolarCaps);
    WritePrivateProfileStringA(ModAppName, "world_polar_caps",
        (conf.world_polar_caps ? "1" : "0"), ModIniFile);

    conf.world_mirror_x = !!(value & MapMirrorX);
    WritePrivateProfileStringA(ModAppName, "world_mirror_x",
        (conf.world_mirror_x ? "1" : "0"), ModIniFile);

    conf.world_mirror_y = !!(value & MapMirrorY);
    WritePrivateProfileStringA(ModAppName, "world_mirror_y",
        (conf.world_mirror_y ? "1" : "0"), ModIniFile);

    conf.manage_player_bases = !!(value & AutoBases);
    WritePrivateProfileStringA(ModAppName, "manage_player_bases",
        (conf.manage_player_bases ? "1" : "0"), ModIniFile);

    conf.manage_player_units = !!(value & AutoUnits);
    WritePrivateProfileStringA(ModAppName, "manage_player_units",
        (conf.manage_player_units ? "1" : "0"), ModIniFile);

    conf.warn_on_former_replace = !!(value & FormerReplace);
    WritePrivateProfileStringA(ModAppName, "warn_on_former_replace",
        (conf.warn_on_former_replace ? "1" : "0"), ModIniFile);

    conf.render_base_info = !!(value & MapBaseInfo);
    WritePrivateProfileStringA(ModAppName, "render_base_info",
        (conf.render_base_info ? "1" : "0"), ModIniFile);

    conf.foreign_treaty_popup = !!(value & TreatyPopup);
    WritePrivateProfileStringA(ModAppName, "foreign_treaty_popup",
        (conf.foreign_treaty_popup ? "1" : "0"), ModIniFile);

    conf.auto_minimise = !!(value & AutoMinimise);
    WritePrivateProfileStringA(ModAppName, "auto_minimise",
        (conf.auto_minimise ? "1" : "0"), ModIniFile);

    draw_map(1);
    if (Win_is_visible(BaseWin)) {
        BaseWin_on_redraw(BaseWin);
    }
    return 0;
}

int show_mod_menu()
{
    parse_says(0, MOD_VERSION, -1, -1);
    parse_says(1, MOD_DATE, -1, -1);

    if (*GameHalted) {
        int ret = popp("modmenu", "MAINMENU", 0, "stars_sm.pcx", 0);
        if (ret == 1) {
            show_mod_config();
        }
        else if (ret == 2) {
            show_rules_menu();
        }
        else if (ret == 3) {
            popup_homepage();
        }
        return 0; // Close dialog
    }
    uint64_t seconds = ThinkerVars->game_time_spent / 1000;
    ParseNumTable[0] = seconds / 3600;
    ParseNumTable[1] = (seconds / 60) % 60;
    ParseNumTable[2] = seconds % 60;
    int ret = popp("modmenu", "GAMEMENU", 0, "stars_sm.pcx", 0);

    if (ret == 1 && !*PbemActive && !*MultiplayerActive) {
        show_mod_stats();
    }
    else if (ret == 2) {
        show_mod_config();
    }
    else if (ret == 3) {
        popup_homepage();
    }
    return 0; // Close dialog
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

int __thiscall SetupWin_buffer_draw(Buffer* src, Buffer* dst, int a3, int a4, int a5, int a6, int a7)
{
    if (conf.window_width >= 1024) {
        const int moon_positions[][4] = {
            {8, 287, 132, 132},
            {221, 0, 80, 51},
            {348, 94, 55, 57},
        };
        for (auto& p : moon_positions) {
            int x = conf.window_width  * p[0] / 1024;
            int y = conf.window_height * p[1] / 768;
            int w = conf.window_width  * p[2] / 1024;
            int h = conf.window_height * p[3] / 768;
            Buffer_copy2(src, dst, p[0], p[1], p[2], p[3], x, y, w, h);
        }
        return 0;
    } else {
        return Buffer_draw(src, dst, a3, a4, a5, a6, a7);
    }
}

int __thiscall SetupWin_buffer_copy(Buffer* src, Buffer* dst,
int xSrc, int ySrc, int xDst, int yDst, int wSrc, int hSrc)
{
    if (conf.window_width >= 1024) {
        int wDst = conf.window_width * wSrc / 1024;
        return Buffer_copy2(src, dst, xSrc, ySrc, wSrc, hSrc,
            conf.window_width - wDst, yDst, wDst, conf.window_height);
    } else {
        return Buffer_copy(src, dst, xSrc, ySrc, xDst, yDst, wSrc, hSrc);
    }
}

int __thiscall SetupWin_soft_update3(Win* This, int a2, int a3, int a4, int a5)
{
    // Update whole screen instead of partial regions
    return GraphicWin_soft_update2(This);
}

int __thiscall window_scale_load_pcx(Buffer* This, char* filename, int a3, int a4, int a5)
{
    int value;
    if (conf.window_width >= 1024) {
        Buffer image;
        Buffer_Buffer(&image);
        value = Buffer_load_pcx(&image, filename, a3, a4, a5);
        Buffer_resize(This, conf.window_width, conf.window_height);
        Buffer_copy2(&image, This, 0, 0, image.stRect->right, image.stRect->bottom,
            0, 0, conf.window_width, conf.window_height);
        Buffer_dtor(&image);
    } else {
        value = Buffer_load_pcx(This, filename, a3, a4, a5);
    }
    if (*GameHalted) {
        char buf[StrBufLen];
        snprintf(buf, StrBufLen, "%s%s%s%s",
            MOD_VERSION, " / ", MOD_DATE, (conf.smac_only ? " / SMAC" : ""));
        Buffer_set_text_color(This, ColorProdName, 0, 1, 1);
        Buffer_set_font(This, &MapWin->oFont1, 0, 0, 0);
        Buffer_write_l(This, buf, 20, conf.window_height-32, 100);
    }
    return value;
}

int __thiscall Credits_GraphicWin_init(
Win* This, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10)
{
    if (conf.window_width >= 1024) {
        return GraphicWin_init(This,
            a2 * conf.window_width / 1024,
            a3 * conf.window_height / 768,
            a4 * conf.window_width / 1024,
            a5 * conf.window_height / 768,
            a6, a7, a8, a9, a10);
    } else {
        return GraphicWin_init(This, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }
}

int __thiscall BaseWin_popup_start(
Win* This, const char* filename, const char* label, int a4, int a5, int a6, int a7)
{
    BASE* base = *CurrentBase;
    Faction* f = &Factions[base->faction_id];
    int item_cost = mineral_cost(*CurrentBaseID, base->queue_items[0]);
    int minerals = item_cost - base->minerals_accumulated - max(0, base->mineral_surplus);
    int credits = max(0, f->energy_credits - f->energy_cost);
    int cost = hurry_cost(*CurrentBaseID, base->queue_items[0], minerals);
    minimal_cost = min(credits, cost);
    if (item_cost <= base->minerals_accumulated) {
        ParseNumTable[0] = 0;
    }
    ParseNumTable[1] = cost;
    ParseNumTable[2] = credits;
    return Popup_start(This, "modmenu", "HURRY", a4, a5, a6, a7);
}

#pragma GCC diagnostic pop

void __thiscall BaseWin_draw_misc_eco_damage(Buffer* This, char* buf, int x, int y, int len)
{
    BASE* base = *CurrentBase;
    Faction* f = &Factions[base->faction_id];
    if (!conf.render_base_info || !strlen(label_eco_damage)) {
        Buffer_write_l(This, buf, x, y, len);
    } else {
        int clean_mins = conf.clean_minerals + f->clean_minerals_modifier
            + clamp(f->satellites_mineral, 0, (int)base->pop_size);
        int damage = terraform_eco_damage(*CurrentBaseID);
        int mins = base->mineral_intake_2 + damage/8;
        int pct;
        if (base->eco_damage > 0) {
            pct = 100 + base->eco_damage;
        } else {
            pct = (clean_mins > 0 ? 100 * clamp(mins, 0, clean_mins) / clean_mins : 0);
        }
        snprintf(buf, StrBufLen, label_eco_damage, pct);
        Buffer_write_l(This, buf, x, y, strlen(buf));
    }
}

void __thiscall BaseWin_draw_support(BaseWindow* This)
{
    RECT& rc = This->oRender.rResWindow;
    Buffer_set_clip(&This->oCanvas, &rc);
    GraphicWin_fill2((Win*)This, &rc, 0);

    MapWin_init((Console*)&This->oRender, 2, 0);
    This->oRender.iZoomFactor = base_zoom_factor;
    This->oRender.iWhatToDrawFlags = MAPWIN_SUPPORT_VIEW|MAPWIN_DRAW_BONUS_RES|\
        MAPWIN_DRAW_RIVERS|MAPWIN_DRAW_IMPROVEMENTS|MAPWIN_DRAW_TRANSLUCENT;

    This->oRender.iTileX = (*CurrentBase)->x;
    This->oRender.iTileY = (*CurrentBase)->y;
    GraphicWin_redraw((Win*)&This->oRender.oBufWin);

    Buffer_copy(&This->oRender.oBufWin.oCanvas, &This->oCanvas,
        0, 0, rc.left + 11, rc.top + 31, rc.right, rc.bottom);
    GraphicWin_soft_update((Win*)This, &rc);
    Buffer_set_clip(&This->oCanvas, &This->oCanvas.stRect[0]);
}

int __cdecl BaseWin_ask_number(const char* label, int value, int a3)
{
    ParseNumTable[0] = value;
    return pop_ask_number(ScriptFile, label, minimal_cost, a3);
}

void __thiscall BaseWin_draw_farm_set_font(Buffer* This, Font* font, int a3, int a4, int a5)
{
    char buf[StrBufLen] = {};
    // Base resource window coordinates including button row
    RECT* rc = &((BaseWindow*)BaseWin)->oRender.rResWindow;
    int x1 = rc->left;
    int y1 = rc->top;
    int x2 = rc->right;
    int y2 = rc->bottom;
    int N = 0;
    int M = 0;
    int E = 0;
    int SE = 0;
    Buffer_set_font(This, font, a3, a4, a5);

    if (*CurrentBaseID < 0 || x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0) {
        assert(0);
    } else if (conf.render_base_info) {
        if (satellite_bonus(*CurrentBaseID, &N, &M, &E)) {
            snprintf(buf, StrBufLen, label_sat_nutrient, N);
            Buffer_set_text_color(This, ColorNutrient, 0, 1, 1);
            Buffer_write_l(This, buf, x1 + 5, y2 - 36 - 28, LineBufLen);

            snprintf(buf, StrBufLen, label_sat_mineral, M);
            Buffer_set_text_color(This, ColorMineral, 0, 1, 1);
            Buffer_write_l(This, buf, x1 + 5, y2 - 36 - 14, LineBufLen);

            snprintf(buf, StrBufLen, label_sat_energy, E);
            Buffer_set_text_color(This, ColorEnergy, 0, 1, 1);
            Buffer_write_l(This, buf, x1 + 5, y2 - 36     , LineBufLen);
        }
        if ((SE = stockpile_energy_active(*CurrentBaseID)) > 0) {
            snprintf(buf, StrBufLen, label_stockpile_energy, SE);
            Buffer_set_text_color(This, ColorEnergy, 0, 1, 1);
            Buffer_write_right_l2(This, buf, x2 - 5, y2 - 36, LineBufLen);
        }
    }
}

void __cdecl BaseWin_draw_psych_strcat(char* buffer, char* source)
{
    BASE* base = &Bases[*CurrentBaseID];
    if (conf.render_base_info && *CurrentBaseID >= 0) {
        if (base->nerve_staple_turns_left > 0
        || has_fac_built(FAC_PUNISHMENT_SPHERE, *CurrentBaseID)) {
            if (!strcmp(source, (*TextLabels)[971])) { // Stapled Base
                strncat(buffer, (*TextLabels)[322], StrBufLen); // Unmodified
                return;
            }
            if (!strcmp(source, (*TextLabels)[327])) { // Secret Projects
                strncat(buffer, (*TextLabels)[971], StrBufLen); // Stapled Base
                return;
            }
        }
        int turns = base->assimilation_turns_left;
        if (turns > 0 && !strcmp(source, (*TextLabels)[970])) { // Captured Base
            snprintf(buffer, StrBufLen, label_captured_base, turns);
            return;
        }
    }
    strncat(buffer, source, StrBufLen);
}

void __thiscall BaseWin_draw_energy_set_text_color(Buffer* This, int a2, int a3, int a4, int a5)
{
    BASE* base = &Bases[*CurrentBaseID];
    char buf[StrBufLen] = {};
    if (conf.render_base_info && *CurrentBaseID >= 0) {
        int workers = base->pop_size - base->talent_total - base->drone_total - base->specialist_total;
        int color;

        if (base_maybe_riot(*CurrentBaseID)) {
            color = ColorRed;
        } else if (base->golden_age()) {
            color = ColorEnergy;
        } else {
            color = ColorIntakeSurplus;
        }
        Buffer_set_text_color(This, color, a3, a4, a5);
        snprintf(buf, StrBufLen, label_pop_size,
            base->talent_total, workers, base->drone_total, base->specialist_total);
        if (DEBUG) {
            strncat(buf, conf.base_psych ? " / B" : " / A", 32);
        }
        Buffer_write_right_l2(This, buf, 690, 423 - 42, LineBufLen);

        if (base_pop_boom(*CurrentBaseID) && base_unused_space(*CurrentBaseID) > 0) {
            Buffer_set_text_color(This, ColorNutrient, a3, a4, a5);
            snprintf(buf, StrBufLen, "%s", label_pop_boom);
            Buffer_write_right_l2(This, buf, 690, 423 - 21, LineBufLen);
        }

        if (base->nerve_staple_turns_left > 0) {
            snprintf(buf, StrBufLen, label_nerve_staple, base->nerve_staple_turns_left);
            Buffer_set_text_color(This, ColorEnergy, a3, a4, a5);
            Buffer_write_right_l2(This, buf, 690, 423, LineBufLen);
        }
    }
    Buffer_set_text_color(This, a2, a3, a4, a5);
}

void __cdecl mod_base_draw(Buffer* buffer, int base_id, int x, int y, int zoom, int opts)
{
    int color = -1;
    int width = 1;
    BASE* base = &Bases[base_id];
    base_draw(buffer, base_id, x, y, zoom, opts);

    if (conf.render_base_info && zoom >= -8) {
        if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
            color = ColorWhite;
            width = 2;
        }
        if (has_fac_built(FAC_GEOSYNC_SURVEY_POD, base_id)
        || has_fac_built(FAC_FLECHETTE_DEFENSE_SYS, base_id)) {
            color = ColorCyan;
        }
        if (base->faction_id == MapWin->cOwner && base->golden_age()) {
            color = ColorEnergy;
        }
        if (base->faction_id == MapWin->cOwner && base_maybe_riot(base_id)) {
            color = ColorRed;
        }
        if (color < 0) {
            return;
        }
        // Game engine uses this way to determine the population label width
        int w = Font_width(*MapLabelFont, (base->pop_size >= 10 ? "88" : "8")) + 5;
        int h = (*MapLabelFont)->iHeight + 4;

        for (int i = 1; i <= width; i++) {
            RECT rr = {x-i, y-i, x+w+i, y+h+i};
            Buffer_box(buffer, &rr, color, color);
        }
    }
}

int __cdecl BaseWin_staple_popp(
const char* filename, const char* label, int a3, const char* imagefile, int a5)
{
    BASE* base = *CurrentBase;
    if (base && base->assimilation_turns_left
    && base->faction_id_former != MapWin->cOwner && is_alive(base->faction_id_former)) {
        return popp("modmenu", "NERVESTAPLE2", a3, imagefile, a5);
    }
    return popp(filename, label, a3, imagefile, a5);
}

/*
Refresh base window workers properly after nerve staple is done.
*/
void __cdecl BaseWin_action_staple(int base_id)
{
    if (can_staple(base_id)) {
        set_base(base_id);
        action_staple(base_id);
        base_compute(1);
        BaseWin_on_redraw(BaseWin);
    }
}

/*
Separate case where nerve stapling is done from another popup.
*/
void __cdecl popb_action_staple(int base_id)
{
    if (can_staple(base_id)) {
        action_staple(base_id);
    }
}

int __thiscall BaseWin_click_staple(Win* This)
{
    // SE_Police value is checked before calling this function
    int base_id = ((BaseWindow*)This)->oRender.base_id;
    if (base_id >= 0 && conf.nerve_staple > Bases[base_id].plr_owner()) {
        return BaseWin_nerve_staple(This);
    }
    return 0;
}

void __cdecl ReportWin_draw_ops_strcat(char* dst, char* UNUSED(src))
{
    BASE* base = *CurrentBase;
    uint32_t gov = base->governor_flags;
    char buf[StrBufLen] = {};
    dst[0] = '\0';

    if (base->faction_id == MapWin->cOwner) {
        if (gov & GOV_ACTIVE) {
            if (gov & GOV_PRIORITY_EXPLORE) {
                strncat(dst, (*TextLabels)[521], 32);
            } else if (gov & GOV_PRIORITY_DISCOVER) {
                strncat(dst, (*TextLabels)[522], 32);
            } else if (gov & GOV_PRIORITY_BUILD) {
                strncat(dst, (*TextLabels)[523], 32);
            } else if (gov & GOV_PRIORITY_CONQUER) {
                strncat(dst, (*TextLabels)[524], 32);
            } else {
                strncat(dst, (*TextLabels)[457], 32); // Governor
            }
            strncat(dst, " ", 32);
        }
    }
    if (strlen(label_base_surplus)) {
        snprintf(buf, StrBufLen, label_base_surplus,
            base->nutrient_surplus, base->mineral_surplus, base->energy_surplus);
        strncat(dst, buf, StrBufLen);
    }
}

void __thiscall ReportWin_draw_ops_color(Buffer* This, int UNUSED(a2), int a3, int a4, int a5)
{
    BASE* base = *CurrentBase;
    int color = (base->mineral_surplus < 0 || base->nutrient_surplus < 0
        ? ColorRed : ColorMediumBlue);
    Buffer_set_text_color(This, color, a3, a4, a5);
}

int __thiscall mod_MapWin_focus(Console* This, int x, int y)
{
    // Return value is non-zero when the map is recentered offscreen
    if (MapWin_focus(This, x, y)) {
        This->drawOnlyCursor = 0;
        draw_map(1);
    }
    return 0;
}

int __thiscall mod_MapWin_set_center(Console* This, int x, int y, int flag)
{
    // Make sure the whole screen is refreshed when clicking on map tiles
    if (!in_box(x, y, RenderTileBounds)) {
        This->drawOnlyCursor = 0;
    }
    return MapWin_set_center(This, x, y, flag);
}

/*
This is called when ReportWin is closing and is used to refresh base labels
on any bases where workers have been adjusted from the base list window.
*/
int __thiscall ReportWin_close_handler(void* This)
{
    SubInterface_release_iface_mode(This);
    return draw_map(1);
}

/*
Fix potential crash when a game is loaded after using Edit Map > Generate/Remove Fungus > No Fungus.
Vanilla version changed MapWin->cOwner variable for unknown reason.
*/
void __thiscall Console_editor_fungus(Console* UNUSED(This))
{
    auto_undo();
    int v1 = X_pop7("FUNGOSITY", PopDialogBtnCancel, 0);
    if (v1 >= 0) {
        int v2 = 0;
        if (!v1 || (v2 = X_pop7("FUNGMOTIZE", PopDialogBtnCancel, 0)) > 0) {
            MAP* sq = *MapTiles;
            for (int i = 0; i < *MapAreaTiles; ++i, ++sq) {
                sq->items &= ~BIT_FUNGUS;
                // Update visible tile items
                for (int j = 1; j < 8; ++j) {
                    *((uint32_t*)&sq->landmarks + j) = sq->items;
                }
            }
        }
        if (v2 < 0) {
            return;
        }
        if (v1 > 0) {
            int v3 = *MapNativeLifeForms;
            *MapNativeLifeForms = v1 - 1;
            world_fungus();
            *MapNativeLifeForms = v3;
        }
        draw_map(1);
    }
}

/*
Fix foreign base names being visible in unexplored tiles when issuing move to or patrol
orders to the tiles. This version adds visibility checks for all base tiles.
*/
void __cdecl mod_say_loc(char* dest, int x, int y, int a4, int a5, int a6)
{
    int base_id = -1;
    MAP* sq;

    if ((sq = mapsq(x, y)) && sq->is_base()
    && (*GameState & STATE_SCENARIO_EDITOR || sq->is_visible(MapWin->cOwner))) {
        base_id = base_at(x, y);
    }
    if (a4 != 0 && base_id < 0) {
        a6 = 0;
        base_id = base_find3(x, y, -1, -1, -1, MapWin->cOwner);
        if (base_id >= 0) {
            strncat(dest, (*TextLabels)[62], 32); // near
            strncat(dest, " ", 2);
        }
    }
    if (base_id >= 0) {
        if (a6) {
            strncat(dest, (*TextLabels)[8], 32); // at
            strncat(dest, " ", 2);
        }
        strncat(dest, Bases[base_id].name, MaxBaseNameLen);
        if (a5) {
            strncat(dest, " ", 2);
        }
    }
    if (a5 == 1 || (a5 == 2 && base_id < 0)) {
        snprintf(dest + strlen(dest), 32, "(%d, %d)", x, y);
    }
}

/*
Fix diplomacy dialog to show the missing response messages (GAVEENERGY) when gifting
energy credits to another faction. The error happens when the label is written to
StrBuffer in make_gift but diplomacy_caption overwrites it with other data.
*/
void __cdecl mod_diplomacy_caption(int faction1, int faction2)
{
    char buf[StrBufLen];
    strncpy(buf, StrBuffer, StrBufLen);
    buf[StrBufLen-1] = '\0';
    diplomacy_caption(faction1, faction2);
    strncpy(StrBuffer, buf, StrBufLen);
}

/*
Fix foreign_treaty_popup displaying same treaty changes multiple times per turn.
In any case, these events will be displayed as a non-persistent message in map window.
*/
static char netmsg_label[StrBufLen] = {};
static char netmsg_item0[StrBufLen] = {};
static char netmsg_item1[StrBufLen] = {};

void __cdecl reset_netmsg_status()
{
    netmsg_label[0] = '\0';
    netmsg_item0[0] = '\0';
    netmsg_item1[0] = '\0';
}

int __thiscall mod_NetMsg_pop(void* This, const char* label, int delay, int a4, const char* a5)
{
    if (!conf.foreign_treaty_popup) {
        return NetMsg_pop(This, label, delay, a4, a5);
    }
    if (!strcmp(label, "GOTMYPROBE")) {
        return NetMsg_pop(This, label, -1, a4, a5);
    }
    if (!strcmp(label, netmsg_label)
    && !strcmp((char*)&ParseStrBuffer[0], netmsg_item0)
    && !strcmp((char*)&ParseStrBuffer[1], netmsg_item1)) {
        // Skip additional popup windows
        return NetMsg_pop(This, label, delay, a4, a5);
    }
    strncpy(netmsg_label, label, StrBufLen);
    netmsg_label[StrBufLen-1] = '\0';
    strncpy(netmsg_item0, (char*)&ParseStrBuffer[0], StrBufLen);
    netmsg_item0[StrBufLen-1] = '\0';
    strncpy(netmsg_item1, (char*)&ParseStrBuffer[1], StrBufLen);
    netmsg_item1[StrBufLen-1] = '\0';
    return NetMsg_pop(This, label, -1, a4, a5);
}

int __thiscall mod_BasePop_start(
void* This, const char* filename, const char* label, int a4, int a5, int a6, int a7)
{
    if (movedlabels.count(label)) {
        return BasePop_start(This, "modmenu", label, a4, a5, a6, a7);
    }
    return BasePop_start(This, filename, label, a4, a5, a6, a7);
}

int __cdecl mod_action_move(int veh_id, int x, int y)
{
    VEH* veh = &Vehs[veh_id];
    if (*MultiplayerActive) {
        return action_move(veh_id, x, y);
    }
    if (veh->faction_id == *CurrentPlayerFaction) {
        if (!veh_ready(veh_id)) {
            return NetMsg_pop(NetMsg, "UNITMOVED", 5000, 0, 0);
        }
    }
    int veh_range = arty_range(veh->unit_id);
    if (map_range(veh->x, veh->y, x, y) <= veh_range) {
        int veh_id_tgt = stack_fix(veh_at(x, y));
        if (veh_id_tgt >= 0) {
            if (veh->faction_id != Vehs[veh_id_tgt].faction_id
            && !has_pact(veh->faction_id, Vehs[veh_id_tgt].faction_id)) {
                int offset = radius_move2(veh->x, veh->y, x, y, TableRange[veh_range]);
                if (offset >= 0) {
                    *VehAttackFlags = 3;
                    return battle_fight_1(veh_id, offset, 1, 1, 0);
                }
            }
        } else {
            return action_destroy(veh_id, 0, x, y);
        }
    } else {
        return NetMsg_pop(NetMsg, "OUTOFRANGE", 5000, 0, 0);
    }
    return 0;
}

int __cdecl MapWin_right_menu_arty(int veh_id, int x, int y)
{
    VEH* veh = &Vehs[veh_id];
    return map_range(veh->x, veh->y, x, y) <= arty_range(veh->unit_id);
}

void __thiscall Console_arty_cursor_on(Console* This, int cursor_type, int veh_id)
{
    int veh_range = arty_range(Vehs[veh_id].unit_id);
    Console_cursor_on(This, cursor_type, veh_range);
}




