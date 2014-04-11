#pragma once
#include <boost/preprocessor/repetition/repeat.hpp>

#define PP_ZEROS(z, n, text) 0,
#define PP_ZERO_LIST(n) BOOST_PP_REPEAT(n, PP_ZEROS, )
