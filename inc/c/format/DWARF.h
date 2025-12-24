// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format: File) DWARF
// Copyright: UNISYM, under Apache License 2.0
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

#ifndef _INC_DWARF
#define _INC_DWARF

#define DW_TAG_compile_unit  0x11
#define DW_TAG_subprogram    0x2e
#define DW_AT_name           0x03
#define DW_AT_stmt_list      0x10
#define DW_AT_low_pc         0x11
#define DW_AT_high_pc        0x12
#define DW_AT_language       0x13
#define DW_AT_producer       0x25
#define DW_AT_frame_base     0x40
#define DW_FORM_addr         0x01
#define DW_FORM_data2        0x05
#define DW_FORM_data4        0x06
#define DW_FORM_string       0x08
#define DW_LNS_extended_op   0
#define DW_LNS_advance_pc    2
#define DW_LNS_advance_line  3
#define DW_LNS_set_file      4
#define DW_LNE_end_sequence  1
#define DW_LNE_set_address   2
#define DW_LNE_define_file   3
#define DW_LANG_Mips_Assembler  0x8001

#endif
