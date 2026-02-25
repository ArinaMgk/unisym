// ASCII CPL TAB4 CRLF
// Docutitle: (Device) Keyboard
// Codifiers: @dosconio: 20240502 ~ 20240502
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../../inc/c/driver/keyboard.h"
#include "../../../inc/c/driver/i8259A.h"

_ESYM_C
// (PS/2 Set 1 scancode & 0x7F) -> USB-IF HID VKC
const byte key_ps2set1_usb[128] = {
	// 0x00 - 0x07 (0x01=Esc, 0x02~0x07=NUM 1~6)
	0,    0x29, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
	// 0x08 - 0x0F (0x0B=0, 0x0C=-, 0x0D==, 0x0E=Backspace, 0x0F=Tab)
	0x24, 0x25, 0x26, 0x27, 0x2D, 0x2E, 0x2A, 0x2B,
	// 0x10 - 0x17 (Q, W, E, R, T, Y, U, I)
	0x14, 0x1A, 0x08, 0x15, 0x17, 0x1C, 0x18, 0x0C,
	// 0x18 - 0x1F (O, P, [, ], Enter, LCtrl, A, S)
	0x12, 0x13, 0x2F, 0x30, 0x28, 0xE0, 0x04, 0x16,
	// 0x20 - 0x27 (D, F, G, H, J, K, L, ;)
	0x07, 0x09, 0x0A, 0x0B, 0x0D, 0x0E, 0x0F, 0x33,
	// 0x28 - 0x2F (', `, LShift, \, Z, X, C, V)
	0x34, 0x35, 0xE1, 0x31, 0x1D, 0x1B, 0x06, 0x19,
	// 0x30 - 0x37 (B, N, M, ,, ., /, RShift, PAD*)
	0x05, 0x11, 0x10, 0x36, 0x37, 0x38, 0xE5, 0x55,
	// 0x38 - 0x3F (LAlt, Space, CapsLock, F1~F5)
	0xE2, 0x2C, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E,
	// 0x40 - 0x47 (F6~F10, NumLock, ScrollLock, PAD 7)
	0x3F, 0x40, 0x41, 0x42, 0x43, 0x53, 0x47, 0x5F,
	// 0x48 - 0x4F (PAD: 8, 9, -, 4, 5, 6, +, 1)
	0x60, 0x61, 0x56, 0x5C, 0x5D, 0x5E, 0x57, 0x59,
	// 0x50 - 0x57 (PAD: 2, 3, 0, ., RESV, RESV, RESV, F11)
	0x5A, 0x5B, 0x62, 0x63, 0,    0,    0,    0x44,
	// 0x58 - 0x5F (F12)
	0x45, 0,    0,    0,    0,    0,    0,    0,
	// 0x60 - 0x7F (RESV 0x73 0x79 0x7B)
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0x87, 0, 0, 0, 0, 0, 0x8A, 0, 0x8B, 0, 0x89, 0, 0,
};
/* JP keyboard
0x73 -> 0x87 (Ro/下劃線), 0x79 -> 0x8A (Henkan/变换), 0x7B -> 0x8B (Muhenkan/无变换), 0x7D -> 0x89 (Yen/竖线)
*/

_ESYM_C
const uint8_t key_ps2set1_usb_E0[128] = {
	[0x1C] = 0x58, // PAD Enter
	[0x1D] = 0xE4, // Right Ctrl
	[0x35] = 0x54, // PAD /
	[0x37] = 0x46, // Print Screen
	[0x38] = 0xE6, // Right Alt
	// 
	[0x47] = 0x4A, // Home
	[0x48] = 0x52, // Up Arrow
	[0x49] = 0x4B, // Page Up
	[0x4B] = 0x50, // Left Arrow
	[0x4D] = 0x4F, // Right Arrow
	[0x4F] = 0x4D, // End
	[0x50] = 0x51, // Down Arrow
	[0x51] = 0x4E, // Page Down
	[0x52] = 0x49, // Insert
	[0x53] = 0x4C, // Delete
	// Modern
	[0x5B] = 0xE3, // Left GUI
	[0x5C] = 0xE7, // Right GUI
	[0x5D] = 0x65, // Apps / Menu 
};


byte keystate_receive_0xE0 = 0;
// ---- Keymap for US MF-2 keyboard ----
// 2 bytes at most
keymap_element_t _tab_keycode2ascii[0x80] =
{ // {SCAN CODE 1}
{ 0  ,  0  , "NUL", "NUL-SHIFT", NULL}, // 0x00
{ 1  ,  1  , "ESC", "ESC-SHIFT", NULL},
{'1' , '!' ,  NULL,  NULL, NULL},
{'2' , '@' ,  NULL,  NULL, NULL},
{'3' , '#' ,  NULL,  NULL, NULL},
{'4' , '$' ,  NULL,  NULL, NULL},
{'5' , '%' ,  NULL,  NULL, NULL},
{'6' , '^' ,  NULL,  NULL, NULL},
{'7' , '&' ,  NULL,  NULL, NULL},
{'8' , '*' ,  NULL,  NULL, NULL},
{'9' , '(' ,  NULL,  NULL, NULL},
{'0' , ')' ,  NULL,  NULL, NULL},
{'-' , '_' ,  NULL,  NULL, NULL},
{'=' , '+' ,  NULL,  NULL, NULL},
{'\b', '\b',  "BACKSPACE", "BACKSPACE", NULL},
{'\t', '\t',  "TAB",  NULL, NULL},
{'q' , 'Q' ,  NULL,  NULL, "WM_PreviousTrack"}, // 0x10
{'w' , 'W' ,  NULL,  NULL, NULL}, // 0x11
{'e' , 'E' ,  NULL,  NULL, NULL}, // 0x12
{'r' , 'R' ,  NULL,  NULL, NULL}, // 0x13
{'t' , 'T' ,  NULL,  NULL, NULL}, // 0x14
{'y' , 'Y' ,  NULL,  NULL, NULL}, // 0x15
{'u' , 'U' ,  NULL,  NULL, NULL}, // 0x16
{'i' , 'I' ,  NULL,  NULL, NULL}, // 0x17
{'o' , 'O' ,  NULL,  NULL, NULL}, // 0x18
{'p' , 'P' ,  NULL,  NULL, "WM_NextTrack"}, // 0x19
{'[' , '{' ,  NULL,  NULL, NULL},
{']' , '}' ,  NULL,  NULL, NULL},
{'\n', '\n', "ENTER",  NULL, "PAD_ENTER"},
{  1 ,  1  , "CTRL", "CTRL-SHIFT", "CTRL_RIGHT"},
{'a' , 'A' ,  NULL,  NULL, NULL},
{'s' , 'S' ,  NULL,  NULL, NULL},
{'d' , 'D' ,  NULL,  NULL, "WM_Mute"}, // 0x20
{'f' , 'F' ,  NULL,  NULL, "WM_Calculator"}, // 0x21
{'g' , 'G' ,  NULL,  NULL, "WM_PlayPause"}, // 0x22
{'h' , 'H' ,  NULL,  NULL, NULL}, // 0x23
{'j' , 'J' ,  NULL,  NULL, "WM_Stop"}, // 0x24
{'k' , 'K' ,  NULL,  NULL, NULL}, // 0x25
{'l' , 'L' ,  NULL,  NULL, NULL}, // 0x26
{';' , ':' ,  NULL,  NULL, NULL}, // 0x27
{'\'', '"' ,  NULL,  NULL, NULL}, // 0x28
{'`' , '~' ,  NULL,  NULL, NULL}, // 0x29
{  1 ,   1 ,  "SHIFT_LEFT", "SHIFT_LEFT", NULL}, // 0x2A
{'\\', '|' ,  NULL,  NULL, NULL}, // 0x2B
{'z' , 'Z' ,  NULL,  NULL, NULL}, // 0x2C
{'x' , 'X' ,  NULL,  NULL, NULL}, // 0x2D
{'c' , 'C' ,  NULL,  NULL, "WM_VolumeDown"}, // 0x2E
{'v' , 'V' ,  NULL,  NULL, NULL}, // 0x2F
{'b' , 'B' ,  NULL,  NULL, "WM_VolumeUp"}, // 0x30
{'n' , 'N' ,  NULL,  NULL, NULL}, // 0x31
{'m' , 'M' ,  NULL,  NULL, "WM_WWWHome"}, // 0x32
{',' , '<' ,  NULL,  NULL, NULL},
{'.' , '>' ,  NULL,  NULL, NULL},
{'/' , '?' ,  NULL,  NULL, "PAD_SLASH"},
{  1 ,   1 ,  "SHIFT_RIGHT", "SHIFT_RIGHT", NULL},
{'*' , '*' ,  NULL,  NULL, NULL},
{  1 ,   1 ,  "ALT_LEFT", "ALT_LEFT", "ALT_RIGHT"},
{' ' , ' ' ,  "SPACE",  NULL, NULL},
{  1 ,   1 ,  "LOCK_CAPS", "LOCK_CAPS", NULL},
{  1 ,   1 ,  "F1" , "F1" , NULL},
{  1 ,   1 ,  "F2" , "F2" , NULL},
{  1 ,   1 ,  "F3" , "F3" , NULL},
{  1 ,   1 ,  "F4" , "F4" , NULL},
{  1 ,   1 ,  "F5" , "F5" , NULL},
{  1 ,   1 ,  "F6" , "F6" , NULL}, // 0x40
{  1 ,   1 ,  "F7" , "F7" , NULL},
{  1 ,   1 ,  "F8" , "F8" , NULL},
{  1 ,   1 ,  "F9" , "F9" , NULL},
{  1 ,   1 ,  "F10", "F10", NULL},
{  1 ,   1 ,  "LOCK_NUM", "LOCK_NUM" , NULL},
{  1 ,   1 ,  "LOCK_SCROLL", "LOCK_SCROLL", NULL},
{  1 ,  '7',  "PAD_HOME", NULL, "HOME" },// 0x47
{  1 ,  '8',  "PAD_UP", NULL, "UP" },
{  1 ,  '9',  "PAD_PAGEUP", NULL, "PAGEUP"},
{  '-' ,  '-',  "PAD_MINUS", NULL, NULL },
{  1 ,  '4',  "PAD_LEFT", NULL, "LEFT"  },
{  1 ,  '5',  "PAD_MID", NULL, "MIDDLE"   },
{  1 ,  '6',  "PAD_RIGHT", NULL, "RIGHT" },
{  '+' ,  '+',  "PAD_PLUS", NULL, NULL  },
{  1 ,  '1',  "PAD_END", NULL, "END"   },
{  1 ,  '2',  "PAD_DOWN", NULL, "DOWN"  }, // 0x50
{  1 ,  '3',  "PAD_PAGEDOWN", NULL, "PAGEDOWN"},
{  1 ,  '0',  "PAD_INS", NULL, "INSERT"   },
{  1 ,  '.',  "PAD_DOT", NULL, "DELETE" },// 0x53
{  0 ,   0 ,  NULL ,  NULL, NULL},
{  0 ,   0 ,  NULL ,  NULL, NULL},
{  0 ,   0 ,  NULL ,  NULL, NULL},
{  1 ,   1 ,  "F11", "F11", NULL},
{  1 ,   1 ,  "F12", "F12", NULL},
{  0 ,   0 ,  NULL ,  NULL, NULL},
{  0 ,   0 ,  NULL ,  NULL, NULL},
{  0 ,   0 ,  NULL ,  NULL, "GUI_LEFT"},
{  0 ,   0 ,  NULL ,  NULL, "GUI_RIGHT"},
{  0 ,   0 ,  NULL ,  NULL, "APPS"},
{  0 ,   0 ,  NULL ,  NULL, "AC_POWER"},// 0x5E
{  0 ,   0 ,  NULL ,  NULL, "AC_SLEEP"},// 0x5F
{  0,0,0,0,0 }, // 0x60
{  0,0,0,0,0 }, // 0x61
{  0,0,0,0,0 }, // 0x62
{  0 ,   0 ,  NULL ,  NULL, "AC_WAKE" },// 0x63
{  0 ,   0 ,  NULL ,  NULL, NULL }, // 0x64
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWSearch" }, // 0x65
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWFavorites" }, // 0x66
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWRefresh" }, // 0x67
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWStop" }, // 0x68
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWForward" }, // 0x69
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWBack" }, // 0x6A
{  0 ,   0 ,  NULL ,  NULL, "WM_MyComputer" }, // 0x6B
{  0 ,   0 ,  NULL ,  NULL, "WM_EMail" }, // 0x6C
{  0 ,   0 ,  NULL ,  NULL, "WM_MediaSelect" }, // 0x6D
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x6E
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x6F
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x70
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x71
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x72
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x73
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x74
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x75
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x76
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x77
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x78
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x79
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x7A
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x7B
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x7C
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x7D
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x7E
{  0 ,   0 ,  NULL ,  NULL , NULL }, // 0x7F
};/* where
* AC stands for ACPI Scan Codes
* WM stands for Windows Multimedia Scan Codes
*/

//{TODO} Multi-bytes key strategy

#ifdef _SUPPORT_Port8

#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47// support mouse

void Keyboard_Wait()// for buffer empty
{
	while (innpb(PORT_KEYBOARD_CMD) & 0x02);
}
static void Keyboard_Acknowledge()
{
	innpb(PORT_KEYBOARD_DAT);// == KEYBOARD_ACK
}

//

void Keyboard_Init()
{
	i8259Master_Enable(1);// Master1 is linked with RTC
	//
	// ---- make support for mouse
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_CMD, KEYCMD_WRITE_MODE);
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_DAT, KBC_MODE);
}

void KbdSetLED(byte stat)
{
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_DAT, KEYBOARD_LED);
	Keyboard_Acknowledge();
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_DAT, stat);
	Keyboard_Acknowledge();
}

#endif
