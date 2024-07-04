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

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#include <imagine/config/defs.hh>

@interface EAGLView : UIView {}

@property(readonly) GLuint colorRenderbuffer;
@property(readonly) GLuint depthRenderbuffer;

- (id)initWithFrame:(CGRect)frame;
- (void)setDrawableColorFormat:(NSString * const)format;
- (void)bindDrawable;
- (void)deleteDrawable;

@end
