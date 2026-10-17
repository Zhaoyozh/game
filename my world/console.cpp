// ============================================================================
//  console.cpp — 控制台 / ANSI / 光标 / 固定窗口 / 鼠标坐标 / 输入模式
// ============================================================================
#include "common.h"

// ============================================================
//  ANSI + 初始输入模式
// ============================================================
void enableAnsi() {
	// 只处理输出：开 ANSI
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hOut, &mode);
	SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	
	// 输入模式由 enableConsoleInput() 单独控制
	enableConsoleInput(true);
}

void hideCursor(bool hide) {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	GetConsoleCursorInfo(h, &info);
	info.bVisible = hide ? FALSE : TRUE;
	SetConsoleCursorInfo(h, &info);
}

void clearScreen() { cout << "\x1b[2J\x1b[H"; cout.flush(); }

// ============================================================
//  系统鼠标显示 / 隐藏（双保险：SetCursor + ShowCursor）
// ============================================================
void syncSystemCursor(bool wantShow) {
	if (wantShow) {
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		while (ShowCursor(TRUE) < 0) {}
	} else {
		SetCursor(NULL);
		while (ShowCursor(FALSE) >= 0) {}
	}
}

// ============================================================
//  开关控制台输入模式
//  enable=true  ：允许 cin 输入（登录/菜单用）
//  enable=false ：禁用快速编辑、鼠标输入（游戏用）
// ============================================================
void enableConsoleInput(bool enable) {
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hIn, &mode);
	
	if (enable) {
		// 恢复标准控制台输入
		mode |= ENABLE_PROCESSED_INPUT;
		mode |= ENABLE_LINE_INPUT;
		mode |= ENABLE_ECHO_INPUT;
		mode &= ~ENABLE_QUICK_EDIT_MODE;
		mode &= ~ENABLE_MOUSE_INPUT;
	} else {
		// 游戏模式：关掉快速编辑和鼠标输入
		mode &= ~ENABLE_QUICK_EDIT_MODE;
		mode &= ~ENABLE_MOUSE_INPUT;
	}
	SetConsoleMode(hIn, mode);
}

// ============================================================
//  字体
// ============================================================
static bool setConsoleFont(const wchar_t* face, short fontH) {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX cfi;
	memset(&cfi, 0, sizeof(cfi));
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = 0;
	cfi.dwFontSize.Y = fontH;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcsncpy(cfi.FaceName, face, LF_FACESIZE - 1);
	cfi.FaceName[LF_FACESIZE - 1] = 0;
	
	// 第二个参数 TRUE：同时改窗口标题栏的字体
	return SetCurrentConsoleFontEx(h, TRUE, &cfi) != 0;
}
// ============================================================
//  固定窗口：大小铺满屏幕，位置钉在 (0,0)
// ============================================================
void setupFixedConsole() {
	SetProcessDPIAware();
	    // 尝试多种中文字体，直到成功
	const wchar_t* fontCandidates[] = {
		L"Consolas",       // Win10+ 会自动 fallback 到中文
		L"SimSun",         // 新宋体（经典）
		L"NSimSun",        // 新宋体变体
		L"Microsoft YaHei", // 微软雅黑
		L"MS Gothic",      // 日文
	};
	bool fontOK = false;
	for (auto f : fontCandidates) {
		if (setConsoleFont(f, 14)) {
			// 验证：拿当前字体，看名字是否真的变了
			CONSOLE_FONT_INFOEX cur;
			memset(&cur, 0, sizeof(cur));
			cur.cbSize = sizeof(cur);
			if (GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &cur)) {
				if (wcscmp(cur.FaceName, f) == 0) {
					fontOK = true;
					break;
				}
			}
		}
	}
	Sleep(200);
	
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	HWND hwnd = GetConsoleWindow();
	
	CONSOLE_FONT_INFO cfi;
	GetCurrentConsoleFont(h, FALSE, &cfi);
	COORD fs = GetConsoleFontSize(h, cfi.nFont);
	if (fs.X <= 0) fs.X = 8;
	if (fs.Y <= 0) fs.Y = 16;
	
	int screenW = GetSystemMetrics(SM_CXSCREEN);
	int screenH = GetSystemMetrics(SM_CYSCREEN);
	
	int cols = (screenW - 20) / fs.X;
	int rows = (screenH - 60) / fs.Y;
	if (cols < 80)  cols = 80;
	if (cols > 300) cols = 300;
	if (rows < 25)  rows = 25;
	if (rows > 120) rows = 120;
	
	COORD buf = {(SHORT)(cols + 2), (SHORT)(rows + 20)};
	SetConsoleScreenBufferSize(h, buf);
	
	SMALL_RECT sr0 = {0, 0, 1, 1};
	SetConsoleWindowInfo(h, TRUE, &sr0);
	SMALL_RECT sr = {0, 0, (SHORT)(cols - 1), (SHORT)(rows - 1)};
	SetConsoleWindowInfo(h, TRUE, &sr);
	
	RECT r = {0, 0, cols * fs.X, rows * fs.Y};
	AdjustWindowRect(&r, (DWORD)GetWindowLong(hwnd, GWL_STYLE), FALSE);
	int ww = r.right - r.left;
	int wh = r.bottom - r.top;
	
	for (int i = 0; i < 8; ++i) {
		ShowWindow(hwnd, SW_RESTORE);
		SetWindowPos(hwnd, HWND_TOP, 0, 0, ww, wh, SWP_NOACTIVATE);
		Sleep(30);
	}
	
	SetConsoleScreenBufferSize(h, buf);
	SetConsoleWindowInfo(h, TRUE, &sr);
	SetWindowPos(hwnd, HWND_TOP, 0, 0, ww, wh, SWP_FRAMECHANGED);
	Sleep(50);
}

int getConsoleVisibleWidth() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) return 80;
	return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

int getConsoleVisibleHeight() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) return 25;
	return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void gotoScreenTopLeft() {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(h, &csbi);
	COORD pos = {csbi.srWindow.Left, csbi.srWindow.Top};
	SetConsoleCursorPosition(h, pos);
}

// ============================================================
//  鼠标字符坐标
// ============================================================
void getMouseChar(int& mx, int& my) {
	HWND hwnd = GetConsoleWindow();
	
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hwnd, &pt);
	
	RECT cr;
	GetClientRect(hwnd, &cr);
	int cw = cr.right - cr.left;
	int ch = cr.bottom - cr.top;
	if (cw <= 0 || ch <= 0) { mx = 0; my = 0; return; }
	
	int visW = getConsoleVisibleWidth();
	int visH = getConsoleVisibleHeight();
	if (visW <= 0) visW = 80;
	if (visH <= 0) visH = 25;
	
	int charW = cw / visW;
	int charH = ch / visH;
	if (charW <= 0) charW = 8;
	if (charH <= 0) charH = 16;
	
	mx = pt.x / charW;
	my = pt.y / charH;
	
	if (mx < 0) mx = 0;
	if (my < 0) my = 0;
}
