
#include "win32.h"
#include <commctrl.h>
#include <algorithm>
#include "resource.h"

Ctrl gCtrl;

void Monitor::Impl::init(Monitor* monitor, const Rect& rect) {
	rect_ = rect;
	wnd_ = CreateWindow(app_title, app_title, WS_POPUP | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		rect_.x, rect_.y, rect_.w, rect_.h, NULL, NULL, NULL, NULL);

	long style = GetWindowLong(wnd_, GWL_STYLE);

	style &= ~WS_VISIBLE;
	style |= WS_EX_TOOLWINDOW;
	style &= ~WS_EX_APPWINDOW; 

	SetWindowLong(wnd_, GWL_STYLE, style);

	SetWindowLongPtr(wnd_, GWL_USERDATA, (LONG_PTR) monitor);
}

void Monitor::Impl::destroy() {

}

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

static HFONT font = NULL;

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
		CRANCH,
		ADD_5MIN,
		ABOUT,
	};
};

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	PAINTSTRUCT ps;
	HDC hdc;
	wchar_t rest[] = L"Rest";
	Monitor& monitor = *((Monitor*) GetWindowLong(hwnd, GWL_USERDATA));

	switch (message) {
	case WM_PAINT: {
		hdc = BeginPaint(hwnd, &ps);
		// Draw black rect
		SelectObject(hdc, GetStockObject(BLACK_BRUSH));
		RECT rect;
		SetRect(&rect, monitor.rect().x, monitor.rect().y, 
			monitor.rect().x + monitor.rect().w, monitor.rect().y + monitor.rect().h);
		Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		
		// Draw text
		SelectObject(hdc, font);
		SetTextAlign(hdc, TA_CENTER);
		SetTextColor(hdc, RGB(150, 150, 150));
		SetBkColor(hdc, RGB(0, 0, 0));
		DrawText(hdc, monitor.impl()->msg_? monitor.impl()->msg_ : rest, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

		EndPaint(hwnd, &ps);
	}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		PostQuitMessage(0);
		break;

	case TF_WM_SHELL_ICON: {
		switch (lparam) {
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU: {
			POINT mouse_pos;
			auto res = GetCursorPos(&mouse_pos);
			SetForegroundWindow(hwnd);
			HMENU popup_menu = monitor.impl()->popup_menu_;
			TrackPopupMenu(popup_menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, mouse_pos.x, mouse_pos.y, 0, hwnd, NULL);
		}
		}
	}
		break;

	case WM_COMMAND: {
		auto id = LOWORD(wparam);
		auto ev = HIWORD(wparam);

		switch (id) {
		case Menu::USUAL:
			gCtrl.set_mode(Mode::USUAL);
			CheckMenuItem(monitor.impl()->popup_menu_, Menu::CRANCH, MF_UNCHECKED);
			CheckMenuItem(monitor.impl()->popup_menu_, Menu::USUAL,  MF_CHECKED);
			
			break;

		case Menu::CRANCH:
			gCtrl.set_mode(Mode::CRANCH);
			CheckMenuItem(monitor.impl()->popup_menu_, Menu::USUAL,  MF_UNCHECKED);
			CheckMenuItem(monitor.impl()->popup_menu_, Menu::CRANCH, MF_CHECKED);
			EnableMenuItem(monitor.impl()->popup_menu_, Menu::ADD_5MIN, MF_GRAYED);
			break;

		case Menu::ADD_5MIN: {
			EnableMenuItem(monitor.impl()->popup_menu_, Menu::ADD_5MIN, MF_GRAYED);
			gCtrl.rest_timer_add(5 * MINUTE_MS);
			break;
		}
		}
	}
		break;

	default:
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	return 0;
}

BOOL CALLBACK monitors_proc(HMONITOR hmonitor, HDC hdc_monitor, LPRECT rc_monitor, LPARAM data) {
	Ctrl& ctrl = *((Ctrl*) data);
	if (ctrl.monitors_num >= length(ctrl.monitors))
		return FALSE; // No more place for monitor data
	Rect rect;
	rect.x = rc_monitor->left;
	rect.y = rc_monitor->top;
	rect.w = rc_monitor->right  - rc_monitor->left;
	rect.h = rc_monitor->bottom - rc_monitor->top;

	ctrl.monitors[ctrl.monitors_num].init(rect);
	++ctrl.monitors_num;
	return TRUE;      // Continue enumeration
}

struct NoInterrupt {};

void usual_mode_fiber(void* data, FiberW* fiber) {
	Ctrl& ctrl = *((Ctrl*) data);
	const wchar_t min2[] = L"2 minutes before lock";
	const wchar_t min1[] = L"1 minute before lock";
	const wchar_t rest[] = L"Rest";

	unsigned int work_ms = ctrl.conf.work_ms;
	unsigned int rest_ms = ctrl.conf.rest_ms;
	unsigned int notf_ms = ctrl.conf.notf_ms;
	unsigned int upd_ms  = ctrl.conf.upd_ms;

	enum State {
		MIN2,
		MIN1,
		WORK,
	};

	State state = WORK;

	while (true) {
		if (!ctrl.monitors_num)
			continue;

		ctrl.set_rest_timer(work_ms);

		while (ctrl.rest_timer()) {
			if (state == MIN2) {
				state = WORK;
				ctrl.monitors[0].set_message(min2);
				ctrl.monitors[0].lock();
				fiber->wait_ms(notf_ms);
				gCtrl.rest_timer_sub(notf_ms);
				ctrl.monitors[0].unlock();
			}
			else if (state == MIN1) {
				state = WORK;
				ctrl.monitors[0].set_message(min1);
				ctrl.monitors[0].lock();
				fiber->wait_ms(notf_ms);
				gCtrl.rest_timer_sub(notf_ms);
				ctrl.monitors[0].unlock();
			}
			else {
				state = WORK;
				auto wait = 0;
				if (2 * upd_ms <= ctrl.rest_timer() && ctrl.rest_timer() < 3 * upd_ms)
				{
					state = MIN2;
					wait = ctrl.rest_timer() - 2 * upd_ms;
				}
				else if (upd_ms <= ctrl.rest_timer() && ctrl.rest_timer() < 2 * upd_ms)
				{
					state = MIN1;
					wait = ctrl.rest_timer() - upd_ms;
				}
				else {
					wait = std::min(ctrl.rest_timer(), upd_ms);
				}
				gCtrl.rest_timer_sub(wait);
				fiber->wait_ms(wait);
			}
		}

		{
			ctrl.monitors[0].set_message(rest);
			ctrl.lock();
			fiber->wait_ms(rest_ms);
			ctrl.unlock();

			EnableMenuItem(ctrl.monitors[0].impl()->popup_menu_, Menu::ADD_5MIN, MF_ENABLED);
		}
	}
}

void Ctrl::Impl::init() {
	WNDCLASSEX wcex;
	wcex.cbSize        = sizeof WNDCLASSEX;
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

	if (!RegisterClassEx(&wcex)) { /* error */ }

	font = CreateFont(48, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Corbel"));

	EnumDisplayMonitors(NULL, NULL, monitors_proc, (LPARAM) &gCtrl);

	if (gCtrl.monitors_num < 1) { /* error */ }

	// Notify icon
	ZeroMemory(&notify_, sizeof notify_);
	notify_.cbSize = sizeof notify_;

	notify_.hIcon  = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1)); 
	notify_.hWnd   = gCtrl.monitors[0].impl()->wnd_; 
	notify_.uID    = WM_USER; 
	notify_.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

	wcscpy_s(notify_.szTip, TEXT(""));

	notify_.uCallbackMessage = TF_WM_SHELL_ICON;
	auto res = Shell_NotifyIcon(NIM_ADD, &notify_); 

	// Menu
	MENUITEMINFO menu_item;
	HMENU popup_menu = CreatePopupMenu();
	ZeroMemory(&menu_item, sizeof MENUITEMINFO);
	menu_item.cbSize = sizeof MENUITEMINFO;
	menu_item.fType  = MFT_RADIOCHECK;
	menu_item.fState = MF_CHECKED;
	menu_item.fMask  = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	menu_item.wID    = Menu::USUAL;
	menu_item.dwTypeData = L"&Usual";
	res = InsertMenuItem(popup_menu, 0, TRUE, &menu_item);

	menu_item.wID    = Menu::CRANCH;
	menu_item.fState = MF_UNCHECKED;
	menu_item.fMask  = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	menu_item.dwTypeData = L"&Cranch";
	res = InsertMenuItem(popup_menu, 1, TRUE, &menu_item);

	menu_item.wID    = Menu::ADD_5MIN;
	menu_item.fType  = MFT_STRING;
	menu_item.fState = MF_ENABLED;
	menu_item.fMask  = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	menu_item.dwTypeData = L"&Add 5 min";
	res = InsertMenuItem(popup_menu, 2, TRUE, &menu_item);

	gCtrl.monitors[0].impl()->popup_menu_ = popup_menu;

	ConvertThreadToFiber(GetCurrentThread());

	fiber_.reset(new FiberW(gCtrl.monitors[0].impl()->wnd_, usual_mode_fiber, &gCtrl));
}

void Ctrl::Impl::destroy() {
	auto res = Shell_NotifyIcon(NIM_DELETE, &notify_);
}

int Ctrl::Impl::run() {
	fiber_->resume();
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
}
