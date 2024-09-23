// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Keyboard
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

keymap_element_t _tab_keycode2ascii[0x80] =
{
{ 0  ,  0  , "NUL", "NUL-SHIFT"},
{ 1  ,  1  , "ESC", "ESC-SHIFT"},// ESC
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
{'\t', '\t',  NULL,  NULL},// TAB
{'q' , 'Q' ,  NULL,  NULL},
{'w' , 'W' ,  NULL,  NULL},
{'e' , 'E' ,  NULL,  NULL},
{'r' , 'R' ,  NULL,  NULL},
{'t' , 'T' ,  NULL,  NULL},
{'y' , 'Y' ,  NULL,  NULL},
{'u' , 'U' ,  NULL,  NULL},
{'i' , 'I' ,  NULL,  NULL},
{'o' , 'O' ,  NULL,  NULL},
{'p' , 'P' ,  NULL,  NULL},
{'[' , '{' ,  NULL,  NULL},
{']' , '}' ,  NULL,  NULL},
{'\n', '\n',  NULL,  NULL},// ENTER 0xE0(PAD_ENTER)
{  1 ,  1  , "CTRL", "CTRL-SHIFT"},// CTRL(Left) 0xE0(CTRL(Right))
{'a' , 'A' ,  NULL,  NULL},
{'s' , 'S' ,  NULL,  NULL},
{'d' , 'D' ,  NULL,  NULL},
{'f' , 'F' ,  NULL,  NULL},
{'g' , 'G' ,  NULL,  NULL},
{'h' , 'H' ,  NULL,  NULL},
{'j' , 'J' ,  NULL,  NULL},
{'k' , 'K' ,  NULL,  NULL},
{'l' , 'L' ,  NULL,  NULL},
{';' , ':' ,  NULL,  NULL},
{'\'', '"' ,  NULL,  NULL},
{'`' , '~' ,  NULL,  NULL},
{  1 ,   1 ,  "SHIFT_LEFT", "SHIFT_LEFT"},
{'\\', '|' ,  NULL,  NULL},
{'z' , 'Z' ,  NULL,  NULL},
{'x' , 'X' ,  NULL,  NULL},
{'c' , 'C' ,  NULL,  NULL},
{'v' , 'V' ,  NULL,  NULL},
{'b' , 'B' ,  NULL,  NULL},
{'n' , 'N' ,  NULL,  NULL},
{'m' , 'M' ,  NULL,  NULL},
{',' , '<' ,  NULL,  NULL},
{'.' , '>' ,  NULL,  NULL},
{'/' , '?' ,  NULL,  NULL}, // 0xE0(PAD_SLASH)
{  1 ,   1 ,  "SHIFT_RIGHT", "SHIFT_RIGHT"},
{'*' , '*' ,  NULL,  NULL},
{  1 ,   1 ,  "ALT_LEFT", "ALT_LEFT"}, // 0xE0(ALT-RIGHT)
{' ' , ' ' ,  NULL,  NULL},// SPACE
{  1 ,   1 ,  "LOCK_CAPS", "LOCK_CAPS"},
{  1 ,   1 ,  "F1" , "F1" },
{  1 ,   1 ,  "F2" , "F2" },
{  1 ,   1 ,  "F3" , "F3" },
{  1 ,   1 ,  "F4" , "F4" },
{  1 ,   1 ,  "F5" , "F5" },
{  1 ,   1 ,  "F6" , "F6" },
{  1 ,   1 ,  "F7" , "F7" },
{  1 ,   1 ,  "F8" , "F8" },
{  1 ,   1 ,  "F9" , "F9" },
{  1 ,   1 ,  "F10", "F10"},
{  1 ,   1 ,  "LOCK_NUM", "LOCK_NUM" },
{  1 ,   1 ,  "LOCK_SCROLL", "LOCK_SCROLL"},
{  1 ,  '7',  "PAD_HOME", NULL  }, // 0xE0(HOME)
{  1 ,  '8',  "PAD_UP", NULL    }, // 0xE0(UP)
{  1 ,  '9',  "PAD_PAGEUP", NULL}, // 0xE0(PAGEUP)
{  1 ,  '-',  "PAD_MINUS", NULL },
{  1 ,  '4',  "PAD_LEFT", NULL  }, // 0xE0(LEFT)
{  1 ,  '5',  "PAD_MID", NULL   }, // 0xE0(MID)
{  1 ,  '6',  "PAD_RIGHT", NULL }, // 0xE0(RIGHT)
{  1 ,  '+',  "PAD_PLUS", NULL  },
{  1 ,  '1',  "PAD_END", NULL   }, // 0xE0(END)
{  1 ,  '2',  "PAD_DOWN", NULL  }, // 0xE0(DOWN)
{  1 ,  '3',  "PAD_PAGEDOWN", NULL}, // 0xE0(PAGEDOWN)
{  1 ,  '0',  "PAD_INS", NULL   }, // 0xE0(INSERT)
{  1 ,  '.',  "PAD_DOT", NULL   }, // 0xE0(DELETE)
{  0 ,   0 ,  NULL ,  NULL},
{  0 ,   0 ,  NULL ,  NULL},
{  0 ,   0 ,  NULL ,  NULL},
{  1 ,   1 ,  "F11", "F11"},
{  1 ,   1 ,  "F12", "F12"},
{  0 ,   0 ,  NULL ,  NULL},
{  0 ,   0 ,  NULL ,  NULL},
{  0 ,   0 ,  NULL ,  NULL}, // 0xE0(GUI_LEFT)
{  0 ,   0 ,  NULL ,  NULL}, // 0xE0(GUI_RIGHT)
{  0 ,   0 ,  NULL ,  NULL}, // 0xE0(APPS)
};

#ifdef _SUPPORT_Port8
void Keyboard_Init()
{
	i8259Master_Enable(1);// Master1 is linked with RTC
}
#endif
