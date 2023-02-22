
#include "gui.h"


// Size in pixels of the bottom middle UI console
const int ConsoleHeight = 219;
const int ConsoleWidth = 1024;

static bool FileboxVisible = false;

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
typedef int(__thiscall *CCANVAS_CREATE_F)(Buffer* pthis);
typedef int(__stdcall *WNDPROC_F)(HWND, int, WPARAM, LPARAM);
typedef int(__thiscall *CMAIN_ZOOMPROCESSING_F)(Console* pthis);
typedef int(__stdcall *PROC_ZOOM_KEY_F)(int iZoomType, int iZero);
typedef int(__thiscall *CMAIN_TILETOPT_F)(Console* pthis, int iTileX, int iTileY, long* piX, long* piY);
typedef int(__thiscall *CMAIN_MOVEMAP_F)(Console *pthis, int iXPos, int iYPos, int a4);
typedef int(__thiscall *CMAIN_REDRAWMAP_F)(Console *pthis, int a2);
typedef int(__thiscall *CMAIN_DRAWMAP_F)(Console* This, int iOwner, int fUnitsOnly);
typedef int(__thiscall *CMAIN_PTTOTILE_F)(Console* This, POINT p, long* piTileX, long* piTileY);
typedef int(__thiscall *CINFOWIN_DRAWTILEINFO_F)(CInfoWin* This);
typedef int(__cdecl *PAINTHANDLER_F)(RECT *prRect, int a2);
typedef int(__cdecl *PAINTMAIN_F)(RECT *pRect);
typedef int(__thiscall *CSPRITE_FROMCANVASRECTTRANS_F)(Sprite *This, Buffer *poCanvas,
    int iTransparentIndex, int iLeft, int iTop, int iWidth, int iHeight, int a8);
typedef int(__thiscall *CCANVAS_DESTROY4_F)(Buffer *This);
typedef int (__thiscall *CSPRITE_STRETCHCOPYTOCANVAS_F)
    (Sprite *This, Buffer *poCanvasDest, int cTransparentIndex, int iLeft, int iTop);
typedef int(__thiscall *CSPRITE_STRETCHCOPYTOCANVAS1_F)
    (Sprite *This, Buffer *poCanvasDest, int cTransparentIndex, int iLeft, int iTop, int iDestScale, int iSourceScale);
typedef int (__thiscall *CSPRITE_STRETCHDRAWTOCANVAS2_F)
    (Sprite *This, Buffer *poCanvas, int a1, int a2, int a3, int a4, int a7, int a8);
typedef int(__thiscall *CWINBASE_ISVISIBLE_F)(Win *This);
typedef int(__thiscall *CTIMER_STARTTIMER_F)(Time *This, int a2, int a3, int iDelay, int iElapse, int uResolution);
typedef int(__thiscall *CTIMER_STOPTIMER_F)(Time *This);
typedef int(__thiscall *DRAWCITYMAP_F)(Win *This, int a2);
typedef int(__cdecl *GETFOODCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithFarm);
typedef int(__cdecl *GETPRODCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithMine);
typedef int(__cdecl *GETENERGYCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithSolar);
typedef int(__thiscall *IMAGEFROMCANVAS_F)(CImage *This, Buffer *poCanvasSource, int iLeft, int iTop, int iWidth, int iHeight, int a7);
typedef int(__cdecl *GETELEVATION_F)(int iTileX, int iTileY);
typedef int(__thiscall *CIMAGE_COPYTOCANVAS2_F)(CImage *This, Buffer *poCanvasDest, int x, int y, int a5, int a6, int a7);
typedef int(__thiscall *CMAINMENU_ADDSUBMENU_F)(Menu* This, int iMenuID, int iMenuItemID, char *lpString);
typedef int(__thiscall *CMAINMENU_ADDBASEMENU_F)(Menu *This, int iMenuItemID, const char *pszCaption, int a4);
typedef int(__thiscall *CMAINMENU_ADDSEPARATOR_F)(Menu *This, int iMenuItemID, int iSubMenuItemID);
typedef int(__thiscall *CMAINMENU_UPDATEVISIBLE_F)(Menu *This, int a2);
typedef int(__thiscall *CMAINMENU_RENAMEMENUITEM_F)(Menu *This, int iMenuItemID, int iSubMenuItemID, const char *pszCaption);
typedef long(__thiscall *CMAP_GETCORNERYOFFSET_F)(MapWin_Alt *This, int iTileX, int iTileY, int iCorner);

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


void __thiscall FileBox_init(void* This) {
    FileboxVisible = true;
    int32_t* pa = (int32_t*)This;
    int8_t* pb = (int8_t*)This;
    int8_t* pc = pb + 780;
    *(pa + 260) = (int32_t)pc;
    *pa = 0;
    *pb = 0;
    pb[260] = 0;
    pb[520] = 0;
    pb[781] = 0;
    pb[1044] = 0;
    pb[1048] = 0;
}

void __thiscall FileBox_close(void* UNUSED(This)) {
    FileboxVisible = false;
}

/*
Returns true only when the world map is visible and has focus
and other large modal windows are not blocking it.
*/
bool map_is_visible() {
    return !*GameHalted
        && !FileboxVisible
        && Win_is_visible(WorldWin)
        && !Win_is_visible(BaseWin)
        && !Win_is_visible(PlanWin)
        && !Win_is_visible(FameWin)
        && !Win_is_visible(MonuWin)
        && !Win_is_visible(ReportWin)
        && !Win_is_visible(SocialWin)
        && !Win_is_visible(CouncilWin)
        && !Win_is_visible(DatalinkWin);
}

bool win_has_focus() {
    return GetFocus() == *phWnd;
}

bool alt_key_down() {
    return GetAsyncKeyState(VK_MENU) < 0;
}

void mouse_over_tile(POINT* p) {
    static POINT ptLastTile = {0, 0};
    POINT ptTile;

    if (CState.MouseOverTileInfo
    && !pMain->fUnitNotViewMode
    && p->x >= 0 && p->x < CState.ScreenSize.x
    && p->y >= 0 && p->y < (CState.ScreenSize.y - ConsoleHeight)
    && MapWin_pixel_to_tile(pMain, p->x, p->y, &ptTile.x, &ptTile.y) == 0
    && memcmp(&ptTile, &ptLastTile, sizeof(POINT)) != 0) {

        pInfoWin->iTileX = ptTile.x;
        pInfoWin->iTileY = ptTile.y;
        StatusWin_on_redraw(pInfoWin);
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
    int mx = *map_axis_x;
    int my = *map_axis_y;
    int i;
    int d;
    if (x && pMain->oMap.iMapTilesEvenX + pMain->oMap.iMapTilesOddX < mx) {
        if (x < 0 && (!(*map_toggle_flat) || pMain->oMap.iMapTileLeft > 0)) {
            i = (int)CState.ScrollOffsetX;
            CState.ScrollOffsetX -= x;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetX);
            while (CState.ScrollOffsetX >= pMain->oMap.iPixelsPerTileX) {
                CState.ScrollOffsetX -= pMain->oMap.iPixelsPerTileX;
                pMain->oMap.iTileX -= 2;
                if (pMain->oMap.iTileX < 0) {
                    if (*map_toggle_flat) {
                        pMain->oMap.iTileX = 0;
                        pMain->oMap.iTileY &= ~1;
                        CState.ScrollOffsetX = 0;
                    } else {
                        pMain->oMap.iTileX += mx;
                    }
                }
            }
        } else if (x < 0 && (*map_toggle_flat)) {
            fScrolled = true;
            CState.ScrollOffsetX = 0;
        }
        if (x > 0 &&
                (!(*map_toggle_flat) ||
                 pMain->oMap.iMapTileLeft +
                 pMain->oMap.iMapTilesEvenX +
                 pMain->oMap.iMapTilesOddX <= mx)) {
            i = (int)CState.ScrollOffsetX;
            CState.ScrollOffsetX -= x;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetX);
            while (CState.ScrollOffsetX <= -pMain->oMap.iPixelsPerTileX) {
                CState.ScrollOffsetX += pMain->oMap.iPixelsPerTileX;
                pMain->oMap.iTileX += 2;
                if (pMain->oMap.iTileX > mx) {
                    if (*map_toggle_flat) {
                        pMain->oMap.iTileX = mx;
                        pMain->oMap.iTileY &= ~1;
                        CState.ScrollOffsetX = 0;
                    } else {
                        pMain->oMap.iTileX -= mx;
                    }
                }
            }
        } else if (x > 0 && (*map_toggle_flat)) {
            fScrolled = true;
            CState.ScrollOffsetX = 0;
        }
    }
    if (y && pMain->oMap.iMapTilesEvenY + pMain->oMap.iMapTilesOddY < my) {
        int iMinTileY = pMain->oMap.iMapTilesOddY - 2;
        int iMaxTileY = my + 4 - pMain->oMap.iMapTilesOddY;
        while (pMain->oMap.iTileY < iMinTileY) {
            pMain->oMap.iTileY += 2;
        }
        while (pMain->oMap.iTileY > iMaxTileY) {
            pMain->oMap.iTileY -= 2;
        }
        d = (pMain->oMap.iTileY - iMinTileY) * pMain->oMap.iPixelsPerHalfTileY - (int)CState.ScrollOffsetY;
        if (y < 0 && d > 0 ) {
            if (y < -d)
                y = -d;
            i = (int)CState.ScrollOffsetY;
            CState.ScrollOffsetY -= y;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetY);
            while (CState.ScrollOffsetY >= pMain->oMap.iPixelsPerTileY && pMain->oMap.iTileY - 2 >= iMinTileY) {
                CState.ScrollOffsetY -= pMain->oMap.iPixelsPerTileY;
                pMain->oMap.iTileY -= 2;
            }
        }
        d = (iMaxTileY - pMain->oMap.iTileY + 1) * pMain->oMap.iPixelsPerHalfTileY + (int)CState.ScrollOffsetY;
        if (y > 0 && d > 0) {
            if (y > d)
                y = d;
            i = (int)CState.ScrollOffsetY;
            CState.ScrollOffsetY -= y;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetY);
            while (CState.ScrollOffsetY <= -pMain->oMap.iPixelsPerTileY && pMain->oMap.iTileY + 2 <= iMaxTileY) {
                CState.ScrollOffsetY += pMain->oMap.iPixelsPerTileY;
                pMain->oMap.iTileY += 2;
            }
        }
    }
    if (fScrolled) {
        MapWin_draw_map(pMain, 0);
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
    CState.ScrollOffsetX = pMain->oMap.iMapPixelLeft;
    CState.ScrollOffsetY = pMain->oMap.iMapPixelTop;
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
                dx = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)pMain->oMap.iPixelsPerTileX / -1000.0;

            } else if ((w - p.x) <= iScrollArea && w >= p.x) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)(w - p.x)) / dArea * (dMax - dMin);
                dx = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)pMain->oMap.iPixelsPerTileX / 1000.0;
            }
            if (p.y <= iScrollArea && p.y >= 0) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)p.y) / dArea * (dMax - dMin);
                dy = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)pMain->oMap.iPixelsPerTileY / -1000.0;

            } else if (h - p.y <= iScrollArea && h >= p.y &&
            // These extra conditions will stop movement when the mouse is over the bottom middle console.
            (p.x <= (CState.ScreenSize.x - ConsoleWidth) / 2 ||
             p.x >= (CState.ScreenSize.x - ConsoleWidth) / 2 + ConsoleWidth ||
             h - p.y <= 8 * CState.ScreenSize.y / 768)) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)(h - p.y)) / dArea * (dMax - dMin);
                dy = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)pMain->oMap.iPixelsPerTileY / 1000.0;
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

    // TODO: determine the purpose of this code
    if (fScrolledAtAll) {
        pMain->oMap.drawOnlyCursor = 1;
        MapWin_set_center(pMain, pMain->oMap.iTileX, pMain->oMap.iTileY, 1);
        pMain->oMap.drawOnlyCursor = 0;
        for (int i = 1; i < 8; i++) {
            if (ppMain[i] && ppMain[i]->oMap.field_1DD74 &&
            (!fLeftButtonDown || ppMain[i]->oMap.field_1DD80) &&
            ppMain[i]->oMap.iMapTilesOddX + ppMain[i]->oMap.iMapTilesEvenX < *map_axis_x) {
                MapWin_set_center(ppMain[i], pMain->oMap.iTileX, pMain->oMap.iTileY, 1);
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

    if (This == pMain) {
        debug("mod_gen_map %d %.4f %.4f\n", CState.Scrolling, CState.ScrollOffsetX, CState.ScrollOffsetY);

        // Save these values to restore them later
        int iMapPixelLeft = This->oMap.iMapPixelLeft;
        int iMapPixelTop = This->oMap.iMapPixelTop;
        int iMapTileLeft = This->oMap.iMapTileLeft;
        int iMapTileTop = This->oMap.iMapTileTop;
        int iMapTilesOddX = This->oMap.iMapTilesOddX;
        int iMapTilesOddY = This->oMap.iMapTilesOddY;
        int iMapTilesEvenX = This->oMap.iMapTilesEvenX;
        int iMapTilesEvenY = This->oMap.iMapTilesEvenY;
        // These are just aliased to save typing and are not modified
        int mx = *map_axis_x;
        int my = *map_axis_y;

        if (iMapTilesOddX + iMapTilesEvenX < mx && !(*map_toggle_flat)) {
            if (iMapPixelLeft > 0) {
                This->oMap.iMapPixelLeft -= This->oMap.iPixelsPerTileX;
                This->oMap.iMapTileLeft -= 2;
                This->oMap.iMapTilesEvenX++;
                This->oMap.iMapTilesOddX++;
                if (This->oMap.iMapTileLeft < 0)
                    This->oMap.iMapTileLeft += mx;
            } else if (iMapPixelLeft < 0 ) {
                This->oMap.iMapTilesEvenX++;
                This->oMap.iMapTilesOddX++;
            }
        }
        if (iMapTilesOddY + iMapTilesEvenY < my) {
            if (iMapPixelTop > 0) {
                This->oMap.iMapPixelTop -= This->oMap.iPixelsPerTileY;
                This->oMap.iMapTileTop -= 2;
                This->oMap.iMapTilesEvenY++;
                This->oMap.iMapTilesOddY++;
            } else if (iMapPixelTop < 0) {
                This->oMap.iMapTilesEvenY++;
                This->oMap.iMapTilesOddY++;
            }
        }
        MapWin_gen_map(This, iOwner, fUnitsOnly);
        // Restore This's original values
        This->oMap.iMapPixelLeft = iMapPixelLeft;
        This->oMap.iMapPixelTop = iMapPixelTop;
        This->oMap.iMapTileLeft = iMapTileLeft;
        This->oMap.iMapTileTop = iMapTileTop;
        This->oMap.iMapTilesOddX = iMapTilesOddX;
        This->oMap.iMapTilesOddY = iMapTilesOddY;
        This->oMap.iMapTilesEvenX = iMapTilesEvenX;
        This->oMap.iMapTilesEvenY = iMapTilesEvenY;
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
//    int w = ((CWinBuffed*)((int)This + (int)This->oMap.vtbl->iOffsetofoClass2))->oCanvas.stBitMapInfo.bmiHeader.biWidth;
//    int h = -((CWinBuffed*)((int)This + (int)This->oMap.vtbl->iOffsetofoClass2))->oCanvas.stBitMapInfo.bmiHeader.biHeight;

    if (This == pMain) {
        debug("mod_calc_dim %d %.4f %.4f\n",  CState.Scrolling, CState.ScrollOffsetX, CState.ScrollOffsetY);
        iOldZoom = This->oMap.iLastZoomFactor;
        ptNewTile.x = This->oMap.iTileX;
        ptNewTile.y = This->oMap.iTileY;
        fx = (ptNewTile.x == ptOldTile.x);
        fy = (ptNewTile.y == ptOldTile.y);
        memcpy(&ptOldTile, &ptNewTile, sizeof(POINT));
        MapWin_tile_to_pixel(This, ptNewTile.x, ptNewTile.y, &ptOldCenter.x, &ptOldCenter.y);
        MapWin_calculate_dimensions(This);

        if (CState.Scrolling) {
            This->oMap.iMapPixelLeft = (int)CState.ScrollOffsetX;
            This->oMap.iMapPixelTop = (int)CState.ScrollOffsetY;
        } else if (iOldZoom != -9999) {
            ptScale.x = This->oMap.iPixelsPerTileX;
            ptScale.y = This->oMap.iPixelsPerTileY;
            MapWin_tile_to_pixel(This, ptNewTile.x, ptNewTile.y, &ptNewCenter.x, &ptNewCenter.y);
            dx = ptOldCenter.x - ptNewCenter.x;
            dy = ptOldCenter.y - ptNewCenter.y;
            if (!This->oMap.iMapPixelLeft && fx && dx > -ptScale.x * 2 && dx < ptScale.x * 2) {
                This->oMap.iMapPixelLeft = dx;
            }
            if (!This->oMap.iMapPixelTop && fy && dy > -ptScale.y * 2 && dy < ptScale.y * 2
            && (dy + *screen_height) / ptScale.y < (*map_axis_y - This->oMap.iMapTileTop) / 2) {
                This->oMap.iMapPixelTop = dy;
            }
        }
    } else {
        MapWin_calculate_dimensions(This);
    }
    return 0;
}

int __cdecl mod_blink_timer() {
    if (!*GameHalted && !*ControlRedraw) {
        if (Win_is_visible(BaseWin)) {
            return TutWin_draw_arrow(TutWin);
        }
        if (Win_is_visible(SocialWin)) {
            return TutWin_draw_arrow(TutWin);
        }
        PlanWin_blink(PlanWin);
        StringBox_clip_ids(StringBox, 150);

        if ((!MapWin->field_23BE8 && (!*multiplayer_active || !(*game_state & STATE_UNK_2))) || *ControlTurnA) {
            MapWin->field_23BFC = 0;
            return 0;
        }
        if (MapWin->field_23BE4 || (*multiplayer_active && *game_state & STATE_UNK_2)) {
            MapWin->field_23BF8 = !MapWin->field_23BF8;
            draw_cursor();
            bool mouse_btn_down = GetAsyncKeyState(VK_LBUTTON) < 0;

            if ((int)*phInstance == GetWindowLongA(GetFocus(), GWL_HINSTANCE) && !Win_is_visible(TutWin)) {
                if (MapWin->field_23D80 != -1
                && MapWin->field_23D84 != -1
                && MapWin->field_23D88 != -1
                && MapWin->field_23D8C != -1) {
                    if (*game_preferences & PREF_BSC_MOUSE_EDGE_SCROLL_VIEW || mouse_btn_down || *dword_9B7AE4 != 0) {
                        CState.ScreenSize.x = *screen_width;
                        CState.ScreenSize.y = *screen_height;
                        check_scroll();
                    }
                }
            }
        }
    }
    return 0;
}

LRESULT WINAPI ModWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int iDeltaAccum = 0;
    bool tools = DEBUG && !*GameHalted;
    POINT p;

    if (msg == WM_MOUSEWHEEL && win_has_focus()) {
        int iDelta = GET_WHEEL_DELTA_WPARAM(wParam) + iDeltaAccum;
        iDeltaAccum = iDelta % WHEEL_DELTA;
        iDelta /= WHEEL_DELTA;
        bool zoom_in = (iDelta >= 0);
        iDelta = labs(iDelta);

        if (map_is_visible() && *map_axis_x) {
            int iZoomType = (zoom_in ? 515 : 516);
            for (int i = 0; i < iDelta; i++) {
                if (MapWin->oMap.iZoomFactor > -8 || zoom_in) {
                    Console_zoom(iZoomType, 0);
                }
            }
        } else {
            int iKey = (zoom_in ? VK_UP : VK_DOWN);
            iDelta *= CState.ListScrollDelta;
            for (int i = 0; i < iDelta; i++) {
                PostMessage(hwnd, WM_KEYDOWN, iKey, 0);
                PostMessage(hwnd, WM_KEYUP, iKey, 0);
            }
            return 0;
        }

    } else if (conf.smooth_scrolling && msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) {
        if (!map_is_visible() || !win_has_focus()) {
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

    } else if (msg == WM_ACTIVATEAPP) {
        // If window has just become inactive e.g. ALT+TAB
        if (LOWORD(wParam)) {
            ShowWindow(*phWnd, SW_RESTORE);
        } else {
            //wParam is 0 if the window has become inactive.
            //ShowWindow(*phWnd, SW_MINIMIZE);
        }
        return WinProc(hwnd, msg, wParam, lParam);

    } else if (msg == WM_CHAR && wParam == 't' && alt_key_down()) {
        show_mod_menu();

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
            *game_state |= STATE_DEBUG_MODE;
            *game_preferences |= PREF_ADV_FAST_BATTLE_RESOLUTION;
            *game_more_preferences |=
                (MPREF_ADV_QUICK_MOVE_VEH_ORDERS | MPREF_ADV_QUICK_MOVE_ALL_VEH);
        } else {
            *game_state &= ~STATE_DEBUG_MODE;
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

    } else if (tools && msg == WM_CHAR && wParam == 'y' && alt_key_down()) {
        static int draw_diplo = 0;
        draw_diplo = !draw_diplo;
        if (draw_diplo) {
            MapWin->oMap.iWhatToDrawFlags |= MAPWIN_DRAW_DIPLO_STATE;
            *game_state |= STATE_DEBUG_MODE;
        } else {
            MapWin->oMap.iWhatToDrawFlags &= ~MAPWIN_DRAW_DIPLO_STATE;
        }
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (tools && msg == WM_CHAR && wParam == 'v' && alt_key_down()) {
        MapWin->oMap.iWhatToDrawFlags |= MAPWIN_DRAW_GOALS;
        memset(pm_overlay, 0, sizeof(pm_overlay));
        static int ts_type = 0;
        int i = 0;
        TileSearch ts;
        ts_type = (ts_type+1) % (MaxTileSearchType+1);
        ts.init(MapWin->oMap.iTileX, MapWin->oMap.iTileY, ts_type, 0);
        while (ts.get_next() != NULL) {
            pm_overlay[ts.rx][ts.ry] = ++i;
        }
        pm_overlay[MapWin->oMap.iTileX][MapWin->oMap.iTileY] = ts_type;
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (tools && msg == WM_CHAR && wParam == 'f' && alt_key_down()) {
        MapWin->oMap.iWhatToDrawFlags |= MAPWIN_DRAW_GOALS;
        memset(pm_overlay, 0, sizeof(pm_overlay));
        MAP* sq = mapsq(MapWin->oMap.iTileX, MapWin->oMap.iTileY);
        if (sq && sq->is_owned()) {
            move_upkeep(sq->owner, M_Visual);
            MapWin_draw_map(MapWin, 0);
            InvalidateRect(hwnd, NULL, false);
        }

    } else if (tools && msg == WM_CHAR && wParam == 'x' && alt_key_down()) {
        MapWin->oMap.iWhatToDrawFlags |= MAPWIN_DRAW_GOALS;
        static int px = 0, py = 0;
        int x = MapWin->oMap.iTileX, y = MapWin->oMap.iTileY;
        int unit = is_ocean(mapsq(x, y)) ? BSC_UNITY_FOIL : BSC_UNITY_ROVER;
        path_distance(px, py, x, y, unit, 1);
        px=x;
        py=y;
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (tools && msg == WM_CHAR && wParam == 'z' && alt_key_down()) {
        int x = MapWin->oMap.iTileX, y = MapWin->oMap.iTileY;
        int base_id;
        if ((base_id = base_at(x, y)) >= 0) {
            print_base(base_id);
        }
        print_map(x, y);
        for (int k=0; k < *total_num_vehicles; k++) {
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

/*
Render custom debug overlays with original goals.
*/
void __thiscall MapWin_gen_overlays(Console* This, int x, int y)
{
    Buffer* Canvas = (Buffer*)0x939888;
    RECT rt;
    if (*game_state & STATE_OMNISCIENT_VIEW && MapWin->oMap.iWhatToDrawFlags & MAPWIN_DRAW_GOALS)
    {
        MapWin_tile_to_pixel(This, x, y, &rt.left, &rt.top);
        rt.right = rt.left + This->oMap.iPixelsPerTileX;
        rt.bottom = rt.top + This->oMap.iPixelsPerHalfTileX;

        char buf[20] = {};
        bool found = false;
        int color = 255;
        int value = pm_overlay[x][y];

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
                            color = 249;
                            break;
                        case AI_GOAL_DEFEND:
                            buf[1] = 'd';
                            color = 251;
                            break;
                        case AI_GOAL_SCOUT:
                            buf[1] = 's';
                            color = 253;
                            break;
                        case AI_GOAL_UNK_1:
                            buf[1] = 'n';
                            color = 252;
                            break;
                        case AI_GOAL_COLONIZE:
                            buf[1] = 'c';
                            color = 254;
                            break;
                        case AI_GOAL_TERRAFORM_LAND:
                            buf[1] = 'f';
                            color = 250;
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
            color = (value >= 0 ? 255 : 251);
            _itoa(value, buf, 10);
        }
        if (found || value) {
            Buffer_set_text_color(Canvas, color, 0, 1, 1);
            Buffer_set_font(Canvas, &This->oMap.oFont2, 0, 0, 0);
            Buffer_write_cent_l3(Canvas, buf, &rt, 10);
        }
    }
}

/*
Override Windows API call to give fake screensize values to SMACX while in windowed mode.
When DirectDraw=0 is set, this allows us to force SMACX run in a borderless window, enabling
very fast rendering and full user access to the other windows.
*/
int WINAPI ModGetSystemMetrics(int nIndex) {
    if (conf.windowed) {
        if (nIndex == SM_CXSCREEN) {
            return conf.window_width;
        }
        if (nIndex == SM_CYSCREEN) {
            return conf.window_height;
        }
    }
    return GetSystemMetrics(nIndex);
}

ATOM WINAPI ModRegisterClassA(WNDCLASS* pstWndClass) {
    pstWndClass->lpfnWndProc = ModWinProc;
    return RegisterClassA(pstWndClass);
}

void __cdecl mod_turn_timer() {
    // Timer calls function every 500ms
    static uint32_t iter = 0;
    static uint32_t prev_time = 0;
    turn_timer();
    if (++iter & 1) {
        return;
    }
    if (*GameHalted && !*map_random_seed) {
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

int __cdecl X_pop2(const char* label, int a2) {
    if (!strcmp(label, "MIMIMI") && !conf.warn_on_former_replace) {
        return 1;
    }
    /*
    Disable unnecessary warning if the map is larger than 128x128.
    In any case the dialog will limit all map sizes to 256x256.
    */
    if (!strcmp(label, "VERYLARGEMAP")) {
        return 0;
    }
    return X_pop("SCRIPT", label, -1, 0, 0, a2);
}

int __cdecl X_pop7(const char* label, int a2, int a3) {
    return X_pop(ScriptFile, label, -1, 0, a2, a3);
}

int __cdecl X_pops4(const char* label, int a2, Sprite* a3, int a4)
{
    return X_pops(ScriptFile, label, -1, 0, a2, (int)a3, 1, 1, a4);
}

int __cdecl X_dialog(const char* label, int faction2)
{
    return X_pops(ScriptFile, label, -1, 0, PopDialogUnk100000,
        (int)FactionPortraits[faction2], 1, 1, (int)sub_5398E0);
}

int __cdecl X_dialog(const char* filename, const char* label, int faction2)
{
    return X_pops(filename, label, -1, 0, PopDialogUnk100000,
        (int)FactionPortraits[faction2], 1, 1, (int)sub_5398E0);
}

void popup_homepage() {
    ShellExecute(NULL, "open", "https://github.com/induktio/thinker", NULL, NULL, SW_SHOWNORMAL);
}

void show_mod_stats() {
    int total_pop = 0,
        total_minerals = 0,
        total_energy = 0,
        faction_pop = 0,
        faction_units = 0,
        faction_minerals = 0,
        faction_energy = 0;

    Faction* f = &Factions[MapWin->cOwner];
    for (int i = 0; i < *total_num_bases; ++i) {
        BASE* b = &Bases[i];
        int mindiv = (has_project(b->faction_id, FAC_SPACE_ELEVATOR)
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
    for (int i = 0; i < *total_num_vehicles; i++) {
        VEH* v = &Vehicles[i];
        if (v->faction_id == MapWin->cOwner) {
            faction_units++;
        }
    }
    ParseNumTable[0] = *total_num_bases;
    ParseNumTable[1] = *total_num_vehicles;
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

int show_mod_config() {
    enum {
        MapGen = 1,
        MapContinents = 2,
        MapLandmarks = 4,
        MapMirrorX = 8,
        MapMirrorY = 16,
        MapLabels = 32,
        AutoBases = 64,
        AutoUnits = 128,
        FormerReplace = 256,
    };
    *DialogChoices = 0
        | (conf.new_world_builder ? MapGen : 0)
        | (conf.world_continents ? MapContinents : 0)
        | (conf.modified_landmarks ? MapLandmarks : 0)
        | (conf.world_map_labels ? MapLabels : 0)
        | (conf.map_mirror_x ? MapMirrorX : 0)
        | (conf.map_mirror_y ? MapMirrorY : 0)
        | (conf.manage_player_bases ? AutoBases : 0)
        | (conf.manage_player_units ? AutoUnits : 0)
        | (conf.warn_on_former_replace ? FormerReplace : 0);

    // Return value is equal to choices bitfield if OK pressed, -1 otherwise.
    int value = X_pop("modmenu", "OPTIONS", -1, 0, PopDialogCheckbox|PopDialogBtnCancel, 0);
    if (value < 0) {
        return 0;
    }
    conf.new_world_builder = !!(value & MapGen);
    WritePrivateProfileStringA(ModAppName, "new_world_builder",
        (conf.new_world_builder ? "1" : "0"), GameIniFile);

    conf.world_continents = !!(value & MapContinents);
    WritePrivateProfileStringA(ModAppName, "world_continents",
        (conf.world_continents ? "1" : "0"), GameIniFile);

    conf.modified_landmarks = !!(value & MapLandmarks);
    WritePrivateProfileStringA(ModAppName, "modified_landmarks",
        (conf.modified_landmarks ? "1" : "0"), GameIniFile);

    conf.map_mirror_x = !!(value & MapMirrorX);
    WritePrivateProfileStringA(ModAppName, "map_mirror_x",
        (conf.map_mirror_x ? "1" : "0"), GameIniFile);

    conf.map_mirror_y = !!(value & MapMirrorY);
    WritePrivateProfileStringA(ModAppName, "map_mirror_y",
        (conf.map_mirror_y ? "1" : "0"), GameIniFile);

    conf.world_map_labels = !!(value & MapLabels);
    WritePrivateProfileStringA(ModAppName, "world_map_labels",
        (conf.world_map_labels ? "1" : "0"), GameIniFile);

    conf.manage_player_bases = !!(value & AutoBases);
    WritePrivateProfileStringA(ModAppName, "manage_player_bases",
        (conf.manage_player_bases ? "1" : "0"), GameIniFile);

    conf.manage_player_units = !!(value & AutoUnits);
    WritePrivateProfileStringA(ModAppName, "manage_player_units",
        (conf.manage_player_units ? "1" : "0"), GameIniFile);

    conf.warn_on_former_replace = !!(value & FormerReplace);
    WritePrivateProfileStringA(ModAppName, "warn_on_former_replace",
        (conf.warn_on_former_replace ? "1" : "0"), GameIniFile);

    draw_map(1);
    return 0;
}

int show_mod_menu() {
    parse_says(0, MOD_VERSION, -1, -1);
    parse_says(1, MOD_VERSION, -1, -1);
    parse_says(2, MOD_DATE, -1, -1);

    if (*GameHalted) {
        int ret = popp("modmenu", "MAINMENU", 0, "stars_sm.pcx", 0);
        if (ret == 1) {
            popup_homepage();
        }
        return 0;
    }
    uint64_t seconds = ThinkerVars->game_time_spent / 1000;
    ParseNumTable[0] = seconds / 3600;
    ParseNumTable[1] = (seconds / 60) % 60;
    ParseNumTable[2] = seconds % 60;
    int ret = popp("modmenu", "GAMEMENU", 0, "stars_sm.pcx", 0);

    if (ret == 1 && !*GameHalted && !*pbem_active && !*multiplayer_active) {
        show_mod_stats();
    }
    else if (ret == 2 && !*GameHalted) {
        show_mod_config();
    }
    else if (ret == 3) {
        popup_homepage();
    }
    return 0;
}

static int minimal_cost = 0;

int __thiscall BaseWin_popup_start(Win* This,
const char* UNUSED(filename), const char* UNUSED(label), int a4, int a5, int a6, int a7)
{
    BASE* base = *current_base_ptr;
    Faction* f = &Factions[base->faction_id];
    int mins = mineral_cost(*current_base_id, base->queue_items[0])
        - base->minerals_accumulated - base->mineral_surplus;
    minimal_cost = hurry_cost(*current_base_id, base->queue_items[0], mins);
    ParseNumTable[1] = minimal_cost;
    ParseNumTable[2] = f->energy_credits - f->energy_cost;
    return Popup_start(This, "modmenu", "HURRY", a4, a5, a6, a7);
}

int __cdecl BaseWin_ask_number(const char* label, int value, int a3)
{
    ParseNumTable[0] = value;
    return pop_ask_number(ScriptFile, label, minimal_cost, a3);
}

void __thiscall Basewin_draw_farm_set_font(Buffer* This, Font* font, int a3, int a4, int a5)
{
    BASE* base = *current_base_ptr;
    char buf[256] = {};
    int x = ((int*)BaseWin)[66277] - 2;
    int y = ((int*)BaseWin)[66274] - 26;

    if (base && x > 0 && y > 0) {
        if (base->nerve_staple_turns_left > 0) {
            snprintf(buf, 256, "Nerve staple: %d turns", base->nerve_staple_turns_left);
            Buffer_set_font(This, font, 0, 0, 0);
            Buffer_set_text_color(This, 7, 0, 1, 1); // 7 = gray color
            Buffer_write_right_l2(This, buf, x, y, strlen(buf));
        }
    } else {
        assert(0);
    }
    Buffer_set_font(This, font, a3, a4, a5);
}

/*
Refresh base window workers properly after nerve staple is done.
*/
void __cdecl BaseWin_action_staple(int base_id) {
    action_staple(base_id);
    base_compute(base_id);
    BaseWin_on_redraw(BaseWin);
}

/*
This is called when ReportWin is closing and it is to refresh world_map_labels
on any bases where workers have been adjusted from the base list window.
*/
int __thiscall ReportWin_close_handler(void* This) {
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
            MAP* sq = *MapPtr;
            for (int i = 0; i < *map_area_tiles; ++i, ++sq) {
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
    && (*game_state & STATE_SCENARIO_EDITOR || sq->is_visible(MapWin->cOwner))) {
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

int __thiscall mod_NetMsg_pop(void* This, char* label, int delay, int a4, void* a5)
{
    if (!strcmp(label, "GOTMYPROBE")) {
        return NetMsg_pop(This, label, -1, a4, a5);
    }
    if (!strcmp(label, netmsg_label)
    && !strcmp((char*)&ParseStrBuffer[0], netmsg_item0)
    && !strcmp((char*)&ParseStrBuffer[1], netmsg_item1)) {
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

int base_trade_value(int base_id, int faction1, int faction2)
{
    if (base_id < 0 || base_id >= *total_num_bases) {
        return 0;
    }
    BASE* base = &Bases[base_id];
    bool own_base = base->faction_id == faction2;
    bool pact = has_pact(faction1, faction2);
    int hq_id = find_hq(faction2);
    int value = 50*base->pop_size;
    int friction_score = *diff_level*2 + clamp(*DiploFriction, 0, 20)
        + 4*max(0, Factions[faction1].ranking - 4)
        + (Factions[faction1].pop_total > Factions[faction2].pop_total ? 4 : 0)
        + 4*Factions[faction1].integrity_blemishes
        + 2*Factions[faction1].atrocities;

    for (int i = Fac_ID_First; i < Fac_ID_Last; i++) {
        if (has_fac_built(base_id, i)) {
            value += Facility[i].cost * 20;
        }
    }
    for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
        if (SecretProjects[i - SP_ID_First] == base_id) {
            value += Facility[i].cost * (own_base && !pact ? 40 : 20)
                + 60 * clamp(project_score(faction2, i, false), 0, 10);
        }
    }
    MAP* sq;
    TileSearch ts;
    ts.init(base->x, base->y, TRIAD_AIR);
    int num_own = 0;
    int num_all = 0;
    while ((sq = ts.get_next()) != NULL && ts.dist <= 10 && num_all < 5) {
        if (sq->is_base()) {
            if (sq->owner == faction2) {
                num_own++;
            }
            num_all++;
        }
    }
    for (auto& m : iterate_tiles(base->x, base->y, 0, 21)) {
        if (mod_base_find3(m.x, m.y, -1, m.sq->region, -1, -1) == base_id) {
            if (m.sq->landmarks & ~(LM_DUNES|LM_SARGASSO|LM_UNITY)) {
                value += (m.sq->landmarks & LM_JUNGLE ? 20 : 15);
            }
            if (mod_bonus_at(m.x, m.y) > 0) {
                value += 20;
            }
            if (m.sq->items & (BIT_MONOLITH|BIT_CONDENSER|BIT_THERMAL_BORE)) {
                value += 20;
            }
        }
    }
    if (own_base) {
        value += 80;
        for (int i = 0; i < *total_num_vehicles; i++) {
            if ((Vehs[i].x == base->x && Vehs[i].y == base->y)
            || Vehs[i].home_base_id == base_id) {
                value += Units[Vehs[i].unit_id].cost * 20;
            }
        }
        if (num_own > 0) {
            value = value * (num_own == num_all ? 5 : 3) / 2;
        }
        value = value * (base->assimilation_turns_left > 8 ? 3 : 4)
            * (max(0, friction_score) + (pact ? 32 : 40)) / (32 * 4);
    } else {
        if (num_own < num_all) {
            value = value * (num_own ? 5 : 3) / 8;
        }
    }
    if (hq_id >= 0) {
        int dist = map_range(base->x, base->y, Bases[hq_id].x, Bases[hq_id].y);
        value = value * (72 - clamp(dist, 8, 40)) / 64;
    }
    if (base->event_flags & BEVENT_OBJECTIVE || *game_rules & RULES_SCN_VICT_ALL_BASE_COUNT_OBJ) {
        value *= 2;
    }
    debug("base_trade_value %s %d %d friction: %d num_own: %d num_all: %d value: %d\n",
    base->name, faction1, faction2, friction_score, num_own, num_all, value);
    flushlog();
    return value;
}

int __cdecl mod_base_swap(int faction1, int faction2)
{
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];
    bool surrender = has_treaty(faction2, faction1, DIPLO_PACT)
        && has_treaty(faction2, faction1, DIPLO_HAVE_SURRENDERED);

    if (*multiplayer_active || *pbem_active || *diplo_ask_base_swap_id < 0 || is_human(faction2)) {
        if (surrender) {
            return base_swap(faction1, faction2);
        }
        X_dialog("NEVERBASESWAP", faction2);
        return 0;
    }
    if (want_revenge(faction2, faction1) || f_cmp.diplo_betrayed[faction1] > 0) {
        X_dialog("NEVERBASESWAP", faction2);
        return 0;
    }
    if (*diplo_counter_proposal_id == DiploCounterThreaten) {
        cause_friction(faction2, faction1, 2);
        parse_says(0, MFactions[faction1].title_leader, -1, -1);
        parse_says(1, MFactions[faction1].name_leader, -1, -1);
        X_dialog("BASENOCEDE", faction2);
        return 0;
    }
    int cost_ask = base_trade_value(*diplo_ask_base_swap_id, faction1, faction2);
    parse_says(0, Bases[*diplo_ask_base_swap_id].name, -1, -1);
    ParseNumTable[0] = cost_ask;

    if (cost_ask < 1 || f_cmp.base_count == 1
    || has_fac_built(*diplo_ask_base_swap_id, FAC_HEADQUARTERS)) {
        X_dialog("NOBASESWAP2", faction2);
        return 0;
    }
    if (*diplo_counter_proposal_id == DiploCounterEnergyPayment
    || *diplo_counter_proposal_id == DiploCounterNameAPrice) {
        if (8*f_plr.energy_credits < cost_ask) {
            X_dialog("NOBASESWAP2", faction2);
            return 0;
        }
        if (f_plr.energy_credits < cost_ask) {
            X_dialog("NOBASESWAP", faction2);
            return 0;
        }
        if (!X_dialog("PAYBASESWAP", faction2)) {
            return 0;
        }
        f_plr.energy_credits -= cost_ask;
        f_cmp.energy_credits += cost_ask;
        give_a_base(*diplo_ask_base_swap_id, faction1);
        StatusWin_redraw(StatusWin);
        draw_map(1);
        return 0;
    }
    if (*diplo_counter_proposal_id == DiploCounterGiveBase && *diplo_bid_base_swap_id >= 0) {
        int cost_bid = base_trade_value(*diplo_bid_base_swap_id, faction1, faction2);
        parse_says(0, Bases[*diplo_ask_base_swap_id].name, -1, -1);
        parse_says(1, Bases[*diplo_bid_base_swap_id].name, -1, -1);

        if (cost_bid < 1 && !surrender) {
            X_dialog("NOBASESWAP2", faction2);
            return 0;
        }
        if (4*cost_bid < cost_ask && !surrender) {
            X_dialog("NOBASESWAP3", faction2);
            return 0;
        }
        if (cost_bid < cost_ask && !surrender) {
            X_dialog("NOBASESWAP", faction2);
            return 0;
        }
        X_dialog("YESBASESWAP", faction2);
        give_a_base(*diplo_ask_base_swap_id, faction1);
        give_a_base(*diplo_bid_base_swap_id, faction2);
        StatusWin_redraw(StatusWin);
        draw_map(1);
        return 0;
    }
    if (*diplo_counter_proposal_id == DiploCounterFriendship) {
        // skipped
    }
    if (*diplo_counter_proposal_id == DiploCounterResearchData) {
        // skipped
    }
    parse_says(0, MFactions[faction1].title_leader, -1, -1);
    parse_says(1, MFactions[faction1].name_leader, -1, -1);
    X_dialog("modmenu", "REJECTIDEA", faction2);
    return 0;
}

int __cdecl mod_energy_trade(int faction1, int faction2)
{
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];

    if (*diplo_current_proposal_id == DiploProposalNeedEnergy
    && *diplo_counter_proposal_id == DiploCounterResearchData
    && f_cmp.energy_credits < 100) {
        tech_analysis(faction1, faction2);
        parse_says(0, MFactions[faction1].title_leader, -1, -1);
        parse_says(1, MFactions[faction1].name_leader, -1, -1);
        if (*diplo_entry_id < 0) {
            X_dialog("REJSELLNONE", faction2);
        } else {
            X_dialog("REJSELLAFFORD", faction2);
        }
        return 0;
    }
    if (*multiplayer_active || *diplo_current_proposal_id != DiploProposalNeedEnergy
    || *diplo_counter_proposal_id != DiploCounterLoanPayment) {
        return energy_trade(faction1, faction2);
    }

    bool is_pact = has_treaty(faction1, faction2, DIPLO_PACT);
    bool is_treaty = has_treaty(faction1, faction2, DIPLO_TREATY);
    int friction = clamp(*DiploFriction, 0, 20);
    int score = 5 - friction
        - 16*f_cmp.diplo_betrayed[faction1]
        - 8*f_plr.integrity_blemishes
        - 4*f_plr.atrocities
        + 2*f_plr.diplo_gifts[faction2]
        + (is_pact ? 10 : 0)
        + (is_treaty ? 5 : 0)
        + (want_revenge(faction2, faction1) ? -50 : 0)
        + (f_plr.pop_total > f_cmp.pop_total ? -5 : 0)
        + (f_plr.labs_total > 2*f_cmp.labs_total ? -5 : 0)
        + (2*f_plr.pop_total > 3*f_cmp.pop_total ? -10 : 0)
        + (3*f_plr.pop_total < 2*f_cmp.pop_total ? 5 : 0);

    for (int i=1; i < MaxPlayerNum; i++) {
        if (Factions[i].base_count && i != faction1 && i != faction2) {
            if (at_war(i, faction1) && has_pact(i, faction2)) {
                score -= 5;
            }
            if (at_war(i, faction2) && has_pact(i, faction1)) {
                score -= 5;
            }
            if (at_war(i, faction1) && at_war(i, faction2)) {
                score += 5;
            }
        }
    }
    int reserve = clamp(*current_turn * f_cmp.base_count / 8, 50, 200 + 20*friction);
    int amount = clamp(f_cmp.energy_credits - reserve, 0, *current_turn * 4)
        * (f_plr.pop_total > f_cmp.pop_total ? 1 : 2)
        * clamp(20 + score - 16*f_cmp.AI_fight + 16*clamp(f_cmp.SE_economy_base, -1, 1)
        - 20*(f_plr.ranking > 5) + 20*is_pact, 16, 64) / (128 * 20) * 20;
    int turns = clamp(50 - *diff_level*5 - friction + min(20, score), 10, 50);
    int payment = ((18 + friction/4 + *diff_level*2)*amount + 15) / (16*turns);

    debug("energy_trade %s score: %d friction: %d credits: %d reserve: %d amount: %d turns: %d payment: %d\n",
    MFactions[faction2].filename, score, friction, f_cmp.energy_credits, reserve, amount, turns, payment);
    flushlog();

    if (f_plr.sanction_turns > 0 || score < -10) {
        parse_says(0, MFactions[faction1].title_leader, -1, -1);
        parse_says(1, MFactions[faction1].name_leader, -1, -1);
        X_dialog("modmenu", "REJECTIDEA", faction2);
        return 0;
    }
    if (f_plr.loan_balance[faction2] > 0) {
        X_dialog("modmenu", "REJECTLOAN", faction2);
        return 0;
    }
    if (amount < 10 || payment < 1 || score < 0) {
        X_dialog(random(2) ? "REJENERGY0" : "REJENERGY1", faction2);
        return 0;
    }

    parse_says(0, MFactions[faction1].title_leader, -1, -1);
    parse_says(1, MFactions[faction1].name_leader, -1, -1);
    ParseNumTable[0] = amount;
    ParseNumTable[1] = payment;
    ParseNumTable[2] = turns;
    int choice = X_dialog(random(2) ? "ENERGYLOAN1" : "ENERGYLOAN2", faction2);

    if (choice == 1) {
        f_plr.loan_balance[faction2] = turns * payment;
        f_plr.loan_payment[faction2] = payment;
        f_plr.energy_credits += amount;
        f_cmp.energy_credits -= amount;
        StatusWin_redraw(StatusWin);
    }
    return 0;
}

int tech_ask_value(int faction1, int faction2, int tech_id, bool high_price)
{
    CTech& tech = Tech[tech_id];
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];

    int level = clamp(tech_level(tech_id, 0), 1, 16);
    int owners = __builtin_popcount(TechOwners[tech_id]);
    int weights = clamp((f_cmp.AI_growth + 1) * tech.AI_growth
        + (f_cmp.AI_power + 1) * tech.AI_power
        + (f_cmp.AI_tech + 1) * tech.AI_tech
        + (f_cmp.AI_wealth + 1) * tech.AI_wealth, 0, 32);
    int value = (20 + 35*level + 4*level*level)
        * (64 + weights/2 + *diff_level*4 + clamp(*DiploFriction, 0, 20)
        + 8*f_plr.integrity_blemishes + 4*f_plr.atrocities) / 64;

    if (*game_rules & RULES_TECH_STAGNATION) {
        value = value * 3 / 2;
    }
    if (high_price) {
        value = value * 3 / 2;
    }
    if (f_plr.ranking > 5) {
        value = value * 5 / 4;
    }
    if (f_plr.pop_total > f_cmp.pop_total) {
        value = value * 5 / 4;
    }
    if (owners > 1) {
        value = value * 3 / 4;
    }
    if (has_pact(faction1, faction2)) {
        value = value * 3 / 4;
    }
    if (conf.revised_tech_cost && f_plr.tech_research_id == tech_id) {
        value = (int)(value * (1.0 - clamp(
            1.0 * f_plr.tech_accumulated / tech_cost(faction1, tech_id), 0.0, 0.7)));
    }
    value = (value + 24) / 25 * 25;

    debug("tech_ask_value %d %d level: %d owners: %d weights: %d value: %d tech: %d %s\n",
        faction1, faction2, level, owners, weights, value, tech_id, Tech[tech_id].name);
    flushlog();
    return value;
}

int __cdecl mod_buy_tech(int faction1, int faction2, int counter_id, bool high_price, int proposal_id)
{
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];

    if (*multiplayer_active) {
        if (!has_pact(faction1, faction2)) {
            X_dialog(random(2) ? "REJTECHLATER0" : "REJTECHLATER1", faction2);
            return 1;
        }
        return buy_tech(faction1, faction2, counter_id, high_price, proposal_id);
    }

    if (proposal_id == DiploProposalTechTrade
    && (counter_id == DiploCounterEnergyPayment
    || counter_id == DiploCounterNameAPrice
    || counter_id == DiploCounterResearchData)) {
        if (*diplo_tech_id1 < 0) {
            return 0; // "no technical data in which you would be interested"
        }
        if (f_plr.sanction_turns > 0 || f_cmp.diplo_betrayed[faction1]
        || want_revenge(faction2, faction1)) {
            X_dialog(random(2) ? "REJTECHLATER0" : "REJTECHLATER1", faction2);
            return 1; // skip additional dialog replies
        }
        int value = tech_ask_value(faction1, faction2, *diplo_tech_id1, high_price);
        if (value < 1) {
            return 0;
        }
        StrBuffer[0] = '\0';
        say_tech((int)StrBuffer, *diplo_tech_id1, 1);
        parse_says(0, StrBuffer, -1, -1);
        ParseNumTable[0] = value;

        if (8*f_plr.energy_credits < value) {
            X_dialog(random(2) ? "REJTECHLATER0" : "REJTECHLATER1", faction2);
            return 1;
        }
        if (f_plr.energy_credits < value) {
            X_dialog(random(2) ? "BUYTECHHIGH0" : "BUYTECHHIGH1", faction2);
            return 1;
        }
        if (X_dialog(random(2) ? "BUYTECH0" : "BUYTECH1", faction2) == 1) {
            f_plr.energy_credits -= value;
            f_cmp.energy_credits += value;
            tech_achieved(faction1, *diplo_tech_id1, faction2, 0);
            StatusWin_redraw(StatusWin);
        }
        return 1;
    }
    return buy_tech(faction1, faction2, counter_id, high_price, proposal_id);
}



