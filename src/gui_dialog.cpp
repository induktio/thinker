
#include "gui_dialog.h"


int __thiscall Popup_load_icon(Popup* This, Sprite* a2, int a3, char* a4) {
    // replace inlined function sub_559000
    if (This->dialogs.spriteBox.strings.count) {
        This->sprite_icon = 0;
    } else {
        This->sprite_icon = a2;
    }
    return SpriteBox_sprite((SpriteBox*)&This->dialogs.spriteBox, a2, a4, a3);
}

int __cdecl X_pop(const char* label, fp_none fn)
{
    if (!conf.warn_on_former_replace && !strcmp(label, "MIMIMI")) {
        return 1;
    }
    return X_pop_9(ScriptFile, label, -1, 0, 0, fn);
}

int __cdecl X_pop_2(const char* filename, const char* label, fp_none fn)
{
    return X_pop_9(filename, label, -1, 0, 0, fn);
}

int __cdecl X_pop_6(const char* label, int a2, fp_none fn)
{
    return X_pop_9(ScriptFile, label, -1, 0, a2, fn);
}

int __cdecl X_pops(const char* label, Sprite* a2, fp_none fn)
{
    return X_pops_18(ScriptFile, label, -1, 0, 0, a2, 1, 1, fn);
}

int __cdecl X_pops_11(const char* label, int a2, Sprite* a3, fp_none fn)
{
    return X_pops_18(ScriptFile, label, -1, 0, a2, a3, 1, 1, fn);
}

int __cdecl X_dialog(const char* label, int faction_id)
{
    return X_pops_18(ScriptFile, label, -1, 0, POPUP_UNK_100000,
        FactionPortraits[faction_id], 1, 1, pop_wait);
}

int __cdecl X_dialog_alt(const char* filename, const char* label, int faction_id)
{
    return X_pops_18(filename, label, -1, 0, POPUP_UNK_100000,
        FactionPortraits[faction_id], 1, 1, pop_wait);
}



