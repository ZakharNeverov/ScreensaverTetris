#include <windows.h>
#include <dwmapi.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <random>
#include <commdlg.h>
#include <string>
#include <tchar.h>
#include "Resource.h"

#pragma comment(lib, "dwmapi.lib")

HINSTANCE hInstance;
POINT mousePosition;

struct Point {
    int x, y;
};

const int TILE_SIZE = 30; // Size of each tile in pixels
int WIDTH; // Width of the Tetris grid
int HEIGHT; // Height of the Tetris grid
int xPos; // Position of the current piece
int yPos;
int currentPieceType;
int nextPieceType;
std::vector<std::vector<int>> grid;
std::vector<Point> currentPiece;
std::vector<Point> nextPiece;
HBITMAP hBitmap; // Handle to the off-screen buffer

std::vector<std::vector<Point>> pieces = {
    {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, // I
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, // O
    {{0, 0}, {1, 0}, {2, 0}, {1, 1}}, // T
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}}, // S
    {{1, 0}, {2, 0}, {0, 1}, {1, 1}}, // Z
    {{0, 0}, {0, 1}, {1, 1}, {2, 1}}, // L
    {{2, 0}, {0, 1}, {1, 1}, {2, 1}}  // J
};

COLORREF pieceColors[] = {
    RGB(122, 235, 255), // Cyan I
    RGB(255, 249, 196), // White O
    RGB(188, 99, 255), // Purple T
    RGB(122, 255, 189), // Green S
    RGB(255, 122, 178), // Red Z
    RGB(111, 109, 252), // Blue J
    RGB(255, 180, 122)  // Orange L
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI RegisterDialogClasses(HANDLE hInst);
void InitializePiece();
void GenerateNextPiece();
bool CheckCollision(int dx, int dy);
void LockPiece();
void ClearLines();
void RotatePiece();
void UpdateGame();
void DrawGrid(HDC hdc);
void DrawPiece(HDC hdc, const std::vector<Point>& piece, int x, int y, COLORREF color);
void DrawRect(HDC hdc, int x, int y, int size, COLORREF color);
COLORREF DarkerColor(COLORREF color);
void SetSemiTransparentBackground(HWND hwnd, int alpha);
void SaveColorsToINI();
void LoadColorsFromINI();
void ChooseColor(HWND hDlg, int buttonID, COLORREF& color);

std::random_device rd;
std::mt19937 rng(rd());

void SetSemiTransparentBackground(HWND hwnd, int alpha) {
    SetLayeredWindowAttributes(hwnd, 0, (255 * alpha) / 100, LWA_ALPHA);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HDC hdcMem;
    static HBITMAP hbmOld;
    static RECT clientRect;

    switch (msg) {
    case WM_CREATE:
    {
        GetCursorPos(&mousePosition);
        SetTimer(hwnd, 1, 64, NULL);
        GenerateNextPiece();
        InitializePiece();
        SetSemiTransparentBackground(hwnd, 90);
        LoadColorsFromINI(); // Load colors from the INI file

        // Initialize double buffering
        HDC hdc = GetDC(hwnd);
        hdcMem = CreateCompatibleDC(hdc);
        GetClientRect(hwnd, &clientRect);
        hBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
        hbmOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
        ReleaseDC(hwnd, hdc);
        break;
    }
    case WM_TIMER:
        UpdateGame();
        // Invalidate only the client area to reduce unnecessary redraws
        InvalidateRect(hwnd, &clientRect, FALSE);
        break;
    case WM_MOUSEMOVE:
    {
        POINT newMousePosition;
        GetCursorPos(&newMousePosition);
        if ((abs(newMousePosition.x - mousePosition.x) > 5) || (abs(newMousePosition.y - mousePosition.y) > 5)) {
            PostQuitMessage(0);
        }
        break;
    }
    case WM_KEYDOWN:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Clear the background
        FillRect(hdcMem, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

        DrawGrid(hdcMem);
        DrawPiece(hdcMem, currentPiece, xPos, yPos, pieceColors[currentPieceType]);

        // Draw the next piece in the top-right corner
        DrawPiece(hdcMem, nextPiece, WIDTH - 5, 1, pieceColors[nextPieceType]);

        // Copy the off-screen buffer to the screen
        BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
    {
        KillTimer(hwnd, 1);
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        LoadColorsFromINI();
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BUTTON_I:
            ChooseColor(hDlg, IDC_BUTTON_I, pieceColors[0]);
            break;
        case IDC_BUTTON_O:
            ChooseColor(hDlg, IDC_BUTTON_O, pieceColors[1]);
            break;
        case IDC_BUTTON_T:
            ChooseColor(hDlg, IDC_BUTTON_T, pieceColors[2]);
            break;
        case IDC_BUTTON_S:
            ChooseColor(hDlg, IDC_BUTTON_S, pieceColors[3]);
            break;
        case IDC_BUTTON_Z:
            ChooseColor(hDlg, IDC_BUTTON_Z, pieceColors[4]);
            break;
        case IDC_BUTTON_J:
            ChooseColor(hDlg, IDC_BUTTON_J, pieceColors[5]);
            break;
        case IDC_BUTTON_L:
            ChooseColor(hDlg, IDC_BUTTON_L, pieceColors[6]);
            break;
        case IDOK:
            SaveColorsToINI();
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst) {
    return TRUE;
}

void InitializePiece() {
    currentPieceType = nextPieceType;
    currentPiece = nextPiece;
    GenerateNextPiece();
    xPos = rng() % (WIDTH - 4);
    yPos = 0;
}

void GenerateNextPiece() {
    std::uniform_int_distribution<int> dist(0, pieces.size() - 1);
    nextPieceType = dist(rng);
    nextPiece = pieces[nextPieceType];
}

bool CheckCollision(int dx, int dy) {
    for (const auto& p : currentPiece) {
        int newX = p.x + xPos + dx;
        int newY = p.y + yPos + dy;
        if (newX < 0 || newX >= WIDTH || newY >= HEIGHT || (newY >= 0 && grid[newY][newX])) {
            return true;
        }
    }
    return false;
}

void LockPiece() {
    for (const auto& p : currentPiece) {
        int newX = p.x + xPos;
        int newY = p.y + yPos;
        if (newY >= 0) {
            grid[newY][newX] = currentPieceType + 1;
        }
    }
    ClearLines();
    InitializePiece();
    if (CheckCollision(0, 0)) {
        for (auto& row : grid) {
            std::fill(row.begin(), row.end(), 0);
        }
    }
}

void ClearLines() {
    for (int y = 0; y < HEIGHT; ++y) {
        bool fullLine = true;
        for (int x = 0; x < WIDTH; ++x) {
            if (grid[y][x] == 0) {
                fullLine = false;
                break;
            }
        }
        if (fullLine) {
            grid.erase(grid.begin() + y);
            grid.insert(grid.begin(), std::vector<int>(WIDTH, 0));
        }
    }
}

void RotatePiece() {
    std::vector<Point> rotatedPiece;
    for (const auto& p : currentPiece) {
        rotatedPiece.push_back({ -p.y, p.x });
    }
    auto backupPiece = currentPiece;
    currentPiece = rotatedPiece;
    if (CheckCollision(0, 0)) {
        currentPiece = backupPiece;
    }
}

void UpdateGame() {
    if (!CheckCollision(0, 1)) {
        ++yPos;
        std::uniform_int_distribution<int> dist(1, 32);
        int cond = dist(rng);
        switch (cond) {
        case 1:
            if (xPos < WIDTH - 3) {
                xPos++;
            }
            break;
        case 2:
            if (xPos > 3) {
                xPos--;
            }
            break;
        case 3:
            RotatePiece();
            break;
        }
    }
    else {
        LockPiece();
    }
}

void DrawGrid(HDC hdc) {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if (grid[y][x]) {
                DrawRect(hdc, x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, pieceColors[grid[y][x] - 1]);
            }
        }
    }
}

void DrawPiece(HDC hdc, const std::vector<Point>& piece, int x, int y, COLORREF color) {
    for (const auto& p : piece) {
        DrawRect(hdc, (x + p.x) * TILE_SIZE, (y + p.y) * TILE_SIZE, TILE_SIZE, color);
    }
}

void DrawRect(HDC hdc, int x, int y, int size, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    COLORREF borderColor = DarkerColor(color);
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    SelectObject(hdc, brush);
    SelectObject(hdc, pen);
    Rectangle(hdc, x, y, x + size, y + size);
    DeleteObject(brush);
    DeleteObject(pen);
}

COLORREF DarkerColor(COLORREF color) {
    int r = GetRValue(color) * 0.75;
    int g = GetGValue(color) * 0.75;
    int b = GetBValue(color) * 0.75;
    return RGB(r, g, b);
}

void SaveColorsToINI() {
    WCHAR szIniFile[MAX_PATH];
    GetModuleFileName(NULL, szIniFile, MAX_PATH);
    WCHAR* p = wcsrchr(szIniFile, L'\\');
    if (p) *(p + 1) = 0;
    wcscat_s(szIniFile, L"screensaver.ini");

    for (int i = 0; i < 7; ++i) {
        WCHAR szColor[32];
        wsprintf(szColor, L"%d %d %d", GetRValue(pieceColors[i]), GetGValue(pieceColors[i]), GetBValue(pieceColors[i]));
        WCHAR szKey[32];
        wsprintf(szKey, L"Color%d", i);
        WritePrivateProfileString(L"Colors", szKey, szColor, szIniFile);
    }
}

void LoadColorsFromINI() {
    WCHAR szIniFile[MAX_PATH];
    GetModuleFileName(NULL, szIniFile, MAX_PATH);
    WCHAR* p = wcsrchr(szIniFile, L'\\');
    if (p) *(p + 1) = 0;
    wcscat_s(szIniFile, L"screensaver.ini");

    for (int i = 0; i < 7; ++i) {
        WCHAR szColor[32];
        WCHAR szKey[32];
        wsprintf(szKey, L"Color%d", i);
        GetPrivateProfileString(L"Colors", szKey, L"", szColor, 32, szIniFile);
        int r, g, b;
        if (swscanf_s(szColor, L"%d %d %d", &r, &g, &b) == 3) {
            pieceColors[i] = RGB(r, g, b);
        }
    }
}

void ChooseColor(HWND hDlg, int buttonID, COLORREF& color) {
    CHOOSECOLOR cc;
    static COLORREF acrCustClr[16];
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hDlg;
    cc.lpCustColors = (LPDWORD)acrCustClr;
    cc.rgbResult = color;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&cc) == TRUE) {
        color = cc.rgbResult;
        HWND hButton = GetDlgItem(hDlg, buttonID);
        HBRUSH hBrush = CreateSolidBrush(color);
        HDC hdc = GetDC(hButton);
        RECT rect;
        GetClientRect(hButton, &rect);
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);
        ReleaseDC(hButton, hdc);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    srand(static_cast<unsigned>(time(0)));
    WNDCLASSEX wc;
    HWND hwnd;
    MSG msg;

    WIDTH = GetSystemMetrics(SM_CXSCREEN) / TILE_SIZE;
    HEIGHT = GetSystemMetrics(SM_CYSCREEN) / TILE_SIZE;
    grid.resize(HEIGHT, std::vector<int>(WIDTH, 0));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"MyScreensaver";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"MyScreensaver",
        L"My Screensaver",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    std::string cmdLine(lpCmdLine);
    std::transform(cmdLine.begin(), cmdLine.end(), cmdLine.begin(), ::tolower);

    if (cmdLine.find("/c") != std::wstring::npos || cmdLine.empty()) {
        DialogBox(hInstance, MAKEINTRESOURCE(DLG_SCRNSAVECONFIGURE), hwnd, ScreenSaverConfigureDialog);
    }
    else if (cmdLine.find("/s") != std::wstring::npos) {
        LoadColorsFromINI();
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
    else if (cmdLine.find("/p") != std::wstring::npos) {
        // Handle preview mode if necessary
        // For now, we just shut down as a placeholder
        PostQuitMessage(0);
    }
    else {
        MessageBox(NULL, L"Invalid command line argument!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
