#pragma once

using int8_t = signed char;
using uint8_t = unsigned char;

using int08_t = signed char;
using int16_t = signed short;
using int32_t = signed int;
using uint08_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;

#ifdef __EMSCRIPTEN__
	using size_t = unsigned long;
#else
	using int64_t = signed long long int;
	using uint64_t = unsigned long long;
	using size_t = unsigned long long;
#endif
