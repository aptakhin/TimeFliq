/* TimeFliq */
#pragma once

#define NOMINMAX
#include <windows.h>
#include <memory>
#include "timefliq.h"

const auto TF_WM_SHELL_ICON = WM_USER + 1;

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

class FiberW;

class Ctrl::Impl {
public:
	void init();
	void destroy();

	int run();

	void set_input_lock(bool lock);

	NOTIFYICONDATA notify_;

	std::unique_ptr<FiberW> fiber_;
};

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
