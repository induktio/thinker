
#include "debug.h"

typedef int(__cdecl *Fexcept_handler3)(EXCEPTION_RECORD*, PVOID, CONTEXT*);
Fexcept_handler3 _except_handler3 = (Fexcept_handler3)0x646DF8;


/*
Replace the default C++ exception handler in SMACX binary with a custom version.
This allows us to get more information about crash locations.
https://en.wikipedia.org/wiki/Microsoft-specific_exception_handling_mechanisms
*/
int __cdecl mod_except_handler3(EXCEPTION_RECORD* rec, PVOID* frame, CONTEXT* ctx)
{
    if (!debug_log && !(debug_log = fopen("debug.txt", "w"))) {
        return _except_handler3(rec, frame, ctx);
    }
    time_t rawtime = time(&rawtime);
    struct tm *now = localtime(&rawtime);
    char datetime[70];
    char savepath[512];
    char filepath[512];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", now);
    strncpy(savepath, LastSavePath, sizeof(savepath));
    savepath[200] = '\0';
    HANDLE hProcess = GetCurrentProcess();
    int ret = GetMappedFileNameA(hProcess, (LPVOID)ctx->Eip, filepath, sizeof(filepath));
    filepath[200] = '\0';

    fprintf(debug_log,
        "************************************************************\n"
        "ModVersion %s (%s)\n"
        "CrashTime  %s\n"
        "SavedGame  %s\n"
        "ModuleName %s\n"
        "************************************************************\n"
        "ExceptionCode    %08x\n"
        "ExceptionFlags   %08x\n"
        "ExceptionRecord  %08x\n"
        "ExceptionAddress %08x\n",
        MOD_VERSION,
        MOD_DATE,
        datetime,
        savepath,
        (ret != 0 ? filepath : ""),
        (int)rec->ExceptionCode,
        (int)rec->ExceptionFlags,
        (int)rec->ExceptionRecord,
        (int)rec->ExceptionAddress);

     fprintf(debug_log,
        "CFlags %08lx\n"
        "EFlags %08lx\n"
        "EAX %08lx\n"
        "EBX %08lx\n"
        "ECX %08lx\n"
        "EDX %08lx\n"
        "ESI %08lx\n"
        "EDI %08lx\n"
        "EBP %08lx\n"
        "ESP %08lx\n"
        "EIP %08lx\n",
        ctx->ContextFlags, ctx->EFlags,
        ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx,
        ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp, ctx->Eip);

    int32_t* p = (int32_t*)&conf;
    for (int32_t i = 0; i < (int32_t)sizeof(conf)/4; i++) {
        fprintf(debug_log, "Config %d: %d\n", i, p[i]);
    }

    fflush(debug_log);
    return _except_handler3(rec, frame, ctx);
}

/*
Original (legacy) game engine logging functions.
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void __cdecl log_say(const char* a1, int a2, int a3, int a4) {
    if (conf.debug_verbose) {
        debug("**** %s %d %d %d\n", a1, a2, a3, a4);
    }
}

void __cdecl log_say2(const char* a1, const char* a2, int a3, int a4, int a5) {
    if (conf.debug_verbose) {
        debug("**** %s %s %d %d %d\n", a1, a2, a3, a4, a5);
    }
}

void __cdecl log_say_hex(const char* a1, int a2, int a3, int a4) {
    if (conf.debug_verbose) {
        debug("**** %s %04x %04x %04x\n", a1, a2, a3, a4);
    }
}

void __cdecl log_say_hex2(const char* a1, const char* a2, int a3, int a4, int a5) {
    if (conf.debug_verbose) {
        debug("**** %s %s %04x %04x %04x\n", a1, a2, a3, a4, a5);
    }
}
#pragma GCC diagnostic pop

