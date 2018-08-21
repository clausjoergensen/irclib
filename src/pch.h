// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <locale>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
