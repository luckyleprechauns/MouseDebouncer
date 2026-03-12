#include <windows.h>
#include <windowsx.h>
#include <shellapi.h> // Required for Shell_NotifyIcon, ShellExecute
#include <winuser.h>  // For mouse hook
#include <tchar.h>    // For _tcsncpy_s, etc.
#include <shlwapi.h>  // For StrToIntEx
#include <commctrl.h> // For InitCommonControlsEx, SysLink
#include "resource.h" // For dialog and control IDs (in resources/)

// Global Variables
HHOOK mouseHook;
bool leftClickRegistered = false;
bool rightClickRegistered = false;
bool middleClickRegistered = false;
DWORD leftDebounceTime = 0;
DWORD rightDebounceTime = 0;
DWORD middleDebounceTime = 0;
DWORD lastLeftClickTime = 0;
DWORD lastRightClickTime = 0;
DWORD lastMiddleClickTime = 0;
BOOL showOnStartup = TRUE; // Registry-compatible boolean (DWORD)

#define APP_VERSION L"2.0"
#define APP_URL     L"https://github.com/luckyleprechauns/MouseDebouncer"

// System Tray Icon related globals
#define WM_APP_NOTIFYICON (WM_APP + 1)
#define ID_TRAY_ICON 100
#define IDM_SETTINGS 200
#define IDM_EXIT 201

// Function Prototypes
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Registry persistence functions
void LoadDebounceSettings() {
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\MouseDebouncer", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        dwSize = sizeof(DWORD);
        if (RegGetValue(hKey, NULL, L"LeftDebounceTime", RRF_RT_DWORD, &dwType, &leftDebounceTime, &dwSize) != ERROR_SUCCESS) {
            leftDebounceTime = 0;
        }
        dwSize = sizeof(DWORD);
        if (RegGetValue(hKey, NULL, L"RightDebounceTime", RRF_RT_DWORD, &dwType, &rightDebounceTime, &dwSize) != ERROR_SUCCESS) {
            rightDebounceTime = 0;
        }
        dwSize = sizeof(DWORD);
        if (RegGetValue(hKey, NULL, L"MiddleDebounceTime", RRF_RT_DWORD, &dwType, &middleDebounceTime, &dwSize) != ERROR_SUCCESS) {
            middleDebounceTime = 0;
        }
        // Load showOnStartup setting
        dwSize = sizeof(DWORD);
        if (RegGetValue(hKey, NULL, L"ShowOnStartup", RRF_RT_DWORD, &dwType, &showOnStartup, &dwSize) != ERROR_SUCCESS) {
            showOnStartup = TRUE; // Default to showing on startup
        }
        RegCloseKey(hKey);
    } else {
        // If key doesn't exist, use default values
        leftDebounceTime = 0;
        rightDebounceTime = 0;
        middleDebounceTime = 0;
        showOnStartup = TRUE; // Default to showing on startup
    }
}

void SaveDebounceSettings() {
    HKEY hKey;
    DWORD dwDisposition;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\MouseDebouncer", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, L"LeftDebounceTime", 0, REG_DWORD, (const BYTE*)&leftDebounceTime, sizeof(DWORD));
        RegSetValueEx(hKey, L"RightDebounceTime", 0, REG_DWORD, (const BYTE*)&rightDebounceTime, sizeof(DWORD));
        RegSetValueEx(hKey, L"MiddleDebounceTime", 0, REG_DWORD, (const BYTE*)&middleDebounceTime, sizeof(DWORD));
        RegSetValueEx(hKey, L"ShowOnStartup", 0, REG_DWORD, (const BYTE*)&showOnStartup, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// WinMain - Entry point for GUI applications
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Implement singleton pattern using a named mutex
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"MouseDebouncerSingletonMutex");

    if (hMutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is already running
        // Find the existing window and bring it to the foreground
        HWND existingHwnd = FindWindow(L"MouseDebouncerWindowClass", L"Mouse Debouncer");
        if (existingHwnd) {
            // Restore if minimized, then bring to foreground
            if (IsIconic(existingHwnd)) {
                ShowWindow(existingHwnd, SW_RESTORE);
            }
            SetForegroundWindow(existingHwnd);
        }
        CloseHandle(hMutex); // Close the mutex handle
        return 0; // Exit the new instance
    }

    // Load settings on startup
    LoadDebounceSettings();

    // Initialize common controls for SysLink
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LINK_CLASS;
    InitCommonControlsEx(&icex);

    // Register the window class
    LPCWSTR CLASS_NAME = L"MouseDebouncerWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window (hidden initially)
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window style
        CLASS_NAME,                     // Window class
        L"Mouse Debouncer",              // Window title
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,                           // Parent window
        NULL,                           // Menu
        hInstance,                      // Instance handle
        NULL                            // Additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Add the system tray icon
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP_NOTIFYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wcscpy_s(nid.szTip, L"Mouse Debouncer"); // Tooltip text

    Shell_NotifyIcon(NIM_ADD, &nid);

    // Install the mouse hook
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
    if (mouseHook == NULL) {
        MessageBox(NULL, L"Failed to install mouse hook!", L"Error", MB_ICONERROR);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        return 1;
    }

    // Conditionally show settings dialog on startup
    if (showOnStartup) {
        DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), hwnd, SettingsDialogProc, 0);
    }

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Unhook the mouse on exit
    UnhookWindowsHookEx(mouseHook);

    // Remove the system tray icon on exit
    Shell_NotifyIcon(NIM_DELETE, &nid);

    CloseHandle(hMutex); // Release the mutex

    return 0;
}

// Window Procedure - Handles messages for the hidden window
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_APP_NOTIFYICON:
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP: {
                    POINT curPoint;
                    GetCursorPos(&curPoint);

                    HMENU hMenu = CreatePopupMenu();
                    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_SETTINGS, L"Settings...");
                    InsertMenu(hMenu, 1, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"Exit");

                    SetForegroundWindow(hWnd);

                    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
                                   curPoint.x, curPoint.y, 0, hWnd, NULL);
                    DestroyMenu(hMenu);
                    return 0;
                }
                case WM_LBUTTONDBLCLK:
                    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), hWnd, SettingsDialogProc, 0);
                    return 0;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_SETTINGS:
                    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), hWnd, SettingsDialogProc, 0);
                    return 0;
                case IDM_EXIT:
                    DestroyWindow(hWnd); // Will trigger WM_DESTROY
                    return 0;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Settings Dialog Procedure
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            // Set dialog icon
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON)));
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON)));

            //HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_HIGH_RES_ICON), IMAGE_ICON, 128, 128, LR_DEFAULTCOLOR);
            HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_HIGH_RES_ICON), IMAGE_ICON, 128, 128, LR_LOADMAP3DCOLORS);
            Static_SetIcon(GetDlgItem(hDlg, IDC_SETTINGS_MOUSE_ICON), hIcon);

            // Load and set the mouse icon for the static control
            //HBITMAP hMouseBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_HIGH_RES_BITMAP), IMAGE_BITMAP, 128, 128, LR_LOADMAP3DCOLORS);
            //if (hMouseBitmap) {
            //    HWND hStaticIcon = GetDlgItem(hDlg, IDC_SETTINGS_MOUSE_ICON);
            //    if (hStaticIcon) {
            //        SendMessage(hStaticIcon, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hMouseBitmap);
            //    } 
            //} 
            
            // Set initial values from global variables
            TCHAR buffer[16];
            _itow_s(leftDebounceTime, buffer, _countof(buffer), 10); SetDlgItemText(hDlg, IDC_LEFT_DEBOUNCE_EDIT, buffer);
            _itow_s(rightDebounceTime, buffer, _countof(buffer), 10); SetDlgItemText(hDlg, IDC_RIGHT_DEBOUNCE_EDIT, buffer);
            _itow_s(middleDebounceTime, buffer, _countof(buffer), 10); SetDlgItemText(hDlg, IDC_MIDDLE_DEBOUNCE_EDIT, buffer);

            // Set initial state of "Show on Startup" checkbox
            CheckDlgButton(hDlg, IDC_SHOW_ON_STARTUP_CHECKBOX, showOnStartup ? BST_CHECKED : BST_UNCHECKED);

            // Center the dialog on screen
            RECT rc;
            GetWindowRect(hDlg, &rc);
            int dlgWidth = rc.right - rc.left;
            int dlgHeight = rc.bottom - rc.top;
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            SetWindowPos(hDlg, NULL, (screenWidth - dlgWidth) / 2, (screenHeight - dlgHeight) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            return (INT_PTR)TRUE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_APPLY_BUTTON: {
                    TCHAR buffer[16];
                    int newLeftDebounce, newRightDebounce, newMiddleDebounce;
                    BOOL success;

                    GetDlgItemText(hDlg, IDC_LEFT_DEBOUNCE_EDIT, buffer, 16);
                    newLeftDebounce = StrToIntEx(buffer, STIF_DEFAULT, &success) ? _ttoi(buffer) : -1;

                    GetDlgItemText(hDlg, IDC_RIGHT_DEBOUNCE_EDIT, buffer, 16);
                    newRightDebounce = StrToIntEx(buffer, STIF_DEFAULT, &success) ? _ttoi(buffer) : -1;

                    GetDlgItemText(hDlg, IDC_MIDDLE_DEBOUNCE_EDIT, buffer, 16);
                    newMiddleDebounce = StrToIntEx(buffer, STIF_DEFAULT, &success) ? _ttoi(buffer) : -1;

                    if (newLeftDebounce >= 0 && newRightDebounce >= 0 && newMiddleDebounce >= 0) {
                        leftDebounceTime = newLeftDebounce;
                        rightDebounceTime = newRightDebounce;
                        middleDebounceTime = newMiddleDebounce;
                        
                        // Update showOnStartup from checkbox state
                        showOnStartup = (IsDlgButtonChecked(hDlg, IDC_SHOW_ON_STARTUP_CHECKBOX) == BST_CHECKED) ? TRUE : FALSE;

                        SaveDebounceSettings();
                        EndDialog(hDlg, LOWORD(wParam));
                    } else {
                        MessageBox(hDlg, L"Please enter valid positive integer values for debounce times.", L"Input Error", MB_ICONERROR);
                    }
                    return (INT_PTR)TRUE;
                }

                case IDC_CANCEL_BUTTON:
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                
                case IDC_HELP_BUTTON:
                    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, AboutDialogProc, 0);
                    return (INT_PTR)TRUE;
                
                case IDC_EXIT_BUTTON:
                    EndDialog(hDlg, IDOK); // Close the dialog
                    PostQuitMessage(0);    // Terminate the application
                    return (INT_PTR)TRUE;
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;

        case WM_DESTROY: {
            // Retrieve the icon handle from the static control and destroy it
            HICON hIco = (HICON)SendDlgItemMessage(hDlg, IDC_SETTINGS_MOUSE_ICON, STM_GETIMAGE, IMAGE_ICON, 0);
            if (hIco) DestroyIcon(hIco);
            break;
        }

    }
    return (INT_PTR)FALSE;
}

// About Dialog Procedure
INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            // Set dialog icon
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON)));
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON)));

            // Set text for static controls
            SetDlgItemText(hDlg, IDC_ABOUT_VERSION, L"Version " APP_VERSION);
            SetDlgItemText(hDlg, IDC_ABOUT_DESCRIPTION, 
                L"Filters out hardware-related double-clicking issues and improves click precision by debouncing rapid, successive mouse signals caused by mouse button wear.");

            // Credits: Original + Your GUI work
            SetDlgItemText(hDlg, IDC_ABOUT_ATTRIBUTION, 
                L"Original logic by ItzOwo.\n"
                L"GUI & System Tray interface developed by luckyleprechauns.");

            SetDlgItemText(hDlg, IDC_ABOUT_GITHUB_LINK, APP_URL);
            return (INT_PTR)TRUE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;

                case IDC_ABOUT_GITHUB_LINK:
                    // Only trigger if it was a click (STN_CLICKED)
                    if (HIWORD(wParam) == STN_CLICKED) {
                        ShellExecute(NULL, L"open", APP_URL, NULL, NULL, SW_SHOWNORMAL);
                    }
                    return (INT_PTR)TRUE;
            }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR lpnmhdr = (LPNMHDR)lParam;
            if (lpnmhdr->code == NM_CLICK || lpnmhdr->code == NM_RETURN) {
                if (lpnmhdr->idFrom == IDC_ABOUT_GITHUB_LINK) {
                    PNMLINK pNMLink = (PNMLINK)lParam;
                    ShellExecute(NULL, L"open", pNMLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
                }
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

// Mouse Hook Procedure
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_LBUTTONDOWN) {
            DWORD currentTime = GetTickCount();
            DWORD timeSinceLastClick = currentTime - lastLeftClickTime;
            lastLeftClickTime = currentTime;

            if (timeSinceLastClick <= leftDebounceTime) {
                leftClickRegistered = true;
                return 1;  // Block the left click event
            }
        }
        else if (wParam == WM_LBUTTONUP) {
            if (leftClickRegistered) {
                leftClickRegistered = false;
                return 1;  // Block the left release event
            }
        }
        else if (wParam == WM_RBUTTONDOWN) {
            DWORD currentTime = GetTickCount();
            DWORD timeSinceLastClick = currentTime - lastRightClickTime;
            lastRightClickTime = currentTime;

            if (timeSinceLastClick <= rightDebounceTime) {
                rightClickRegistered = true;
                return 1;  // Block the right click event
            }
        }
        else if (wParam == WM_RBUTTONUP) {
            if (rightClickRegistered) {
                rightClickRegistered = false;
                return 1;  // Block the right release event
            }
        }
        else if (wParam == WM_MBUTTONDOWN) {
            DWORD currentTime = GetTickCount();
            DWORD timeSinceLastClick = currentTime - lastMiddleClickTime;
            lastMiddleClickTime = currentTime;

            if (timeSinceLastClick <= middleDebounceTime) {
                middleClickRegistered = true;
                return 1;  // Block the middle click event
            }
        }
        else if (wParam == WM_MBUTTONUP) {
            if (middleClickRegistered) {
                middleClickRegistered = false;
                return 1;  // Block the middle release event
            }
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}
