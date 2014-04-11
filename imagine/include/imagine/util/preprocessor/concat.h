#pragma once

#include <boost/preprocessor/cat.hpp>
#define PP_concat(a, b) BOOST_PP_CAT(a, b)
#define PP_concatMid(a, b, c) BOOST_PP_CAT(BOOST_PP_CAT(a, b), c)
