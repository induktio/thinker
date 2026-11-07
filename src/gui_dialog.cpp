
#include "gui_dialog.h"


void Popup_init_icon(Popup* This, const char* filename, int* px, int* py) {
    if (!Buffer_get_pcx_dimensions(filename, px, py)) {
        if (!Sprite_init(&This->sprite, filename, *px, *py)) {
            This->sprite_icon = &This->sprite;
        }
    }
}

int __thiscall Popup_load_icon(Popup* This, Sprite* image, int a3, void* a4) {
    // replace inlined function sub_559000
    if (This->dialogs.spriteBox.strings.count) {
        This->sprite_icon = 0;
    } else {
        This->sprite_icon = image;
    }
    return SpriteBox_sprite((SpriteBox*)&This->dialogs.spriteBox, image, a4, a3);
}

int __cdecl X_pop(const char* label, fp_none fn) {
    if (!conf.warn_on_former_replace && !strcmp(label, "MIMIMI")) {
        return 1;
    }
    return X_pop_9(ScriptFile, label, -1, 0, 0, fn);
}

int __cdecl X_pop_2(const char* filename, const char* label, fp_none fn) {
    return X_pop_9(filename, label, -1, 0, 0, fn);
}

int __cdecl X_pop_6(const char* label, uint32_t flags, fp_none fn) {
    return X_pop_9(ScriptFile, label, -1, 0, flags, fn);
}

int __cdecl X_pops(const char* label, Sprite* image, fp_none fn) {
    return X_pops_18(ScriptFile, label, -1, 0, 0, image, 1, 1, fn);
}

int __cdecl X_pops_11(const char* label, uint32_t flags, Sprite* image, fp_none fn) {
    return X_pops_18(ScriptFile, label, -1, 0, flags, image, 1, 1, fn);
}

int __cdecl X_dialog(const char* label, int faction_id) {
    return X_pops_18(ScriptFile, label, -1, 0, POPUP_EXTERNAL,
        FactionPortraits[faction_id], 1, 1, pop_wait);
}

int __cdecl X_dialog_alt(const char* filename, const char* label, int faction_id) {
    return X_pops_18(filename, label, -1, 0, POPUP_EXTERNAL,
        FactionPortraits[faction_id], 1, 1, pop_wait);
}



