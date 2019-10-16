
#ifndef _bg2e_wnd_keyboard_event_hpp_
#define _bg2e_wnd_keyboard_event_hpp_

#include <bg2e/wnd/glfw_defines.hpp>

namespace bg2e {
    namespace wnd {
        
        class KeyboardEvent {
        public:
            enum KeyCode {
                KeyUNKNOWN      =   -1,
                KeySPACE        =   32,
                KeyAPOSTROPHE   =   39,
                KeyCOMMA        =   44,
                KeyMINUS        =   45,
                KeyPERIOD       =   46,
                KeySLASH        =   47,
                Key0            =   48,
                Key1            =   49,
                Key2            =   50,
                Key3            =   51,
                Key4            =   52,
                Key5            =   53,
                Key6            =   54,
                Key7            =   55,
                Key8            =   56,
                Key9            =   57,
                KeySEMICOLON    =   59,
                KeyEQUAL        =   61,
                KeyA            =   65,
                KeyB            =   66,
                KeyC            =   67,
                KeyD            =   68,
                KeyE            =   69,
                KeyF            =   70,
                KeyG            =   71,
                KeyH            =   72,
                KeyI            =   73,
                KeyJ            =   74,
                KeyK            =   75,
                KeyL            =   76,
                KeyM            =   77,
                KeyN            =   78,
                KeyO            =   79,
                KeyP            =   80,
                KeyQ            =   81,
                KeyR            =   82,
                KeyS            =   83,
                KeyT            =   84,
                KeyU            =   85,
                KeyV            =   86,
                KeyW            =   87,
                KeyX            =   88,
                KeyY            =   89,
                KeyZ            =   90,
                KeyLEFT_BRACKET =   91,
                KeyBACKSLASH    =   92,
                KeyRIGHT_BRACKET =   93,
                KeyGRAVE_ACCENT  =   96,
                KeyWORLD_1      =   161,
                KeyWORLD_2      =   162,
                KeyESCAPE       =   256,
                KeyENTER        =   257,
                KeyTAB          =   258,
                KeyBACKSPACE    =   259,
                KeyINSERT       =   260,
                KeyDELETE       =   261,
                KeyRIGHT        =   262,
                KeyLEFT         =   263,
                KeyDOWN         =   264,
                KeyUP           =   265,
                KeyPAGE_UP      =   266,
                KeyPAGE_DOWN    =   267,
                KeyHOME         =   268,
                KeyEND          =   269,
                KeyCAPS_LOCK    =   280,
                KeySCROLL_LOCK  =   281,
                KeyNUM_LOCK     =   282,
                KeyPRINT_SCREEN =   283,
                KeyPAUSE        =   284,
                KeyF1           =   290,
                KeyF2           =   291,
                KeyF3           =   292,
                KeyF4           =   293,
                KeyF5           =   294,
                KeyF6           =   295,
                KeyF7           =   296,
                KeyF8           =   297,
                KeyF9           =   298,
                KeyF10          =   299,
                KeyF11          =   300,
                KeyF12          =   301,
                KeyF13          =   302,
                KeyF14          =   303,
                KeyF15          =   304,
                KeyF16          =   305,
                KeyF17          =   306,
                KeyF18          =   307,
                KeyF19          =   308,
                KeyF20          =   309,
                KeyF21          =   310,
                KeyF22          =   311,
                KeyF23          =   312,
                KeyF24          =   313,
                KeyF25          =   314,
                KeyKP_0         =   320,
                KeyKP_1         =   321,
                KeyKP_2         =   322,
                KeyKP_3         =   323,
                KeyKP_4         =   324,
                KeyKP_5         =   325,
                KeyKP_6         =   326,
                KeyKP_7         =   327,
                KeyKP_8         =   328,
                KeyKP_9         =   329,
                KeyKP_DECIMAL   =   330,
                KeyKP_DIVIDE    =   331,
                KeyKP_MULTIPLY  =   332,
                KeyKP_SUBTRACT  =   333,
                KeyKP_ADD       =   334,
                KeyKP_ENTER     =   335,
                KeyKP_EQUAL     =   336,
                KeyLEFT_SHIFT   =   340,
                KeyLEFT_CONTROL =   341,
                KeyLEFT_ALT     =   342,
                KeyLEFT_SUPER   =   343,
                KeyRIGHT_SHIFT  =   344,
                KeyRIGHT_CONTROL =   345,
                KeyRIGHT_ALT    =   346,
                KeyRIGHT_SUPER  =   347,
                KeyMENU         =   348
            };
            
            KeyboardEvent(int key, int scancode, int mods) {
                _shift = mods & GLFW_MOD_SHIFT;
                _alt = mods & GLFW_MOD_ALT;
                _control = mods & GLFW_MOD_CONTROL;
                _capsLock = mods & GLFW_MOD_CAPS_LOCK;
                _numLock = mods & GLFW_MOD_NUM_LOCK;
                _key = static_cast<KeyCode>(key);
            }
            
            inline bool shiftEnabled() const { return _shift; }
            inline bool controlEnabled() const { return _control; }
            inline bool altEnabled() const { return _alt; }
            inline bool capsLockEnabled() const { return _capsLock; }
            inline bool numLockEnabled() const { return _numLock; }
            inline KeyCode keyCode() const { return _key; }

        protected:
            bool _shift;
            bool _control;
            bool _alt;
            bool _capsLock;
            bool _numLock;

            KeyCode _key;
        };
    
    }
}

#endif /* keyboard_event_h */
