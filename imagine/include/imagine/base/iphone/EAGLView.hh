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
#include <imagine/engine-globals.h>
#include <imagine/base/iphone/config.h>

#ifdef CONFIG_BASE_IOS_GLKIT
#import <GLKit/GLKView.h>
using EAGLViewSuper = GLKView;
#else
using EAGLViewSuper = UIView;
#endif

@interface EAGLView : EAGLViewSuper
#if defined(CONFIG_BASE_IOS_KEY_INPUT) || defined(CONFIG_INPUT_ICADE)
<UIKeyInput>
#endif
{
#ifndef CONFIG_BASE_IOS_GLKIT
@private
	GLint backingWidth;
	GLint backingHeight;
	GLuint viewFramebuffer;
	GLuint depthRenderbuffer;
#endif
	GLuint viewRenderbuffer;
}

- (void)bindDrawable;
- (void)deleteDrawable;

#ifndef CONFIG_BASE_IOS_GLKIT
- (id)initWithFrame:(CGRect)frame context:(EAGLContext *)context;
- (void)setDrawableColorFormat:(NSString * const)format;
@property(nonatomic, strong) EAGLContext *context;
#endif

@end
