
#include "timefliq.h"
#ifdef _WIN32
#	include "win32.h"
#else
#	error "Need something implemented!"
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
