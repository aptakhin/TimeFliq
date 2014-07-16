
#include "win32.h"
#include "resource.h"

void* __cdecl operator new(unsigned int bytes) {
	return HeapAlloc(GetProcessHeap(), 0, bytes);
	// We have a few allocations. Have to use stack
}

void __cdecl operator delete(void* ptr) {
	if (ptr)
		HeapFree(GetProcessHeap(), 0, ptr);
}

size_t wlen(const wchar_t* str) {
	size_t size = 0;
	while (*(str++) != L'\0') 
		size += 1;
	return size;
}

void ccopy(void* dst, const void* src, size_t size) {
	char* d = (char*) dst, *s = (char*) src;
	while (size--) {
		*d = *s;
	}
}

void wcopy(wchar_t* dst, const wchar_t* src, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		dst[i] = src[i];
	}
}

wchar_t* wcopy(const wchar_t* copy) {
	size_t size = wlen(copy);
	wchar_t* cpy = new wchar_t[size + 1];
	RtlSecureZeroMemory(cpy, (size + 1) * sizeof (wchar_t));
	wcopy(cpy, copy, size);
	cpy[size] = L'\0';
	return cpy;
}

void Monitor::Impl::init(Monitor* monitor, const Rect& rect) {
	rect_ = rect;
	wnd_ = CreateWindow(app_title, app_title, WS_POPUP | WS_EX_TOPMOST,
		rect_.x, rect_.y, rect_.w, rect_.h, NULL, NULL, NULL, NULL);

	SetWindowLongPtr(wnd_, GWL_USERDATA, (LONG_PTR) monitor);
}

void Monitor::Impl::destroy() {}

void Monitor::Impl::set_visible(bool show) {
	ShowWindow(wnd_, show? SW_SHOW : SW_HIDE);
}

void Monitor::Impl::lock() {
	set_visible(true);
	UpdateWindow(wnd_);
	SetFocus(wnd_);
}

void Monitor::Impl::unlock() {
	set_visible(false);
	SetFocus(wnd_);
}

void Monitor::Impl::set_message(const wchar_t* msg) {
	msg_ = msg;
}

static UINT_PTR gtimer = NULL;
static HFONT font = NULL;

void CALLBACK time2unlock(HWND, UINT msg, UINT_PTR event_id, DWORD time);

void CALLBACK time2lock(HWND, UINT msg, UINT_PTR event_id, DWORD time) {
	SetTimer(NULL, gtimer, 10000, time2unlock);
	ctrl.lock();
}

void CALLBACK time2unlock(HWND, UINT msg, UINT_PTR event_id, DWORD time) {
	ctrl.unlock();
	SetTimer(NULL, gtimer, 45 * 60000, time2lock);
}

class FiberW {
public:
	FiberW(HWND hwnd, void (*func)(void*, FiberW*), void* arg);

	~FiberW();

	void resume();

	void wait_ms(unsigned msec);

	void can_pause();

	void pause_softly();

private:
	static void __stdcall call_func(void* arg) {
		auto fiber = (FiberW*) arg;
		fiber->func_(fiber->func_arg_, fiber);
	}

	static void CALLBACK on_timer(HWND, UINT msg, UINT_PTR event_id, DWORD time) {
		auto fiber = (FiberW*) event_id;
		fiber->resume();
	}

private:
	bool to_pause_;
	void* fiber_;
	void* back_fiber_;
	void (*func_)(void*, FiberW*);
	void* func_arg_;
	HWND hwnd_;
	UINT_PTR timer_;
};

FiberW::FiberW(HWND hwnd, void (*func)(void*, FiberW*), void* arg)
:	to_pause_(false),
	hwnd_(hwnd),
	func_(func),
	func_arg_(arg),
	back_fiber_(nullptr) {
	fiber_ = CreateFiber(1024, FiberW::call_func, this);
}

FiberW::~FiberW() {

}

void FiberW::resume() {
	back_fiber_ = GetCurrentFiber();
	SwitchToFiber(fiber_);
}

void FiberW::wait_ms(unsigned msec) {
	timer_ = SetTimer(hwnd_, UINT_PTR(this), msec, FiberW::on_timer);
	SwitchToFiber(back_fiber_);
}

void FiberW::can_pause() {
	if (to_pause_) {
		KillTimer(hwnd_, timer_);
		to_pause_ = false;
	}
}

void FiberW::pause_softly() {
	to_pause_ = true;
}

struct Menu {
	enum {
		USUAL,
		CRANCH
	};
};

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
	wchar_t rest[] = L"Rest";
	Monitor& monitor = *((Monitor*) GetWindowLong(hWnd, GWL_USERDATA));

    switch (message) {
    case WM_PAINT: {
        hdc = BeginPaint(hWnd, &ps);
		SelectObject(hdc, GetStockObject(BLACK_BRUSH));
		
		RECT rect;
		SetRect(&rect, monitor.rect().x, monitor.rect().y, 
			monitor.rect().x + monitor.rect().w, monitor.rect().y + monitor.rect().h);
		Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		
        SelectObject(hdc, font);
		SetTextAlign(hdc, TA_CENTER);
		SetTextColor(hdc, RGB(150, 150, 150));
		SetBkColor(hdc, RGB(0, 0, 0));
		DrawText(hdc, monitor.impl()->msg_? monitor.impl()->msg_ : rest, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        EndPaint(hWnd, &ps);
	}
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

	case WM_KEYDOWN:
		PostQuitMessage(0);
        break;

	case WM_USER + 1: {
		switch (lParam) {
        case WM_RBUTTONDOWN:
        case WM_CONTEXTMENU: {
			POINT mouse_pos;
			auto res = GetCursorPos(&mouse_pos);
			HMENU popup_menu = monitor.impl()->popup_menu_;
            TrackPopupMenu(popup_menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, mouse_pos.x, mouse_pos.y, 0, hWnd, NULL);
		}
        }
	}
		break;

	case WM_COMMAND: {
		auto id = LOWORD(wParam);
		auto ev = HIWORD(wParam);

		switch (id) {
		case Menu::USUAL:
			ctrl.set_mode(Mode::USUAL);
			CheckMenuItem(monitor.impl()->popup_menu_, Menu::CRANCH, MF_UNCHECKED);
			CheckMenuItem(monitor.impl()->popup_menu_, Menu::USUAL,  MF_CHECKED);
			break;

		case Menu::CRANCH:
			ctrl.set_mode(Mode::CRANCH);
			CheckMenuItem(monitor.impl()->popup_menu_, Menu::USUAL,  MF_UNCHECKED);
			CheckMenuItem(monitor.impl()->popup_menu_, Menu::CRANCH, MF_CHECKED);
			break;
		}
		int l = 0;
	}
		break;

	case WM_SYSCOMMAND: {
		int l = 0;
	}
		break;

	case WM_CONTEXTMENU: {
		int l = 0;
	}
		break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

BOOL CALLBACK monitors_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	Ctrl& ctrl = *((Ctrl*) dwData);
	if (ctrl.monitors_num >= length(ctrl.monitors))
		return FALSE; // No more place for monitor data
	Rect rect;
	rect.x = lprcMonitor->left;
	rect.y = lprcMonitor->top;
	rect.w = lprcMonitor->right  - lprcMonitor->left;
	rect.h = lprcMonitor->bottom - lprcMonitor->top;

	ctrl.monitors[ctrl.monitors_num].init(rect);
	ctrl.monitors[ctrl.monitors_num].set_rect(rect);
	++ctrl.monitors_num;
	return TRUE;      // Continue enumeration
}

struct NoInterrupt {};

void usual_mode_fiber(void* data, FiberW* fiber) {
	Ctrl& ctrl = *((Ctrl*) data);
	const wchar_t min2[] = L"2 minutes before lock";
	const wchar_t min1[] = L"1 minute before lock";
	while (true) {
		if (!ctrl.monitors_num)
			continue;
		fiber->wait_ms(48 * 60000);

		{
			NoInterrupt nointer;
			ctrl.monitors[0].set_message(min2);
			ctrl.monitors[0].lock();
			fiber->wait_ms(1500);
			ctrl.monitors[0].unlock();
		}

		fiber->wait_ms(2 * 60000 - 3000);

		{
			NoInterrupt nointer;
			ctrl.monitors[0].set_message(min1);
			ctrl.monitors[0].lock();
			fiber->wait_ms(1500);
			ctrl.monitors[0].unlock();
		}
	}
}

void Ctrl::Impl::init() {
	WNDCLASSEX wcex;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = wnd_proc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = NULL;
    wcex.hIcon         = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = app_title;
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

    if (!RegisterClassEx(&wcex)) {
        //MessageBox(NULL, L"Call to RegisterClassEx failed!", app_title, NULL);
    }

	font = CreateFont(48, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Corbel"));

	EnumDisplayMonitors(NULL, NULL, monitors_proc, (LPARAM) &ctrl);

	if (ctrl.monitors_num < 1) { /* error */ }

	// Notify icon
	RtlSecureZeroMemory(&notify_, sizeof notify_);
	notify_.cbSize = sizeof notify_;
	notify_.hIcon  = LoadIcon(NULL, MAKEINTRESOURCE(IDI_ICON1)); 
	notify_.hWnd   = ctrl.monitors[0].impl()->wnd_; 
	notify_.uID    = WM_USER + 2; 
	notify_.uFlags = NIF_ICON | NIF_MESSAGE;

	notify_.uCallbackMessage = WM_USER + 1; 
	auto res = Shell_NotifyIcon(NIM_ADD, &notify_); 

	// Menu
	MENUITEMINFO menu_item;
	HMENU popup_menu = CreatePopupMenu();
	RtlSecureZeroMemory(&menu_item, sizeof MENUITEMINFO);
	menu_item.cbSize = sizeof MENUITEMINFO;
	menu_item.fType  = MFT_RADIOCHECK;
	menu_item.fState = MF_CHECKED;
	menu_item.fMask  = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	menu_item.wID    = Menu::USUAL;
	menu_item.dwTypeData = L"&Usual";
	res = InsertMenuItem(popup_menu, 0, TRUE, &menu_item);
                
	menu_item.wID    = Menu::CRANCH;
	menu_item.fState = MF_UNCHECKED;
	menu_item.dwTypeData = L"&Cranch";
	res = InsertMenuItem(popup_menu, 1, TRUE, &menu_item);
	ctrl.monitors[0].impl()->popup_menu_ = popup_menu;

	gtimer = SetTimer(NULL, NULL, 100, time2unlock);

	ConvertThreadToFiber(GetCurrentThread());

	auto fib = new FiberW(ctrl.monitors[0].impl()->wnd_, usual_mode_fiber, &ctrl);
	fib->resume();
}

void Ctrl::Impl::destroy() {
	auto res = Shell_NotifyIcon(NIM_DELETE, &notify_); 
}

int Ctrl::Impl::run() {
	MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	return 0;
}

void Ctrl::Impl::set_input_lock(bool lock) {
	BlockInput(lock? TRUE : FALSE);
	auto err = GetLastError();
	if (err) {
		//TRA
		//if (err == ERROR_ACCESS_DENIED) 
		//	MessageBox(NULL, L"Access denied", L"Access", 0);
	}
}

