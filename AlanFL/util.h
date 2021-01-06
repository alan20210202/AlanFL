#pragma once

/*
 * Utilities for debugging use!
 */

#include <string>
#include <sstream>
#include <codecvt>

namespace alanfl {
	/*
	 * Because std::wstring is used for better support to Unicode characters,
	 * we cannot sometimes directly use other library functions, which usually 
	 * accepts std::string as arguments
	 */

	template <typename T>
	std::string to_str(const T x) {
		std::stringstream s;
		s << x;
		return s.str();
	}

	template <typename T>
	std::wstring to_wstr(const T x) {
		std::wstringstream s;
		s << x; 
		return s.str();
	}

	template <>
	inline std::wstring to_wstr(const std::string x) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> cvt;
		return cvt.from_bytes(x);
	}

	template <>
	inline std::wstring to_wstr(const char *x) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> cvt;
		return cvt.from_bytes(x);
	}

	template <>
	inline std::string to_str(const std::wstring x) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> cvt;
		return cvt.to_bytes(x);
	}

	/*
	 * This is used when it is 'clear' that it is unreachable
	 */
	inline void unreachable(const std::string &message) { throw std::logic_error("unreachable code: " + message); }

	/*
	 * This is used when the code is still left unimplemented
	 */
	inline void unimplemented(const std::string &message) { throw std::logic_error("unimplemented: " + message); }
}
