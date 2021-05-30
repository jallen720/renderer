static void map_keys(Platform *platform) {
    // Alpha-numeric Keys
    platform->key_map[(s32)InputKey::NUM_0] = 0x30;
    platform->key_map[(s32)InputKey::NUM_1] = 0x31;
    platform->key_map[(s32)InputKey::NUM_2] = 0x32;
    platform->key_map[(s32)InputKey::NUM_3] = 0x33;
    platform->key_map[(s32)InputKey::NUM_4] = 0x34;
    platform->key_map[(s32)InputKey::NUM_5] = 0x35;
    platform->key_map[(s32)InputKey::NUM_6] = 0x36;
    platform->key_map[(s32)InputKey::NUM_7] = 0x37;
    platform->key_map[(s32)InputKey::NUM_8] = 0x38;
    platform->key_map[(s32)InputKey::NUM_9] = 0x39;
    platform->key_map[(s32)InputKey::A]     = 0x41;
    platform->key_map[(s32)InputKey::B]     = 0x42;
    platform->key_map[(s32)InputKey::C]     = 0x43;
    platform->key_map[(s32)InputKey::D]     = 0x44;
    platform->key_map[(s32)InputKey::E]     = 0x45;
    platform->key_map[(s32)InputKey::F]     = 0x46;
    platform->key_map[(s32)InputKey::G]     = 0x47;
    platform->key_map[(s32)InputKey::H]     = 0x48;
    platform->key_map[(s32)InputKey::I]     = 0x49;
    platform->key_map[(s32)InputKey::J]     = 0x4A;
    platform->key_map[(s32)InputKey::K]     = 0x4B;
    platform->key_map[(s32)InputKey::L]     = 0x4C;
    platform->key_map[(s32)InputKey::M]     = 0x4D;
    platform->key_map[(s32)InputKey::N]     = 0x4E;
    platform->key_map[(s32)InputKey::O]     = 0x4F;
    platform->key_map[(s32)InputKey::P]     = 0x50;
    platform->key_map[(s32)InputKey::Q]     = 0x51;
    platform->key_map[(s32)InputKey::R]     = 0x52;
    platform->key_map[(s32)InputKey::S]     = 0x53;
    platform->key_map[(s32)InputKey::T]     = 0x54;
    platform->key_map[(s32)InputKey::U]     = 0x55;
    platform->key_map[(s32)InputKey::V]     = 0x56;
    platform->key_map[(s32)InputKey::W]     = 0x57;
    platform->key_map[(s32)InputKey::X]     = 0x58;
    platform->key_map[(s32)InputKey::Y]     = 0x59;
    platform->key_map[(s32)InputKey::Z]     = 0x5A;

    // Mouse Buttons
    platform->key_map[(s32)InputKey::MOUSE_0] = VK_LBUTTON;
    platform->key_map[(s32)InputKey::MOUSE_1] = VK_RBUTTON;
    platform->key_map[(s32)InputKey::CANCEL]  = VK_CANCEL;
    platform->key_map[(s32)InputKey::MOUSE_2] = VK_MBUTTON; /* NOT contiguous with L & RBUTTON */
    #if(_WIN32_WINNT >= 0x0500)
        platform->key_map[(s32)InputKey::MOUSE_3] = VK_XBUTTON1; /* NOT contiguous with L & RBUTTON */
        platform->key_map[(s32)InputKey::MOUSE_4] = VK_XBUTTON2; /* NOT contiguous with L & RBUTTON */
    #endif /* _WIN32_WINNT >= 0x0500 */

    platform->key_map[(s32)InputKey::BACK]       = VK_BACK;
    platform->key_map[(s32)InputKey::TAB]        = VK_TAB;
    platform->key_map[(s32)InputKey::CLEAR]      = VK_CLEAR;
    platform->key_map[(s32)InputKey::RETURN]     = VK_RETURN;
    platform->key_map[(s32)InputKey::SHIFT]      = VK_SHIFT;
    platform->key_map[(s32)InputKey::CONTROL]    = VK_CONTROL;
    platform->key_map[(s32)InputKey::MENU]       = VK_MENU;
    platform->key_map[(s32)InputKey::PAUSE]      = VK_PAUSE;
    platform->key_map[(s32)InputKey::CAPITAL]    = VK_CAPITAL;
    platform->key_map[(s32)InputKey::KANA]       = VK_KANA;
    platform->key_map[(s32)InputKey::HANGEUL]    = VK_HANGEUL;    /* old name - should be here for compatibility */
    platform->key_map[(s32)InputKey::HANGUL]     = VK_HANGUL;
    platform->key_map[(s32)InputKey::JUNJA]      = VK_JUNJA;
    platform->key_map[(s32)InputKey::FINAL]      = VK_FINAL;
    platform->key_map[(s32)InputKey::HANJA]      = VK_HANJA;
    platform->key_map[(s32)InputKey::KANJI]      = VK_KANJI;
    platform->key_map[(s32)InputKey::ESCAPE]     = VK_ESCAPE;
    platform->key_map[(s32)InputKey::CONVERT]    = VK_CONVERT;
    platform->key_map[(s32)InputKey::NONCONVERT] = VK_NONCONVERT;
    platform->key_map[(s32)InputKey::ACCEPT]     = VK_ACCEPT;
    platform->key_map[(s32)InputKey::MODECHANGE] = VK_MODECHANGE;
    platform->key_map[(s32)InputKey::SPACE]      = VK_SPACE;
    platform->key_map[(s32)InputKey::PRIOR]      = VK_PRIOR;
    platform->key_map[(s32)InputKey::NEXT]       = VK_NEXT;
    platform->key_map[(s32)InputKey::END]        = VK_END;
    platform->key_map[(s32)InputKey::HOME]       = VK_HOME;
    platform->key_map[(s32)InputKey::LEFT]       = VK_LEFT;
    platform->key_map[(s32)InputKey::UP]         = VK_UP;
    platform->key_map[(s32)InputKey::RIGHT]      = VK_RIGHT;
    platform->key_map[(s32)InputKey::DOWN]       = VK_DOWN;
    platform->key_map[(s32)InputKey::SELECT]     = VK_SELECT;
    platform->key_map[(s32)InputKey::PRINT]      = VK_PRINT;
    platform->key_map[(s32)InputKey::EXECUTE]    = VK_EXECUTE; // Out of bounds?
    platform->key_map[(s32)InputKey::SNAPSHOT]   = VK_SNAPSHOT;
    platform->key_map[(s32)InputKey::INSERT]     = VK_INSERT;
    platform->key_map[(s32)InputKey::DELETE_KEY] = VK_DELETE;
    platform->key_map[(s32)InputKey::HELP]       = VK_HELP;
    platform->key_map[(s32)InputKey::LWIN]       = VK_LWIN;
    platform->key_map[(s32)InputKey::RWIN]       = VK_RWIN;
    platform->key_map[(s32)InputKey::APPS]       = VK_APPS;
    platform->key_map[(s32)InputKey::SLEEP]      = VK_SLEEP;
    platform->key_map[(s32)InputKey::NUMPAD_0]   = VK_NUMPAD0;
    platform->key_map[(s32)InputKey::NUMPAD_1]   = VK_NUMPAD1;
    platform->key_map[(s32)InputKey::NUMPAD_2]   = VK_NUMPAD2;
    platform->key_map[(s32)InputKey::NUMPAD_3]   = VK_NUMPAD3;
    platform->key_map[(s32)InputKey::NUMPAD_4]   = VK_NUMPAD4;
    platform->key_map[(s32)InputKey::NUMPAD_5]   = VK_NUMPAD5;
    platform->key_map[(s32)InputKey::NUMPAD_6]   = VK_NUMPAD6;
    platform->key_map[(s32)InputKey::NUMPAD_7]   = VK_NUMPAD7;
    platform->key_map[(s32)InputKey::NUMPAD_8]   = VK_NUMPAD8;
    platform->key_map[(s32)InputKey::NUMPAD_9]   = VK_NUMPAD9;
    platform->key_map[(s32)InputKey::MULTIPLY]   = VK_MULTIPLY;
    platform->key_map[(s32)InputKey::ADD]        = VK_ADD;
    platform->key_map[(s32)InputKey::SEPARATOR]  = VK_SEPARATOR;
    platform->key_map[(s32)InputKey::SUBTRACT]   = VK_SUBTRACT;
    platform->key_map[(s32)InputKey::DECIMAL]    = VK_DECIMAL;
    platform->key_map[(s32)InputKey::DIVIDE]     = VK_DIVIDE;
    platform->key_map[(s32)InputKey::F1]         = VK_F1;
    platform->key_map[(s32)InputKey::F2]         = VK_F2;
    platform->key_map[(s32)InputKey::F3]         = VK_F3;
    platform->key_map[(s32)InputKey::F4]         = VK_F4;
    platform->key_map[(s32)InputKey::F5]         = VK_F5;
    platform->key_map[(s32)InputKey::F6]         = VK_F6;
    platform->key_map[(s32)InputKey::F7]         = VK_F7;
    platform->key_map[(s32)InputKey::F8]         = VK_F8;
    platform->key_map[(s32)InputKey::F9]         = VK_F9;
    platform->key_map[(s32)InputKey::F10]        = VK_F10;
    platform->key_map[(s32)InputKey::F11]        = VK_F11;
    platform->key_map[(s32)InputKey::F12]        = VK_F12;
    platform->key_map[(s32)InputKey::F13]        = VK_F13;
    platform->key_map[(s32)InputKey::F14]        = VK_F14;
    platform->key_map[(s32)InputKey::F15]        = VK_F15;
    platform->key_map[(s32)InputKey::F16]        = VK_F16;
    platform->key_map[(s32)InputKey::F17]        = VK_F17;
    platform->key_map[(s32)InputKey::F18]        = VK_F18;
    platform->key_map[(s32)InputKey::F19]        = VK_F19;
    platform->key_map[(s32)InputKey::F20]        = VK_F20;
    platform->key_map[(s32)InputKey::F21]        = VK_F21;
    platform->key_map[(s32)InputKey::F22]        = VK_F22;
    platform->key_map[(s32)InputKey::F23]        = VK_F23;
    platform->key_map[(s32)InputKey::F24]        = VK_F24;
    #if(_WIN32_WINNT >= 0x0604)
        platform->key_map[(s32)InputKey::NAVIGATION_VIEW]   = VK_NAVIGATION_VIEW;   // reserved
        platform->key_map[(s32)InputKey::NAVIGATION_MENU]   = VK_NAVIGATION_MENU;   // reserved
        platform->key_map[(s32)InputKey::NAVIGATION_UP]     = VK_NAVIGATION_UP;     // reserved
        platform->key_map[(s32)InputKey::NAVIGATION_DOWN]   = VK_NAVIGATION_DOWN;   // reserved
        platform->key_map[(s32)InputKey::NAVIGATION_LEFT]   = VK_NAVIGATION_LEFT;   // reserved
        platform->key_map[(s32)InputKey::NAVIGATION_RIGHT]  = VK_NAVIGATION_RIGHT;  // reserved
        platform->key_map[(s32)InputKey::NAVIGATION_ACCEPT] = VK_NAVIGATION_ACCEPT; // reserved
        platform->key_map[(s32)InputKey::NAVIGATION_CANCEL] = VK_NAVIGATION_CANCEL; // reserved
    #endif /* _WIN32_WINNT >= 0x0604 */
    platform->key_map[(s32)InputKey::NUMLOCK]      = VK_NUMLOCK;
    platform->key_map[(s32)InputKey::SCROLL]       = VK_SCROLL;
    platform->key_map[(s32)InputKey::NUMPAD_EQUAL] = VK_OEM_NEC_EQUAL;  // '='          key      on  numpad
    platform->key_map[(s32)InputKey::FJ_JISHO]     = VK_OEM_FJ_JISHO;   // 'Dictionary' key
    platform->key_map[(s32)InputKey::FJ_MASSHOU]   = VK_OEM_FJ_MASSHOU; // 'Unregister  word'    key
    platform->key_map[(s32)InputKey::FJ_TOUROKU]   = VK_OEM_FJ_TOUROKU; // 'Register    word'    key
    platform->key_map[(s32)InputKey::FJ_LOYA]      = VK_OEM_FJ_LOYA;    // 'Left        OYAYUBI' key
    platform->key_map[(s32)InputKey::FJ_ROYA]      = VK_OEM_FJ_ROYA;    // 'Right       OYAYUBI' key
    platform->key_map[(s32)InputKey::LSHIFT]       = VK_LSHIFT;
    platform->key_map[(s32)InputKey::RSHIFT]       = VK_RSHIFT;
    platform->key_map[(s32)InputKey::LCONTROL]     = VK_LCONTROL;
    platform->key_map[(s32)InputKey::RCONTROL]     = VK_RCONTROL;
    platform->key_map[(s32)InputKey::LMENU]        = VK_LMENU;
    platform->key_map[(s32)InputKey::RMENU]        = VK_RMENU;
    #if(_WIN32_WINNT >= 0x0500)
        platform->key_map[(s32)InputKey::BROWSER_BACK]        = VK_BROWSER_BACK;
        platform->key_map[(s32)InputKey::BROWSER_FORWARD]     = VK_BROWSER_FORWARD;
        platform->key_map[(s32)InputKey::BROWSER_REFRESH]     = VK_BROWSER_REFRESH;
        platform->key_map[(s32)InputKey::BROWSER_STOP]        = VK_BROWSER_STOP;
        platform->key_map[(s32)InputKey::BROWSER_SEARCH]      = VK_BROWSER_SEARCH;
        platform->key_map[(s32)InputKey::BROWSER_FAVORITES]   = VK_BROWSER_FAVORITES;
        platform->key_map[(s32)InputKey::BROWSER_HOME]        = VK_BROWSER_HOME;
        platform->key_map[(s32)InputKey::VOLUME_MUTE]         = VK_VOLUME_MUTE;
        platform->key_map[(s32)InputKey::VOLUME_DOWN]         = VK_VOLUME_DOWN;
        platform->key_map[(s32)InputKey::VOLUME_UP]           = VK_VOLUME_UP;
        platform->key_map[(s32)InputKey::MEDIA_NEXT_TRACK]    = VK_MEDIA_NEXT_TRACK;
        platform->key_map[(s32)InputKey::MEDIA_PREV_TRACK]    = VK_MEDIA_PREV_TRACK;
        platform->key_map[(s32)InputKey::MEDIA_STOP]          = VK_MEDIA_STOP;
        platform->key_map[(s32)InputKey::MEDIA_PLAY_PAUSE]    = VK_MEDIA_PLAY_PAUSE;
        platform->key_map[(s32)InputKey::LAUNCH_MAIL]         = VK_LAUNCH_MAIL;
        platform->key_map[(s32)InputKey::LAUNCH_MEDIA_SELECT] = VK_LAUNCH_MEDIA_SELECT;
        platform->key_map[(s32)InputKey::LAUNCH_APP1]         = VK_LAUNCH_APP1;
        platform->key_map[(s32)InputKey::LAUNCH_APP2]         = VK_LAUNCH_APP2;
    #endif /* _WIN32_WINNT >= 0x0500 */
    platform->key_map[(s32)InputKey::SEMICOLON_COLON] = VK_OEM_1;      // ';:' for US
    platform->key_map[(s32)InputKey::PLUS]            = VK_OEM_PLUS;   // '+'  any country
    platform->key_map[(s32)InputKey::COMMA]           = VK_OEM_COMMA;  // ','  any country
    platform->key_map[(s32)InputKey::MINUS]           = VK_OEM_MINUS;  // '-'  any country
    platform->key_map[(s32)InputKey::PERIOD]          = VK_OEM_PERIOD; // '.'  any country
    platform->key_map[(s32)InputKey::SLASH_QUESTION]  = VK_OEM_2;      // '/?' for US
    platform->key_map[(s32)InputKey::BACKTICK_TILDE]  = VK_OEM_3;      // '`~' for US
    #if(_WIN32_WINNT >= 0x0604)
        // Gamepad Input
        platform->key_map[(s32)InputKey::GAMEPAD_A]                       = VK_GAMEPAD_A;                       // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_B]                       = VK_GAMEPAD_B;                       // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_X]                       = VK_GAMEPAD_X;                       // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_Y]                       = VK_GAMEPAD_Y;                       // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_RIGHT_SHOULDER]          = VK_GAMEPAD_RIGHT_SHOULDER;          // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_LEFT_SHOULDER]           = VK_GAMEPAD_LEFT_SHOULDER;           // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_LEFT_TRIGGER]            = VK_GAMEPAD_LEFT_TRIGGER;            // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_RIGHT_TRIGGER]           = VK_GAMEPAD_RIGHT_TRIGGER;           // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_DPAD_UP]                 = VK_GAMEPAD_DPAD_UP;                 // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_DPAD_DOWN]               = VK_GAMEPAD_DPAD_DOWN;               // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_DPAD_LEFT]               = VK_GAMEPAD_DPAD_LEFT;               // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_DPAD_RIGHT]              = VK_GAMEPAD_DPAD_RIGHT;              // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_MENU]                    = VK_GAMEPAD_MENU;                    // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_VIEW]                    = VK_GAMEPAD_VIEW;                    // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_LEFT_THUMBSTICK_BUTTON]  = VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON;  // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_RIGHT_THUMBSTICK_BUTTON] = VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON; // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_LEFT_THUMBSTICK_UP]      = VK_GAMEPAD_LEFT_THUMBSTICK_UP;      // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_LEFT_THUMBSTICK_DOWN]    = VK_GAMEPAD_LEFT_THUMBSTICK_DOWN;    // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_LEFT_THUMBSTICK_RIGHT]   = VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT;   // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_LEFT_THUMBSTICK_LEFT]    = VK_GAMEPAD_LEFT_THUMBSTICK_LEFT;    // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_RIGHT_THUMBSTICK_UP]     = VK_GAMEPAD_RIGHT_THUMBSTICK_UP;     // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_RIGHT_THUMBSTICK_DOWN]   = VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN;   // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_RIGHT_THUMBSTICK_RIGHT]  = VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT;  // reserved
        platform->key_map[(s32)InputKey::GAMEPAD_RIGHT_THUMBSTICK_LEFT]   = VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT;   // reserved
    #endif /* _WIN32_WINNT >= 0x0604 */

    platform->key_map[(s32)InputKey::OPEN_BRACKET]     = VK_OEM_4; // '[{' for US
    platform->key_map[(s32)InputKey::BACKSLASH_PIPE]   = VK_OEM_5; // '\|' for US
    platform->key_map[(s32)InputKey::CLOSE_BRACKET]    = VK_OEM_6; // ']}' for US
    platform->key_map[(s32)InputKey::APOSTROPHE_QUOTE] = VK_OEM_7; // ''"' for US
    platform->key_map[(s32)InputKey::OEM_8]            = VK_OEM_8;

    // Various Extended or Enhanced Keyboards
    platform->key_map[(s32)InputKey::OEM_AX]   = VK_OEM_AX;   // 'AX' key on  Japanese AX kbd
    platform->key_map[(s32)InputKey::OEM_102]  = VK_OEM_102;  // "<>" or  "\| "        on RT  102-key kbd.
    platform->key_map[(s32)InputKey::ICO_HELP] = VK_ICO_HELP; // Help key on  ICO
    platform->key_map[(s32)InputKey::ICO_00]   = VK_ICO_00;   // 00   key on  ICO
}
