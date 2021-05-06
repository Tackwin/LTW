#pragma once

namespace xstd {
	template<typename T> struct Remove_Reference_      { using type = T; };
	template<typename T> struct Remove_Reference_<T&>  { using type = T; };
	template<typename T> struct Remove_Reference_<T&&> { using type = T; };
	template<typename T> using remove_reference_t = typename Remove_Reference_<T>::type;
};