#define Fixed MacTypes_Fixed
#define Rect MacTypes_Rect
#import <UIKit/UIKit.h>
#undef Fixed
#undef Rect

#include <config.h>

#ifdef GREYSTRIPE
#import <GSAdEngine.h>
#endif

#ifdef IPHONE_MSG_COMPOSE
#import <MessageUI/MessageUI.h>
#endif

#ifdef IPHONE_GAMEKIT
#import <GameKit/GameKit.h>
#endif

#define IPHONE_VKEYBOARD

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
#ifdef GREYSTRIPE
, GreystripeDelegate
#endif
>
{

}

@end
