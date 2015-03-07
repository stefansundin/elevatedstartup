/*
  Copyright (C) 2015  Stefan Sundin

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
*/

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0600
#define _WIN32_IE 0x0600

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>

// App
#define APP_NAME            L"ElevatedStartup"
#define APP_VERSION         "0.1"

// Boring stuff
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HINSTANCE g_hinst = NULL;
HWND g_hwnd = NULL;

// Cool stuff
wchar_t startup_path[MAX_PATH];

// Include stuff
#include "include/resource.h"
#include "include/error.c"
#include "include/autostart.c"


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, char *szCmdLine, int iCmdShow) {
  g_hinst = hInst;

  short start = 0;
  if (!strcmp(szCmdLine,"-start")) {
    start = 1;
  }

  // Check if elevated if in >= Vista
  short elevated = 0;
  OSVERSIONINFO vi = { sizeof(OSVERSIONINFO) };
  GetVersionEx(&vi);
  short vista = (vi.dwMajorVersion >= 6);
  if (vista) {
    HANDLE token;
    TOKEN_ELEVATION elevation;
    DWORD len;
    if (OpenProcessToken(GetCurrentProcess(),TOKEN_READ,&token) && GetTokenInformation(token,TokenElevation,&elevation,sizeof(elevation),&len)) {
      elevated = elevation.TokenIsElevated;
    }
  }

  if (start && !elevated) {
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, ARRAY_SIZE(path));
    int ret = (INT_PTR) ShellExecute(NULL, L"runas", path, L"-start", NULL, SW_SHOWNORMAL);
    if (ret <= 32) {
      Error(L"ShellExecute()", L"Could not elevate to administrator privileges.", GetLastError());
    }
    return 0;
  }


  // Note: this can be updated to SHGetKnownFolderPath when cygwin mingw becomes more recent
  int res = SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, startup_path);
  if (FAILED(res)) {
    Error(L"SHGetFolderPath()", L"Could not get path to start menu.", GetLastError());
    return 1;
  }
  wcscat(startup_path, L"\\ElevatedStartup\\");
  // DBG("startup_path: %s", startup_path);

  if (start) {
    wcscat(startup_path, L"*");

    WIN32_FIND_DATA ffd;
    HANDLE find = FindFirstFile(startup_path, &ffd);
    if (find == INVALID_HANDLE_VALUE) {
      Error(L"FindFirstFile()", L"", GetLastError());
      return 1;
    }

    do {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        continue;
      }

      PathRemoveFileSpec(startup_path);
      wcscat(startup_path, L"\\");
      wcscat(startup_path, ffd.cFileName);

      int ret = (INT_PTR) ShellExecute(NULL, L"open", startup_path, NULL, NULL, SW_SHOWNORMAL);
      if (ret <= 32) {
        Error(L"ShellExecute()", L"", GetLastError());
      }
    } while (FindNextFile(find, &ffd) != 0);

    int err = GetLastError();
    if (err != ERROR_NO_MORE_FILES) {
      Error(L"FindNextFile()", L"", err);
    }

    FindClose(find);

    return 0;
  }

  g_hwnd = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_DIALOG), NULL, DialogProc);
  if (g_hwnd == NULL) {
    Error(L"CreateDialog()", L"", GetLastError());
  }
  SetWindowText(g_hwnd, APP_NAME L" " APP_VERSION);

  HICON app_icon = LoadImage(g_hinst, L"app_icon", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
  SendMessage(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM) app_icon);

  // Message loop
  MSG msg;
  while (GetMessage(&msg,NULL,0,0)) {
    if (!IsDialogMessage(g_hwnd, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return msg.wParam;
}

int OpenUrl(wchar_t *url) {
  int ret = (INT_PTR) ShellExecute(NULL, L"open", url, NULL, NULL, SW_SHOWDEFAULT);
  if (ret <= 32 && MessageBox(NULL,L"Failed to open browser. Copy url to clipboard?",APP_NAME,MB_ICONWARNING|MB_YESNO) == IDYES) {
    int size = (wcslen(url)+1)*sizeof(url[0]);
    wchar_t *data = LocalAlloc(LMEM_FIXED, size);
    memcpy(data, url, size);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, data);
    CloseClipboard();
    LocalFree(data);
  }
  return ret;
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_INITDIALOG) {
    Button_SetCheck(GetDlgItem(hwnd,IDENABLED), CheckAutostart()?BST_CHECKED:BST_UNCHECKED);
  }
  else if (msg == WM_COMMAND) {
    int id = LOWORD(wParam);
    int event = HIWORD(wParam);
    HWND control = GetDlgItem(hwnd, id);
    int val = Button_GetCheck(control);

    if (id == IDENABLED) {
      SetAutostart(val);
    }
    else if (id == IDOPENDIR) {
      ShellExecute(NULL, L"open", startup_path, NULL, NULL, SW_SHOWNORMAL);
    }
    else if (id == IDCANCEL) {
      PostQuitMessage(0);
    }
  }
  else if (msg == WM_DESTROY || msg == WM_CLOSE) {
    PostQuitMessage(0);
  }
  else if (msg == WM_NOTIFY) {
    NMLINK *link = (NMLINK*) lParam;
    if (link->hdr.code == NM_CLICK || link->hdr.code == NM_RETURN) {
      OpenUrl(link->item.szUrl);
    }
  }
  return FALSE;
}
