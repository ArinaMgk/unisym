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

byte keystate_receive_0xE0 = 0;
// ---- Keymap for US MF-2 keyboard ----
// 2 bytes at most
keymap_element_t _tab_keycode2ascii[0x80] =
{ // {SCAN CODE 1}
{ 0  ,  0  , "NUL", "NUL-SHIFT"}, // 0x00
{ 1  ,  1  , "ESC", "ESC-SHIFT"},
{'1' , '!' ,  NULL,  NULL},
{'2' , '@' ,  NULL,  NULL},
{'3' , '#' ,  NULL,  NULL},
{'4' , '$' ,  NULL,  NULL},
{'5' , '%' ,  NULL,  NULL},
{'6' , '^' ,  NULL,  NULL},
{'7' , '&' ,  NULL,  NULL},
{'8' , '*' ,  NULL,  NULL},
{'9' , '(' ,  NULL,  NULL},
{'0' , ')' ,  NULL,  NULL},
{'-' , '_' ,  NULL,  NULL},
{'=' , '+' ,  NULL,  NULL},
{'\b', '\b',  "BACKSPACE", "BACKSPACE"},
{'\t', '\t',  "TAB",  NULL},
{'q' , 'Q' ,  NULL,  NULL, "WM_PreviousTrack"}, // 0x10
{'w' , 'W' ,  NULL,  NULL}, // 0x11
{'e' , 'E' ,  NULL,  NULL}, // 0x12
{'r' , 'R' ,  NULL,  NULL}, // 0x13
{'t' , 'T' ,  NULL,  NULL}, // 0x14
{'y' , 'Y' ,  NULL,  NULL}, // 0x15
{'u' , 'U' ,  NULL,  NULL}, // 0x16
{'i' , 'I' ,  NULL,  NULL}, // 0x17
{'o' , 'O' ,  NULL,  NULL}, // 0x18
{'p' , 'P' ,  NULL,  NULL, "WM_NextTrack"}, // 0x19
{'[' , '{' ,  NULL,  NULL},
{']' , '}' ,  NULL,  NULL},
{'\n', '\n', "ENTER",  NULL, "PAD_ENTER"},
{  1 ,  1  , "CTRL", "CTRL-SHIFT", "CTRL_RIGHT"},
{'a' , 'A' ,  NULL,  NULL},
{'s' , 'S' ,  NULL,  NULL},
{'d' , 'D' ,  NULL,  NULL, "WM_Mute"}, // 0x20
{'f' , 'F' ,  NULL,  NULL, "WM_Calculator"}, // 0x21
{'g' , 'G' ,  NULL,  NULL, "WM_PlayPause"}, // 0x22
{'h' , 'H' ,  NULL,  NULL}, // 0x23
{'j' , 'J' ,  NULL,  NULL, "WM_Stop"}, // 0x24
{'k' , 'K' ,  NULL,  NULL}, // 0x25
{'l' , 'L' ,  NULL,  NULL}, // 0x26
{';' , ':' ,  NULL,  NULL}, // 0x27
{'\'', '"' ,  NULL,  NULL}, // 0x28
{'`' , '~' ,  NULL,  NULL}, // 0x29
{  1 ,   1 ,  "SHIFT_LEFT", "SHIFT_LEFT"}, // 0x2A
{'\\', '|' ,  NULL,  NULL}, // 0x2B
{'z' , 'Z' ,  NULL,  NULL}, // 0x2C
{'x' , 'X' ,  NULL,  NULL}, // 0x2D
{'c' , 'C' ,  NULL,  NULL, "WM_VolumeDown"}, // 0x2E
{'v' , 'V' ,  NULL,  NULL}, // 0x2F
{'b' , 'B' ,  NULL,  NULL, "WM_VolumeUp"}, // 0x30
{'n' , 'N' ,  NULL,  NULL}, // 0x31
{'m' , 'M' ,  NULL,  NULL, "WM_WWWHome"}, // 0x32
{',' , '<' ,  NULL,  NULL},
{'.' , '>' ,  NULL,  NULL},
{'/' , '?' ,  NULL,  NULL, "PAD_SLASH"},
{  1 ,   1 ,  "SHIFT_RIGHT", "SHIFT_RIGHT"},
{'*' , '*' ,  NULL,  NULL},
{  1 ,   1 ,  "ALT_LEFT", "ALT_LEFT", "ALT_RIGHT"},
{' ' , ' ' ,  "SPACE",  NULL},
{  1 ,   1 ,  "LOCK_CAPS", "LOCK_CAPS"},
{  1 ,   1 ,  "F1" , "F1" },
{  1 ,   1 ,  "F2" , "F2" },
{  1 ,   1 ,  "F3" , "F3" },
{  1 ,   1 ,  "F4" , "F4" },
{  1 ,   1 ,  "F5" , "F5" },
{  1 ,   1 ,  "F6" , "F6" }, // 0x40
{  1 ,   1 ,  "F7" , "F7" },
{  1 ,   1 ,  "F8" , "F8" },
{  1 ,   1 ,  "F9" , "F9" },
{  1 ,   1 ,  "F10", "F10"},
{  1 ,   1 ,  "LOCK_NUM", "LOCK_NUM" },
{  1 ,   1 ,  "LOCK_SCROLL", "LOCK_SCROLL"},
{  1 ,  '7',  "PAD_HOME", NULL, "HOME" },// 0x47
{  1 ,  '8',  "PAD_UP", NULL, "UP" },
{  1 ,  '9',  "PAD_PAGEUP", NULL, "PAGEUP"},
{  '-' ,  '-',  "PAD_MINUS", NULL },
{  1 ,  '4',  "PAD_LEFT", NULL, "LEFT"  },
{  1 ,  '5',  "PAD_MID", NULL, "MIDDLE"   },
{  1 ,  '6',  "PAD_RIGHT", NULL, "RIGHT" },
{  '+' ,  '+',  "PAD_PLUS", NULL  },
{  1 ,  '1',  "PAD_END", NULL, "END"   },
{  1 ,  '2',  "PAD_DOWN", NULL, "DOWN"  }, // 0x50
{  1 ,  '3',  "PAD_PAGEDOWN", NULL, "PAGEDOWN"},
{  1 ,  '0',  "PAD_INS", NULL, "INSERT"   },
{  1 ,  '.',  "PAD_DOT", NULL, "DELETE" },// 0x53
{  0 ,   0 ,  NULL ,  NULL},
{  0 ,   0 ,  NULL ,  NULL},
{  0 ,   0 ,  NULL ,  NULL},
{  1 ,   1 ,  "F11", "F11"},
{  1 ,   1 ,  "F12", "F12"},
{  0 ,   0 ,  NULL ,  NULL},
{  0 ,   0 ,  NULL ,  NULL},
{  0 ,   0 ,  NULL ,  NULL, "GUI_LEFT"},
{  0 ,   0 ,  NULL ,  NULL, "GUI_RIGHT"},
{  0 ,   0 ,  NULL ,  NULL, "APPS"},
{  0 ,   0 ,  NULL ,  NULL, "AC_POWER"},// 0x5E
{  0 ,   0 ,  NULL ,  NULL, "AC_SLEEP"},// 0x5F
{  0 }, // 0x60
{  0 }, // 0x61
{  0 }, // 0x62
{  0 ,   0 ,  NULL ,  NULL, "AC_WAKE" },// 0x63
{  0 ,   0 ,  NULL ,  NULL }, // 0x64
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWSearch" }, // 0x65
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWFavorites" }, // 0x66
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWRefresh" }, // 0x67
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWStop" }, // 0x68
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWForward" }, // 0x69
{  0 ,   0 ,  NULL ,  NULL, "WM_WWWBack" }, // 0x6A
{  0 ,   0 ,  NULL ,  NULL, "WM_MyComputer" }, // 0x6B
{  0 ,   0 ,  NULL ,  NULL, "WM_EMail" }, // 0x6C
{  0 ,   0 ,  NULL ,  NULL, "WM_MediaSelect" }, // 0x6D
{  0 ,   0 ,  NULL ,  NULL }, // 0x6E
{  0 ,   0 ,  NULL ,  NULL }, // 0x6F
{  0 ,   0 ,  NULL ,  NULL }, // 0x70
{  0 ,   0 ,  NULL ,  NULL }, // 0x71
{  0 ,   0 ,  NULL ,  NULL }, // 0x72
{  0 ,   0 ,  NULL ,  NULL }, // 0x73
{  0 ,   0 ,  NULL ,  NULL }, // 0x74
{  0 ,   0 ,  NULL ,  NULL }, // 0x75
{  0 ,   0 ,  NULL ,  NULL }, // 0x76
{  0 ,   0 ,  NULL ,  NULL }, // 0x77
{  0 ,   0 ,  NULL ,  NULL }, // 0x78
{  0 ,   0 ,  NULL ,  NULL }, // 0x79
{  0 ,   0 ,  NULL ,  NULL }, // 0x7A
{  0 ,   0 ,  NULL ,  NULL }, // 0x7B
{  0 ,   0 ,  NULL ,  NULL }, // 0x7C
{  0 ,   0 ,  NULL ,  NULL }, // 0x7D
{  0 ,   0 ,  NULL ,  NULL }, // 0x7E
{  0 ,   0 ,  NULL ,  NULL }, // 0x7F
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
