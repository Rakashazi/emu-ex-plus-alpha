#pragma once
#include <util/builtins.h>
#include <assert.h>

template <typename T>
class Delegate {};

#define TEMPLATE_LIST typename R
#define TEMPLATE_IMPL_LIST R ()
#define TEMPLATE_ARG_LIST
#define TEMPLATE_ARG_LIST2
#define TEMPLATE_ARG_LIST3
#define TEMPLATE_ARG_LIST4
#define TEMPLATE_ARG_LIST5
#define TEMPLATE_ARG_LIST6
#include "DelegateInclude.hh"

#define TEMPLATE_LIST typename R, typename ARG0
#define TEMPLATE_IMPL_LIST R (ARG0)
#define TEMPLATE_ARG_LIST , ARG0
#define TEMPLATE_ARG_LIST2 ARG0
#define TEMPLATE_ARG_LIST3 , ARG0 a0
#define TEMPLATE_ARG_LIST4 , a0
#define TEMPLATE_ARG_LIST5 ARG0 a0
#define TEMPLATE_ARG_LIST6 a0
#include "DelegateInclude.hh"

#define TEMPLATE_LIST typename R, typename ARG0, typename ARG1
#define TEMPLATE_IMPL_LIST R (ARG0, ARG1)
#define TEMPLATE_ARG_LIST , ARG0, ARG1
#define TEMPLATE_ARG_LIST2 ARG0, ARG1
#define TEMPLATE_ARG_LIST3 , ARG0 a0, ARG1 a1
#define TEMPLATE_ARG_LIST4 , a0, a1
#define TEMPLATE_ARG_LIST5 ARG0 a0, ARG1 a1
#define TEMPLATE_ARG_LIST6 a0, a1
#include "DelegateInclude.hh"

