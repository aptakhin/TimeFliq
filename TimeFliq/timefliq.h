/* TimeFliq */
#pragma once

#include <atomic>
static wchar_t app_title[] = L"TimeFliq";

const static unsigned int MINUTE_MS = 60000;

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
	//void set_rect(const Rect& rect);

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

	unsigned int time2rest_;

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

extern Ctrl gCtrl;

