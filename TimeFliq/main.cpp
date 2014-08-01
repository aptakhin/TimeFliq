 
#include "timefliq.h"
#ifdef _WIN32
#	include "win32.h"
#endif

#ifdef _WIN32
int WinMainCRTStartup() {
#else
#	error "Need main implemented!"
#endif
	ctrl.init();
	int result = ctrl.run();
	ctrl.destroy();
	return result;
}