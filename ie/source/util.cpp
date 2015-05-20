#include "stdafx.h"
#include "util.h"

#if 0
Logger::pointer logger(new Logger(Logger::ALL, L""));
#endif

Logger::pointer logger(new Logger(Logger::DBG, L"C:\\forge.log"));
