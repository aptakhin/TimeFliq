
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

void Ctrl::init() {
	impl_ = new Impl;
	impl_->init();
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
	wsprintf(buf, L"%d minutes left", (rest_timer_ + 59999) / 60000);
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
