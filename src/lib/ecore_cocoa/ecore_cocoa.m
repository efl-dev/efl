#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#import <Cocoa/Cocoa.h>
#import "ecore_cocoa_window.h"
#import "ecore_cocoa_app.h"

#include <Eina.h>

#include <Ecore.h>
#include <ecore_private.h>
#include <Ecore_Input.h>

#include "Ecore_Cocoa.h"
#include "Ecore_Cocoa_Keys.h"


EAPI int ECORE_COCOA_EVENT_GOT_FOCUS = 0;
EAPI int ECORE_COCOA_EVENT_LOST_FOCUS = 0;
EAPI int ECORE_COCOA_EVENT_RESIZE = 0;
EAPI int ECORE_COCOA_EVENT_EXPOSE = 0;

static int _ecore_cocoa_init_count = 0;

static int old_flags;

EAPI int
ecore_cocoa_init(void)
{
   if (++_ecore_cocoa_init_count != 1)
     return _ecore_cocoa_init_count;

   if (!ecore_init())
     return --_ecore_cocoa_init_count;

   if (!ecore_event_init())
     return --_ecore_cocoa_init_count;

   ECORE_COCOA_EVENT_GOT_FOCUS  = ecore_event_type_new();
   ECORE_COCOA_EVENT_LOST_FOCUS = ecore_event_type_new();
   ECORE_COCOA_EVENT_RESIZE     = ecore_event_type_new();
   ECORE_COCOA_EVENT_EXPOSE     = ecore_event_type_new();

   /* Init the Application handler */
   [Ecore_Cocoa_Application sharedApplication];
   [NSApp setDelegate:[Ecore_Cocoa_AppDelegate appDelegate]];

   /* Start events monitoring */
   [NSApp run];

   return _ecore_cocoa_init_count;
}

/**
 * Shuts down the Ecore_Cocoa library.
 * @return  @c The number of times the system has been initialised without
 *             being shut down.
 * @ingroup Ecore_Cocoa_Library_Group
 */
EAPI int
ecore_cocoa_shutdown(void)
{
   if (--_ecore_cocoa_init_count != 0)
     return _ecore_cocoa_init_count;

   ecore_event_shutdown();

   return _ecore_cocoa_init_count;
}

static unsigned int
_ecore_cocoa_event_modifiers(unsigned int mod)
{
   unsigned int modifiers = 0;

   if(mod & NSShiftKeyMask) modifiers |= ECORE_EVENT_MODIFIER_SHIFT;
   if(mod & NSControlKeyMask) modifiers |= ECORE_EVENT_MODIFIER_CTRL;
   if(mod & NSAlternateKeyMask) modifiers |= ECORE_EVENT_MODIFIER_ALT;
   if(mod & NSCommandKeyMask) modifiers |= ECORE_EVENT_MODIFIER_WIN;
   if(mod & NSNumericPadKeyMask) modifiers |= ECORE_EVENT_LOCK_NUM;

   printf("key modifiers: %d, %d\n", mod, modifiers);
   return modifiers;
}

static inline Eina_Bool
_nsevent_window_is_type_of(NSEvent *event, Class class)
{
   /* An NSPeriodic event has no window (undefined behaviour) */
   if ([event type] == NSPeriodic) return EINA_FALSE;
   return [[event window] isKindOfClass:class];
}

static inline Eina_Bool
_has_ecore_cocoa_window(NSEvent *event)
{
   return _nsevent_window_is_type_of(event, [EcoreCocoaWindow class]);
}

EAPI Eina_Bool
ecore_cocoa_feed_events(void *anEvent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(anEvent, EINA_FALSE);

   NSEvent *event = anEvent;
   unsigned int time = (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff);
   Eina_Bool pass = EINA_FALSE;
   static Eina_Bool compose = EINA_FALSE;
   static NSText *edit;

   switch ([event type])
   {
      case NSMouseMoved:
      case NSLeftMouseDragged:
      case NSRightMouseDragged:
      case NSOtherMouseDragged:
      {
         if (_has_ecore_cocoa_window(event))
           {
              Ecore_Event_Mouse_Move * ev = calloc(1, sizeof(Ecore_Event_Mouse_Move));
              if (!ev) return pass;

              EcoreCocoaWindow *window = (EcoreCocoaWindow *)[event window];
              NSView *view = [window contentView];
              NSPoint pt = [event locationInWindow];

              ev->x = pt.x;
              ev->y = [view frame].size.height - pt.y;
              ev->root.x = ev->x;
              ev->root.y = ev->y;
              ev->timestamp = time;
              ev->window = (Ecore_Window)window.ecore_window_data;
              ev->event_window = ev->window;
              ev->modifiers = 0; /* FIXME: keep modifier around. */

              ecore_event_add(ECORE_EVENT_MOUSE_MOVE, ev, NULL, NULL);
           }
         else
           {
              // We might want to handle cases such as events on the menubar.
              // If so, let's do it here.
           }
         pass = EINA_TRUE;
         break;
      }
      case NSLeftMouseDown:
      case NSRightMouseDown:
      case NSOtherMouseDown:
      {
         if (_has_ecore_cocoa_window(event))
           {
              Ecore_Event_Mouse_Button * ev = calloc(1, sizeof(Ecore_Event_Mouse_Button));
              if (!ev) return pass;

              EcoreCocoaWindow *window = (EcoreCocoaWindow *)[event window];
              NSView *view = [window contentView];
              NSPoint pt = [event locationInWindow];

              ev->x = pt.x;
              ev->y = [view frame].size.height - pt.y;
              ev->root.x = ev->x;
              ev->root.y = ev->y;
              ev->timestamp = time;
              switch ([event buttonNumber])
                {
                 case 0: ev->buttons = 1; break;
                 case 1: ev->buttons = 3; break;
                 case 2: ev->buttons = 2; break;
                 default: ev->buttons = 0; break;
                }
              ev->window = (Ecore_Window)window.ecore_window_data;
              ev->event_window = ev->window;

              if ([event clickCount] == 2)
                ev->double_click = 1;
              else
                ev->double_click = 0;

              if ([event clickCount] >= 3)
                ev->triple_click = 1;
              else
                ev->triple_click = 0;

              ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, ev, NULL, NULL);
           }
         else
           {
              // We might want to handle cases such as events on the menubar.
              // If so, let's do it here.
           }
         pass = EINA_TRUE;
         break;
      }
      case NSLeftMouseUp:
      case NSRightMouseUp:
      case NSOtherMouseUp:
      {
         Ecore_Event_Mouse_Button * ev = calloc(1, sizeof(Ecore_Event_Mouse_Button));
         if (!ev) return pass;

         if (_has_ecore_cocoa_window(event))
           {
              EcoreCocoaWindow *window = (EcoreCocoaWindow *)[event window];
              NSView *view = [window contentView];
              NSPoint pt = [event locationInWindow];

              ev->x = pt.x;
              ev->y = [view frame].size.height - pt.y;
              ev->root.x = ev->x;
              ev->root.y = ev->y;
              ev->timestamp = time;
              switch ([event buttonNumber])
                {
                 case 0: ev->buttons = 1; break;
                 case 1: ev->buttons = 3; break;
                 case 2: ev->buttons = 2; break;
                 default: ev->buttons = 0; break;
                }
              ev->window = (Ecore_Window)window.ecore_window_data;
              ev->event_window = ev->window;

              if ([event clickCount] == 2)
                ev->double_click = 1;
              else
                ev->double_click = 0;

              if ([event clickCount] >= 3)
                ev->triple_click = 1;
              else
                ev->triple_click = 0;

              ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_UP, ev, NULL, NULL);
           }
         else
           {
              // We might want to handle cases such as events on the menubar.
              // If so, let's do it here.
           }
         pass = EINA_TRUE;
         break;
      }
      case NSKeyDown:
      {
         Ecore_Event_Key *ev;
         unsigned int     i;
         EcoreCocoaWindow *window = (EcoreCocoaWindow *)[event window];
         NSString *keychar = [event characters];

         ev = calloc(1, sizeof (Ecore_Event_Key));
         if (!ev) return pass;
         ev->timestamp = time;
         ev->modifiers = _ecore_cocoa_event_modifiers([event modifierFlags]);

         if (compose)
           {
              [edit interpretKeyEvents:[NSArray arrayWithObject:event]];
              compose=EINA_FALSE;
           }

         if ([keychar length] > 0){
           for (i = 0; i < sizeof (keystable) / sizeof (struct _ecore_cocoa_keys_s); ++i)
           {
              if (keystable[i].code == [keychar characterAtIndex:0])
              {
                 printf("Key pressed : %s\n", keystable[i].name);
                 ev->keyname = keystable[i].name;
                 ev->key = keystable[i].name;
                 ev->string = keystable[i].compose;
                 ev->window = (Ecore_Window)window.ecore_window_data;
                 ev->event_window = ev->window;
                 ecore_event_add(ECORE_EVENT_KEY_DOWN, ev, NULL, NULL);
                 return pass;
              }
           }
         }else{
            compose=EINA_TRUE;
            edit = [[event window]  fieldEditor:YES forObject:nil];
            [edit interpretKeyEvents:[NSArray arrayWithObject:event]];
         }

         break;
      }
      case NSKeyUp:
      {
         Ecore_Event_Key *ev;
         unsigned int     i;
         EcoreCocoaWindow *window = (EcoreCocoaWindow *)[event window];
         NSString *keychar = [event characters];

         printf("Key Up\n");

         ev = calloc(1, sizeof (Ecore_Event_Key));
         if (!ev) return pass;
         ev->timestamp = time;
         ev->modifiers = _ecore_cocoa_event_modifiers([event modifierFlags]);

         if ([keychar length] > 0){
           for (i = 0; i < sizeof (keystable) / sizeof (struct _ecore_cocoa_keys_s); ++i)
           {
              if (keystable[i].code == tolower([keychar characterAtIndex:0]))
              {
                 ev->keyname = keystable[i].name;
                 ev->key = keystable[i].name;
                 ev->string = keystable[i].compose;
                 ev->window = (Ecore_Window)window.ecore_window_data;
                 ev->event_window = ev->window;
                 ecore_event_add(ECORE_EVENT_KEY_UP, ev, NULL, NULL);
                 return pass;
              }
           }
         }

         break;
      }
      case NSFlagsChanged:
      {
         int flags = [event modifierFlags];

         Ecore_Event_Key *evDown = NULL;
         Ecore_Event_Key *evUp = NULL;

         evDown = calloc(1, sizeof (Ecore_Event_Key));
         if (!evDown) return pass;

         evUp = calloc(1, sizeof (Ecore_Event_Key));
         if (!evUp)
           {
              free(evDown);
              return pass;
           }

         // Turn special key flags on
         if (flags & NSShiftKeyMask)
            evDown->keyname = "Shift_L";
         else if (flags & NSControlKeyMask)
            evDown->keyname = "Control_L";
         else if (flags & NSAlternateKeyMask)
            evDown->keyname = "Alt_L";
         else if (flags & NSCommandKeyMask)
            evDown->keyname = "Super_L";
         else if (flags & NSAlphaShiftKeyMask)
            evDown->keyname = "Caps_Lock";

         if (evDown->keyname)
         {
            evDown->timestamp = time;
            evDown->string = "";
            ecore_event_add(ECORE_EVENT_KEY_DOWN, evDown, NULL, NULL);
            old_flags = flags;
            break;
         }

         int changed_flags = flags ^ old_flags;

         // Turn special key flags off
         if (changed_flags & NSShiftKeyMask)
            evUp->keyname = "Shift_L";
         else if (changed_flags & NSControlKeyMask)
            evUp->keyname = "Control_L";
         else if (changed_flags & NSAlternateKeyMask)
            evUp->keyname = "Alt_L";
         else if (changed_flags & NSCommandKeyMask)
            evUp->keyname = "Super_L";
         else if (changed_flags & NSAlphaShiftKeyMask)
            evUp->keyname = "Caps_Lock";

         if (evUp->keyname)
         {
            evUp->timestamp = time;
            evUp->string = "";
            ecore_event_add(ECORE_EVENT_KEY_UP, evUp, NULL, NULL);
            old_flags = flags;
            break;
         }

         break;
      }
      case NSAppKitDefined:
      {
         if ([event subtype] == NSApplicationActivatedEventType)
            ecore_event_add(ECORE_COCOA_EVENT_GOT_FOCUS, NULL, NULL, NULL);
         else if ([event subtype] == NSApplicationDeactivatedEventType)
            ecore_event_add(ECORE_COCOA_EVENT_LOST_FOCUS, NULL, NULL, NULL);
         pass = EINA_TRUE; // pass along AppKit events, for window manager
         break;
      }
      case NSScrollWheel:
      {
         printf("Scroll Wheel\n");
         break;
      }
      default:
      {
         pass = EINA_TRUE;
         break;
      }
   }

   return pass;
}

EAPI void
ecore_cocoa_screen_size_get(Ecore_Cocoa_Screen *screen, int *w, int *h)
{
   NSSize pt =  [[[NSScreen screens] objectAtIndex:0] frame].size;

   if (w) *w = (int)pt.width;
   if (h) *h = (int)pt.height;
}

EAPI int
ecore_cocoa_titlebar_height_get(void)
{
   static int height = -1;

   if (height == -1)
     {
        NSRect frame = NSMakeRect(0, 0, 100, 100);
        NSRect contentRect;
        contentRect = [NSWindow contentRectForFrameRect:frame
                                              styleMask:NSTitledWindowMask];
        height = (frame.size.height - contentRect.size.height);
     }
   return height;
}

