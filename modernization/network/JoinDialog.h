#pragma once
#include "Protocol.h"
#include <string>
#include <vector>
#include <windows.h>
namespace dt {
struct DialogState {
    bool host;
    std::string code;
    uint64_t lobby = 0;
    HFONT font = nullptr;
};
inline void Clipboard(HWND window, const std::string &s) {
    if (!OpenClipboard(window))
        return;
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, s.size() + 1);
    if (mem) {
        void *p = GlobalLock(mem);
        if (p) {
            memcpy(p, s.c_str(), s.size() + 1);
            GlobalUnlock(mem);
            EmptyClipboard();
            if (!SetClipboardData(CF_TEXT, mem))
                GlobalFree(mem);
        } else
            GlobalFree(mem);
    }
    CloseClipboard();
}
inline INT_PTR CALLBACK DialogProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    auto *st = reinterpret_cast<DialogState *>(GetWindowLongPtrW(w, GWLP_USERDATA));
    if (msg == WM_INITDIALOG) {
        st = reinterpret_cast<DialogState *>(lp);
        SetWindowLongPtrW(w, GWLP_USERDATA, lp);
        st->font = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
                               CLEARTYPE_QUALITY, 0, L"Segoe UI");
        SetWindowTextW(w, st->host ? L"Your multiplayer join code" : L"Steam multiplayer");
        CreateWindowW(L"STATIC",
                      st->host ? L"Send this code to your friend, then continue to the lobby."
                               : L"Paste your friend's code to join, or choose Host a game.",
                      WS_CHILD | WS_VISIBLE, 16, 15, 520, 36, w, 0, 0, 0);
        HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL |
                                        (st->host ? ES_READONLY : 0),
                                    16, 57, 500, 32, w, (HMENU)100, 0, 0);
        SendMessageW(edit, EM_SETLIMITTEXT, 64, 0);
        SetWindowTextA(edit, st->code.c_str());
        CreateWindowW(L"BUTTON", st->host ? L"Copy code" : L"Host a game",
                      WS_CHILD | WS_VISIBLE | WS_TABSTOP, 270, 110, 115, 34, w, (HMENU)IDCANCEL, 0,
                      0);
        CreateWindowW(L"BUTTON", st->host ? L"Continue" : L"Join",
                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 395, 110, 120, 34, w,
                      (HMENU)IDOK, 0, 0);
        for (HWND c = GetWindow(w, GW_CHILD); c; c = GetWindow(c, GW_HWNDNEXT))
            SendMessageW(c, WM_SETFONT, (WPARAM)st->font, TRUE);
        SetFocus(edit);
        return FALSE;
    }
    if (!st)
        return FALSE;
    if (msg == WM_COMMAND) {
        if (LOWORD(wp) == IDCANCEL) {
            if (st->host)
                Clipboard(w, st->code);
            else
                EndDialog(w, 0);
            return TRUE;
        }
        if (LOWORD(wp) == IDOK) {
            if (!st->host) {
                char b[65]{};
                GetDlgItemTextA(w, 100, b, 65);
                if (!Decode(b, st->lobby)) {
                    MessageBoxW(w, L"That code is incomplete or contains a typing mistake.",
                                L"Check the join code", MB_OK | MB_ICONINFORMATION);
                    return TRUE;
                }
            }
            EndDialog(w, 1);
            return TRUE;
        }
    }
    if (msg == WM_CLOSE) {
        EndDialog(w, 0);
        return TRUE;
    }
    if (msg == WM_DESTROY && st->font)
        DeleteObject(st->font);
    return FALSE;
}
inline bool JoinDialog(bool host, std::string code, uint64_t &lobby) {
    struct Template {
        DLGTEMPLATE dialog;
        WORD menu, windowClass, title;
    } t{};
    t.dialog.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
    t.dialog.cx = 280;
    t.dialog.cy = 90;
    DialogState state{host, code};
    INT_PTR r = DialogBoxIndirectParamW(GetModuleHandleW(nullptr), &t.dialog, GetActiveWindow(),
                                        DialogProc, (LPARAM)&state);
    if (r == 1 && !host)
        lobby = state.lobby;
    return r == 1;
}
} // namespace dt
