/* TimeFliq */
#pragma once

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

struct Config {
	unsigned int work_ms = 50 * MINUTE_MS;
	unsigned int rest_ms = 10 * MINUTE_MS;
	unsigned int notf_ms = 1500;
	unsigned int upd_ms  = MINUTE_MS;
};

class Ctrl {
public:
	Monitor monitors[4];
	size_t monitors_num = 0;

	Config conf;

	void init(const char* cmd);
	void destroy();

	int run();

	void lock();
	void unlock();

	void set_mode(Mode mode);
	Mode mode() const { return mode_; }

	class Impl;

	Impl* impl() { return impl_; }

	unsigned int rest_timer() const { return rest_timer_; }
	void rest_timer_sub(unsigned int sub);
	void rest_timer_add(unsigned int add);
	void set_rest_timer(unsigned int set);

private:
	Impl* impl_ = nullptr;

	Mode mode_;

	unsigned int rest_timer_;
};

extern Ctrl gCtrl;

