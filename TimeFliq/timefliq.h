/* TimeFliq */
#pragma once

#include "win32common.h"
static wchar_t app_title[] = L"TimeFliq";

template <class T, size_t N>
size_t length(T (&arr)[N]) {
	return N;
}

struct Rect {
	int x = 0, y = 0, w = 0, h = 0;
};

class Monitor {
public:
	void init(const Rect& rect);
	void destroy();
	void set_visible(bool show);
	void lock();
	void unlock();
	void set_message(const wchar_t* msg);

	const Rect& rect() const;
	void set_rect(const Rect& rect);

	class Impl;
	Impl* impl() { return impl_; }

private:
	Impl* impl_ = nullptr;
};

enum class Mode {
	USUAL,
	CRANCH
};

class Ctrl {
public:
	Monitor monitors[4];
	size_t monitors_num = 0;

	void init();
	void destroy();

	int run();

	void lock();
	void unlock();

	void set_mode(Mode mode);

	class Impl;

	Impl* impl() { return impl_; }

private:
	Impl* impl_ = nullptr;
};

namespace {
	Ctrl ctrl;
}

class Fiber {
public:
	Fiber(void (*func)(void));

	static void on(Fiber& fiber);
	static void pause();

	class Impl;
	Impl* impl() { return impl_; }

private:
	Impl* impl_ = nullptr;

protected:
	void on();

	void call_func();

	//static void __stdcall FiberProc(LPVOID);

protected:
	//LPVOID fiber_;

	void (*func_)(void);
};

