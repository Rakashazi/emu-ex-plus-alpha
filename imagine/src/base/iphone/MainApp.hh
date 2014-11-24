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
#include <imagine/base/Window.hh>

#ifdef IPHONE_MSG_COMPOSE
#import <MessageUI/MessageUI.h>
#endif

#ifdef IPHONE_GAMEKIT
#import <GameKit/GameKit.h>
#endif

#define IPHONE_VKEYBOARD

@interface MainUIApp : UIApplication {}
@end

@interface MainApp : NSObject <UIApplicationDelegate
#ifdef IPHONE_VKEYBOARD
, UITextFieldDelegate
//, UITextViewDelegate
#endif
#ifdef IPHONE_IMG_PICKER
, UINavigationControllerDelegate, UIImagePickerControllerDelegate
#endif
#ifdef IPHONE_MSG_COMPOSE
, MFMailComposeViewControllerDelegate
#endif
#ifdef IPHONE_GAMEKIT
, GKSessionDelegate, GKPeerPickerControllerDelegate
#endif
> {}
@end

namespace Base
{
void updateWindowSizeAndContentRect(Base::Window &win, int width, int height, UIApplication *sharedApp);
}
