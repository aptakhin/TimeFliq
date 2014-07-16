/* TimeFliq */
#pragma once

#include "win32common.h"
#include <windows.h>
#include "timefliq.h"

void* __cdecl operator new(unsigned bytes);
void __cdecl operator delete(void* ptr);

class Monitor::Impl {
public:
	Rect rect_;

	HWND wnd_ = nullptr;

	HMENU popup_menu_ = nullptr;

	const wchar_t* msg_ = nullptr;

	void init(Monitor* monitor, const Rect& rect);
	void destroy();
	void set_visible(bool show);
	void lock();
	void unlock();
	void set_message(const wchar_t* msg);
	void set_rect(const Rect& rect) { rect_ = rect; }
	const Rect& rect() const { return rect_; }
};

class Ctrl::Impl {
public:
	void init();
	void destroy();

	int run();

	void set_input_lock(bool lock);

	NOTIFYICONDATA notify_;
};

class Fiber::Impl {
public:
	Impl(void (*func)(void));

	static void on(Fiber& fiber);
	static void pause();

private:
	LPVOID fiber_;

	void (*func_)(void);
};

