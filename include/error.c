/*
  Copyright (C) 2015  Stefan Sundin

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
*/

LRESULT CALLBACK ErrorMsgProc(INT nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HCBT_ACTIVATE) {
    // Edit the caption of the buttons
    SetDlgItemText((HWND)wParam, IDYES, L"Copy error");
    SetDlgItemText((HWND)wParam, IDNO,  L"OK");
  }
  return 0;
}

void _Error(wchar_t *func, wchar_t *info, int errorcode, wchar_t *file, int line) {
  // Format message
  wchar_t msg[1000], *errormsg;
  int length = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorcode, 0, (wchar_t*)&errormsg, 0, NULL);
  if (length != 0) {
    errormsg[length-2] = '\0'; // Remove that damn newline at the end of the formatted error message
  }
  // swprintf(msg, L"%s failed in file %s, line %d.\nError: %s (%d)\n\n%s", func, file, line, errormsg, errorcode, info);
  swprintf(msg, ARRAY_SIZE(msg), L"%s failed in file %s, line %d.\nError: %s (%d)\n\n%s", func, file, line, errormsg, errorcode, info);
  LocalFree(errormsg);
  // Display message
  // Tip: You can press Ctrl+C in a MessageBox window to copy the text
  HHOOK hhk = SetWindowsHookEx(WH_CBT, &ErrorMsgProc, 0, GetCurrentThreadId());
  int response = MessageBox(NULL, msg, APP_NAME" Error", MB_ICONERROR|MB_YESNO|MB_DEFBUTTON2);
  UnhookWindowsHookEx(hhk);
  if (response == IDYES) {
    // Copy message to clipboard
    int size = (wcslen(msg)+1)*sizeof(msg[0]);
    wchar_t *data = LocalAlloc(LMEM_FIXED, size);
    memcpy(data, msg, size);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, data);
    CloseClipboard();
    LocalFree(data);
  }
}

#define Error(a,b,c) _Error(a, b, c, TEXT(__FILE__), __LINE__)

//DBG("%d", 5);
//DBGA("%d", 5);

#define DBG(fmt, ...) { \
  wchar_t _txt[1000]; \
  wsprintf(_txt, TEXT(fmt), ##__VA_ARGS__); \
  MessageBox(NULL, _txt, APP_NAME" Debug", MB_ICONINFORMATION|MB_OK); \
}

#define DBGA(fmt, ...) { \
  char _txt[1000]; \
  sprintf(_txt, fmt, ##__VA_ARGS__); \
  MessageBoxA(NULL, _txt, "Debug", MB_ICONINFORMATION|MB_OK); \
}
