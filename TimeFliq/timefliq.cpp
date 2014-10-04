
#include "timefliq.h"
#ifdef _WIN32
#	include "win32.h"
#endif

void Monitor::init(const Rect& rect) { 
	impl_ = new Impl; 
	impl_->init(this, rect);
}

void Monitor::destroy() {
	impl_->destroy();
	delete impl_;
}

void Monitor::set_visible(bool show) {
	impl_->set_visible(show);
}

void Monitor::lock() {
	impl_->lock();
}

void Monitor::unlock() {
	impl_->unlock();
}

void Monitor::set_message(const wchar_t* msg) {
	impl_->set_message(msg);
}

const Rect& Monitor::rect() const {
	return impl_->rect();
}

void Ctrl::init(const char* cmd) {
	impl_ = new Impl;
	impl_->init();

	if (strstr(cmd, "--test_work")) {
		conf.work_ms = 10 * 1000;
		conf.rest_ms = 1 * 1000;
		conf.upd_ms  = 1000;
	} 
	else if (strstr(cmd, "--test_rest")) {
		conf.work_ms = 3 * 1000;
		conf.rest_ms = 7 * 1000;
		conf.upd_ms  = 1000;
	}
	else if (strstr(cmd, "--test_short")) {
		conf.work_ms = 5 * MINUTE_MS;
		conf.rest_ms = 5 * 1000;
		conf.upd_ms  = 1000;
	}
}

void Ctrl::destroy() {
	impl_->destroy();
	delete impl_;
	impl_ = nullptr;
}

int Ctrl::run() {
	return impl_->run();
}

void Ctrl::set_mode(Mode mode) {
	if (mode_ != mode) {
		mode_ = mode;
		if (mode == Mode::CRANCH)
			set_rest_timer(~0u);
		else
			set_rest_timer(conf.work_ms);
	}
}

void Ctrl::rest_timer_sub(unsigned int sub) {
	set_rest_timer(rest_timer_ - sub);
}

void Ctrl::rest_timer_add(unsigned int add) {
	set_rest_timer(rest_timer_ + add);
}

void Ctrl::set_rest_timer(unsigned int set) {
	rest_timer_ = set;

	wchar_t buf[256] = L"";
	if (~set) {
		unsigned int show_minutes = (rest_timer_ + MINUTE_MS - 1) / MINUTE_MS;
		if (show_minutes > 1)
			wsprintf(buf, L"%d minutes left", show_minutes);
		else if (show_minutes == 1)
			wsprintf(buf, L"One minute left");
		else
			wsprintf(buf, L"Rest in one minute");
	}
	else
		wsprintf(buf, L"It's cranch time!");

	wcscpy_s(impl_->notify_.szTip, buf);

	auto res = Shell_NotifyIcon(NIM_MODIFY, &impl_->notify_);
}

void Ctrl::lock() {
	for (size_t i = 0; i < monitors_num; ++i) {
		monitors[i].lock();
	}
	impl_->set_input_lock(true);
}

void Ctrl::unlock() {
	impl_->set_input_lock(false);
	for (size_t i = 0; i < monitors_num; ++i) {
		monitors[i].unlock();
	}
}
