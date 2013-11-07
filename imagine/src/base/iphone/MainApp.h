#import <UIKit/UIKit.h>

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
