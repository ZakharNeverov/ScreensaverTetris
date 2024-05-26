#include <windows.h>
#include <dwmapi.h>
#include <vector>
#include <ctime>
#include <cstdlib>

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
std::vector<std::vector<int>> grid;
std::vector<Point> currentPiece;
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
	RGB(0, 255, 255), // Cyan I
	RGB(255, 255, 0), // Yellow O
	RGB(128, 0, 128), // Purple T
	RGB(0, 255, 0), // Green S
	RGB(255, 0, 0), // Red Z
	RGB(0, 0, 255), // Blue J
	RGB(255, 165, 0) // Orange L
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitializePiece();
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
		SetTimer(hwnd, 1, 100, NULL);
		InitializePiece();
		SetSemiTransparentBackground(hwnd, 90);

		// Initialize double buffering
		HDC hdc = GetDC(hwnd);
		hdcMem = CreateCompatibleDC(hdc);
		GetClientRect(hwnd, &clientRect);
		hBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
		hbmOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
		ReleaseDC(hwnd, hdc);
	}
	break;
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
	}
	break;
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

		// Copy the off-screen buffer to the screen
		BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

		EndPaint(hwnd, &ps);
	}
	break;
	case WM_DESTROY:
	{
		KillTimer(hwnd, 1);
		SelectObject(hdcMem, hbmOld);
		DeleteObject(hBitmap);
		DeleteDC(hdcMem);
		PostQuitMessage(0);
	}
	break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void InitializePiece() {
	currentPieceType = rand() % pieces.size();
	currentPiece = pieces[currentPieceType];
	xPos = rand() % (WIDTH - 4);
	yPos = 0;
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

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}