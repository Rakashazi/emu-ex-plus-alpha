#pragma once

// enumeration + string array in one step
#include <boost/preprocessor/list/enum.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/list/transform.hpp>
#define ENUM_LISTS(name, elems, nameList, enumName, arrModfiers) typedef enum { BOOST_PP_LIST_ENUM(elems) } enumName; arrModfiers const char *name[] = { BOOST_PP_LIST_ENUM(nameList) };

#define TOSTRING(d, data, elem) #elem

#define ENUM_LIST(name, elems, enumName, arrModfiers) ENUM_LISTS(name, elems, BOOST_PP_LIST_TRANSFORM(TOSTRING, _, elems), enumName, arrModfiers)

#define ENUM(name, size, elems) ENUM_LIST(name, BOOST_PP_TUPLE_TO_LIST(size, elems), name##Enum, )
#define ENUM_EX(name, size, elems, enumName, arrModfiers) ENUM_LIST(name, BOOST_PP_TUPLE_TO_LIST(size, elems))
