#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <logger/interface.h>
namespace Base
{
	void exitVal(int returnVal) ATTRS(noreturn);
	void abort() ATTRS(noreturn);
}
#define BRANCH_H_PRINTF(str, ...) logErr(str, ## __VA_ARGS__)
#define BRANCH_H_EXIT() Base::abort()
#include <util/branch2.h>

#define doOrElse(Do, Else) { if(Do != OK) Else; }
#define doOrReturnVal(Do, Val) doOrElse(Do, return(Val))
#define doOrReturn(Do) { CallResult _r = Do; if(_r != OK) return(_r); }
#define doOrExit(Do) doOrElse(Do, Base::exitVal(0))
