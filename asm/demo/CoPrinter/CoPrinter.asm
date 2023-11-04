; GBK [MASM 6.11] SPACE4 demo project @dosconio
; CoPrinter 2023/10/25~30
; ==== ==== INCLUDE sy6.inc ==== ====
    ; patial from unisym.org repository /dosconio/unisym/tree/main/asm
    ; -----------asm\m_INT10p0X.asm-----------
    ConRollUp macro lines, attr, p0col, p0row, p1col, p1row
        push ax
        push cx
        push dx
        push bx
        mov ah, 006h
        mov al, lines
        mov bh, attr
        mov cl, p0col
        mov ch, p0row
        mov dl, p1col
        mov dh, p1row
        int 10h
        pop bx
        pop dx
        pop cx
        pop ax
    endm

    ConCursorGet macro pgno
        push ax
        push bx
        mov ah, 003h
        mov bh, pgno
        int 10h
        pop bx
        pop ax
    endm

    ConCursorSet macro col, row, pgno
        push ax
        push dx
        push bx
        mov ah, 002h
        mov dh, row
        mov dl, col
        mov bh, pgno
        int 10h
        pop bx
        pop dx
        pop ax
    endm

    ConPrintCharFore macro ascii, fore, pgno
        push ax
        push bx
        mov al, ascii; make this before fixed 00Eh
        mov bl, fore
        mov bh, pgno
        mov ah, 00Eh
        int 10h
        pop bx
        pop ax
    endm

    ConPageSet macro pgno, bufseg
        push ax
        push bx
        mov ah, 005h
        mov al, pgno
        mov bx, bufseg
        int 10h
        pop bx
        pop ax
    endm

    ; -----------asm\MSDOS\INT21p3X.ASM INT21p4X.ASM-----------
    ; CF(Err),AX(Errno/Handle) <<< DskFileCreate <<< DS:DX(ASCIZAddr),CX(Attributes)
    DskFileCreate macro Addr, Attr
        push cx
        push dx
        mov dx, Addr
        mov cx, Attr
        mov ah, 03Ch
        int 21h
        pop dx
        pop cx
    endm

    ; AH = 3Eh DOS 2+ - CLOSE A FILE WITH HANDLE - DskFileClose
    ; CF(Err),AX(Errno) <<< DskFileClose <<< BX(Handle)
    DskFileClose macro Handle
        push bx
        mov bx, Handle
        mov ah, 03Eh
        int 21h
        pop bx
    endm

    ; AH = 40h DOS 2+ - WRITE TO FILE WITH HANDLE - DskFileWrite
    ; CF(Err),AX(Errno/BytofWritten) <<< DskFileWrite <<< DS:DX(Addr),CX(Count),BX(Handle)
    ; - if CX is zero, no data is written, and the file is truncated or extended to the current position
    DskFileWrite macro Addr,Count,Handle
        push cx
        push dx
        push bx
        mov ah, 040h
        mov bx, Handle
        mov cx, Count
        mov dx, Addr
        int 21h
        pop bx
        pop dx
        pop cx
    endm

    ; -----------asm\m_Usualint.asm-----------
    ConTerm macro
        ; mov ah, 000h <<< (CONTROVERSIAL)
        mov ax, 4C00h
        int 21h
    endm
    ConPrintCharsFore macro str, fore, pgno
        push si
        push ax
        mov si, str
        ConPrintCharsFore_lup:
            cmp byte ptr[si], 0
            jz ConPrintCharsFore_endo
            ConPrintCharFore [si],fore,pgno
            inc si
            jmp ConPrintCharsFore_lup
        ConPrintCharsFore_endo:
        pop ax
        pop si
    endm

    Puts macro lbl
        mov si, offset lbl
        call __puts
    endm

    ConGetc macro
        xor ah, ah
        int 16h
    endm

    ConVideoBufferSetCursor macro ascii, attr, pgno
        push ax
        push bx
        push cx
        mov ah, 009h
        mov al, ascii
        mov bl, attr
        mov bh, pgno
        mov cx, 1
        int 10h
        pop cx
        pop bx
        pop ax
    endm

    ; ---- self def ----

    CrtLn macro
        mov ax, 0B80AH
        mov es, ax
        mov al, RB_ROW
        dec al
        mov bx, 160
        mul bl; >>> ax
        mov dl, LT_COL
        shl dl, 1; at most (79*2 < 255)
        add al, dl
        adc ah, 0
        mov bx, ax
    endm

; ==== ==== ==== ==== ==== ==== ==== ====
; ==== ==== INCLUDE sy6.dat ==== ====
DATA SEGMENT
    ; for bed
    TotalP0C EQU 0
    TotalP0R EQU 0
    TotalP1C EQU 80-1
    TotalP1R EQU 25-1
    LogoMsg     	DB 'Cong Printer 2023',13,10,0
    COLOR_0     	DB 05EH
    COLOR_1     	DB 0E5H
    text_state  	DB '[F1]Help    [F2]Resize  [F3]Theme   [F4]Reboot  [F5]About   [!]Quit    ...(F1)',0; Global Scope
    text_config 	DB '                              ==== CONFIGURATION ====',13,10
                    DB 'Input [F10] to create a full-screen windows, titled with "TEXT"',13,10,0
    text_01     	DB 'Input 2 numbers, for the COL of left-top point  (range 03~76):',13,10,0
        LT_COL  	DB ?
    text_02     	DB 'Input 2 numbers, for the ROW of left-top point  (range 02~20):',13,10,0
        LT_ROW  	DB ?
    text_03     	DB 'Input 2 numbers, for the COL of right-btm point (range '
        RBC_PROM	DB '??~78):',13,10,0
        RB_COL  	DB ?
    text_04     	DB 'Input 2 numbers, for the ROW of right-btm point (range '
        RBR_PROM	DB '??~22):',13,10,0
        RB_ROW  	DB ?
    text_00     	DB 'You input "00", if finishing "0000,0000", this will quit, or re-input',13,10,0
    text_ow     	DB 'Your input is over the range! Press any key to re-try.',0
    text_buf    	DB 81 dup(0)
    text_ptr    	DW 0
                    ;
    help_title  	DB '                              ==== HELP INFORMATION ====',13,10,10
    help_body   	DB 'The program is so easy that you need nothing helps.',13,10
                    DB 'In Configuration form(enter by [F2]):',13,10
                    DB '[F10] Re-size with the maximum window "TEXT"',13,10
                    DB 13,10
                    DB 'In Editor form:',13,10
                    DB '"EXIT" of a line: exit the program (The length of the line should >= than 4)',13,10
                    DB 13,10
    help_end    	DB 'Press any key to back...',0
    exit_title  	DB ' Do you want to quit? Press [!] to quit to continue',13,10
                    DB ', press [1] to input the character "!" in echo window',13,10
                    DB ', else to cancel. ',0
                    ;
    about_title 	DB '                              ==== Xuecong Printer ====',13,10
                    DB '^_^',13,10,10
                    DB '2023/10/25, by dosconio aka Yang Xuecong, dosconyo@gmail.com',13,10
                    DB '[UNISYM]Repo. https://github.com/dosconio/unisym/tree/main/asm/demo/coprinter/',13,10
                    DB 'If you like this, please give me a STAR~',13,10
                    DB 10
                    DB 'Copyright(C) 2023 dosconio and UNISYM',13,10
                    DB 10
                    DB 'Licensed under the Apache License, Version 2.0 (the "License");',13,10
                    DB 'you may not use this file except in compliance with the License.',13,10
                    DB 'You may obtain a copy of the License at',13,10
                    DB '    http://www.apache.org/licenses/LICENSE-2.0',13,10
                    DB 0
                    ;
    them_title  	DB '                              ==== CHOOSE THE THEME ====',13,10
                    DB '[1] Default yellow-rose.',13,10
                    DB '[2] Dark Rose',13,10
                    DB '[3] Blackboard',13,10
                    DB '[4] Black on white',13,10
                    DB '[5] Green on black',13,10
                    DB 0
    LNNUMBER    	DB 1
    BYT_TMP     	DB ?
    BY2_TMP     	DB ?
    FILENAME    	DB 'TEXT.TXT',0
    FHandle     	DW 0
    FHeader     	DB 'CoPrinter:',13,10
                    DB 0
    FHEADLEN EQU 12;LENGTH FHeader
DATA ENDS

STACK SEGMENT STACK
    STA DW 64 DUP(?)
    TOP EQU LENGTH STA
STACK ENDS

; ==== ==== ==== ==== ==== ==== ==== ====

CODE SEGMENT
    ASSUME CS:CODE, DS:DATA, SS:STACK, ES:DATA
START:
    MOV AX, DATA
    MOV DS, AX
    MOV ES, AX
    MOV AX, STACK
    MOV SS, AX
    MOV SP, TOP
    call __init__
    call __clear_screen
        Puts text_config
        Puts text_01
    proc_00:; ----{range 03~76}----
        call __get_byte
        cmp dl, 0
            je is_00
        jdg_00:
            cmp dl, 3
            jae jdg_00_out1
            call __err_ow
            jmp proc_00
        jdg_00_out1:
            cmp dl, 76
            jbe jdg_00_out2
            call __err_ow
            jmp proc_00
        jdg_00_out2:
            mov LT_COL, dl
            call __new_line

        Puts text_02
    proc_01:; ----{range 02~20}----
        call __get_byte
        jdg_01:
            cmp dl, 2
            jae jdg_01_out1
            call __err_ow
            jmp proc_01
        jdg_01_out1:
            cmp dl, 20
            jbe jdg_01_out2
            call __err_ow
            jmp proc_01
        jdg_01_out2:
            mov LT_ROW, dl
            call __new_line

        mov cl, LT_COL
        inc cl
        mov si, offset RBC_PROM
        call __write_byte_si
        Puts text_03
    proc_02:; ----{LT_COL+1<re-write the prom.> ~ 78}----
        call __get_byte
        jdg_02:
            cmp dl, cl
            jae jdg_02_out1
            call __err_ow
            jmp proc_02
        jdg_02_out1:
            cmp dl, 78
            jbe jdg_02_out2
            call __err_ow
            jmp proc_02
        jdg_02_out2:
            mov RB_COL, dl
            call __new_line

        mov cl, LT_ROW
        inc cl
        mov si, offset RBR_PROM
        call __write_byte_si
        Puts text_04
    proc_03:; ----{LT_ROW+1<re-write the prom.> ~ 22}----
        call __get_byte
        jdg_03:
            cmp dl, cl
            jae jdg_03_out1
            call __err_ow
            jmp proc_03
        jdg_03_out1:
            cmp dl, 22
            jbe jdg_03_out2
            call __err_ow
            jmp proc_03
        jdg_03_out2:
            mov RB_ROW, dl
            call __new_line

    proc_04_cls:
        call __clear_screen
        ConRollUp 0,COLOR_1,LT_COL,LT_ROW,RB_COL,RB_ROW; Draw Text Block
        mov dh, LT_ROW
        mov dl, LT_COL
        dec dh
        dec dl
        sub dl, 2
        ConCursorSet dl,dh,0
        ConPrintCharFore 201,0,0;Box drawing character double line upper left corner
        draw_top_line:
            mov cl, RB_COL
            mov ch, LT_COL
            inc cl
            add cl, 2; column of LineNo.
            sub cl, ch
            xor ch, ch
            draw_top_line_lup:
                ConPrintCharFore 205,0,0;Box drawing character double horizontal line
                loop draw_top_line_lup
        ConPrintCharFore 187,0,0;Box drawing character double line upper right corner
        push ax; ---- avoid duplic.----
            mov al, LT_COL
            sub al, 2
            mov BYT_TMP, al
            inc al
            mov BY2_TMP, al
        pop ax
        draw_left_line:
            mov dl, BYT_TMP
            dec dl
            ConCursorSet dl,LT_ROW,0
            xor cx, cx
            mov cl, RB_ROW
            mov al, LT_ROW
            inc cl
            sub cl, al
            push cx
            ConCursorGet 0
            pop cx
            draw_left_line_lup:
                ConCursorSet dl,dh,0
                ConPrintCharFore 186,0,0; Box drawing character double vertical line
                inc dh
                loop draw_left_line_lup
        draw_right_line:; >>> base on left
            mov dl, RB_COL
            inc dl
            ConCursorSet dl,LT_ROW,0
            xor cx, cx
            mov cl, RB_ROW
            mov al, LT_ROW
            inc cl
            sub cl, al
            push cx
            ConCursorGet 0
            pop cx
            draw_right_line_lup:
                ConCursorSet dl,dh,0
                ConPrintCharFore 186,0,0; Box drawing character double vertical line
                inc dh
                loop draw_right_line_lup
        mov dl, BYT_TMP
        dec dl
        mov dh, RB_ROW
        inc dh
        ConCursorSet dl, dh, 0

        ConPrintCharFore 200,0,0; Box drawing character double line lower left corner

        draw_bottom_line:; base-on-top
            mov cl, RB_COL
            mov ch, LT_COL
            inc cl
            add cl, 2; column of LineNo.
            sub cl, ch
            xor ch, ch
            draw_bottom_line_lup:
                ConPrintCharFore 205,0,0;Box drawing character double horizontal line
                loop draw_bottom_line_lup
        ConPrintCharFore 188,0,0; Box drawing character double line lower right corner

        draw_title_bar:
            mov dh, LT_ROW
            dec dh
            mov dl, LT_COL
            add dl, RB_COL
            shr dl, 1
            sub dl, 2
            ConCursorSet dl,dh,0
            ConVideoBufferSetCursor 'T',01011111B,0
            inc dl
            ConCursorSet dl,dh,0
            ConVideoBufferSetCursor 'E',01110101B,0
            inc dl
            ConCursorSet dl,dh,0
            ConVideoBufferSetCursor 'X',01011111B,0
            inc dl
            ConCursorSet dl,dh,0
            ConVideoBufferSetCursor 'T',01110101B,0; Q  w  Q ~~~
        draw_line_number_01:
            ConCursorSet BYT_TMP,RB_ROW,0
            mov dl, LNNUMBER
            add byte ptr LNNUMBER, 1
            call __dbg_byte

    lup:
        col_update:
            push dx
            ConCursorGet 0
            push dx
                push dx
                    mov dl, 80-2
                    xor dh, dh
                    ConCursorSet dl,dh,0
                pop dx
                sub dl, LT_COL
                call __dbg_byte
                cmp dl, 4
                    jne col_update_conti0
                    call __chk_04
                    col_update_conti0:
            pop dx
            ConCursorSet dl,dh,0
            pop dx
        call __getch
        cmp al, 13; CR
            je is_cr
        cmp ah, 0eh; BACKSPA
            je is_back
        cmp ah, 15; TAB
            je is_table
        cmp al, 20H
            jb lup
        cmp al, 0FFH
            jae lup
        ConPrintCharFore al,0,0
        ConCursorGet 0
        cmp RB_COL, dl; see what comment? goto see unisym. Jya ...
            jb is_cr

    jmp lup
    endo:
        ConPageSet 1,0
        ConRollUp 0,COLOR_1,TotalP0C,TotalP0R,TotalP1C,TotalP1R; Clear the screen
        _FF_QUIT_Prints:
            ConCursorSet 0,1,1
            Puts exit_title; {Potential Pgno}
        ConGetc
            cmp al, '!'
                je endo_real
            cmp al, '1'
                je endo_input_exclamation_mark
        ConPageSet 0,0
        xor al, al
        ret
        jmp lup
        endo_real:
            cmp word ptr FHandle, 0
                je endo_real_next
            DskFileClose FHandle
            mov word ptr FHandle, 0
            endo_real_next:
            pop ax
            ConPageSet 0,0
            ConTerm
        endo_input_exclamation_mark:
            mov al, '!'
            ConPageSet 0,0
            ret
    is_cr:
        call __new_line_block
        jmp lup
    is_back:
        ConCursorGet 0
        cmp LT_COL, dl
            jae lup
        dec dl
        ConCursorSet dl,dh,0
        ConPrintCharFore ' ',0,0
        ConCursorSet dl,dh,0
        jmp lup
    is_table:
        ConCursorGet 0
        add dl, 4; <MAGIKA NUM>
        cmp RB_COL, dl; see what comment? goto see unisym. Jya ...
            jb is_cr
        ConCursorSet dl,dh,0; else
        jmp lup
; ==== ==== INCLUDE sy6.ext ==== ====
__puts:
    ConPrintCharsFore si,0,0
    ret
__getch:
    ConGetc
    cmp ah, 03BH; [F1] Help
        je _F1_HELP
    cmp ah, 03CH; [F2] Resize, care cmd's F2
        je START
    cmp ah, 03DH; [F3]
        je _F3_THEME
    cmp ah, 03EH; [F4]
        je _F4_REBOOT
    cmp ah, 03FH; [F5] About Page
        je _F5_ABOUT
    cmp al, '!'
        jne __getch_back
        call endo
        or al, al
            jnz __getch_back
        jmp __getch
    __getch_back:
    ret
    _F1_HELP:
        ConPageSet 1,0
        ConRollUp 0,COLOR_1,TotalP0C,TotalP0R,TotalP1C,TotalP1R; Clear the screen
        _F1_HELP_Prints:
            ConCursorSet 0,1,1
            Puts help_title; {Potential Pgno}
        ConGetc
        ConPageSet 0,0
        jmp __getch
    _F3_THEME:
        ConPageSet 2,0
        ConRollUp 0,COLOR_1,TotalP0C,TotalP0R,TotalP1C,TotalP1R; Clear the screen
        _F3_THEME_Prints:
            ConCursorSet 0,1,2
            Puts them_title; {Potential Pgno}
        ConGetc
            cmp al, '1'
                je _F3_THEME_DEFA
            cmp al, '2'
                je _F3_THEME_Dark_Rose
            cmp al, '3'
                je _F3_THEME_Blackboard
            cmp al, '4'
                je _F3_THEME_BlackOnWhite
            cmp al, '5'
                je _F3_THEME_GreenOnBlack
        ConPageSet 0,0
        jmp __getch
        _F3_THEME_DEFA:; [1]
            mov byte ptr COLOR_0, 05EH
            mov byte ptr COLOR_1, 0E5H
            ConPageSet 0,0
            jmp START
        _F3_THEME_Dark_Rose:; [2]
            mov byte ptr COLOR_0, 01DH
            mov byte ptr COLOR_1, 0D1H
            ConPageSet 0,0
            jmp START
        _F3_THEME_Blackboard:; [3]
            mov byte ptr COLOR_0, 037H
            mov byte ptr COLOR_1, 073H
            ConPageSet 0,0
            jmp START
        _F3_THEME_BlackOnWhite:; [4]
            mov byte ptr COLOR_0, 007H
            mov byte ptr COLOR_1, 070H
            ConPageSet 0,0
            jmp START
        _F3_THEME_GreenOnBlack:; [5]
            mov byte ptr COLOR_0, 002H
            mov byte ptr COLOR_1, 020H
            ConPageSet 0,0
            jmp START
    _F4_REBOOT:
        ; Do not use this yo WindowsHostEnrv
        ;mov ax, 0ffffh
        ;push ax
        ;xor ax, ax
        ;push ax
        ;retf
        jmp __getch
    _F5_ABOUT:; Pg-3
        ConPageSet 3,0
        ConRollUp 0,37H,TotalP0C,TotalP0R,TotalP1C,TotalP1R; Clear the screen
        _F5_ABOUT_Prints:
            ConCursorSet 0,1,3
            Puts about_title; {Potential Pgno}
        ConGetc
        ConPageSet 0,0
        jmp __getch
__get_byte:; return yo DL and echo [10 dec]
    __get_byte_lup1:; unit 10
        call __getch
        cmp ah, 44H ; [F10]
            je __get_byte_F10
        sub al, '0'; more to see UNISYM
        cmp al, 9
        ja __get_byte_lup1
    add al, '0'
    ConPrintCharFore al,0,0
    xor ah, ah
    sub al, '0'
    mov dl, 10
    mul dl
    mov dl, al
    __get_byte_lup2:; unit 01
        call __getch
        sub al, '0'
        cmp al, 9
        ja __get_byte_lup2
    add dl, al
    add al, '0'
    ConPrintCharFore al,0,0
    ret
    __get_byte_F10:
        pop ax
        mov RB_ROW, 22
        mov RB_COL, 78
        mov LT_ROW, 02
        mov LT_COL, 03
        jmp proc_04_cls
__dbg_byte:; input DL yo DX [10 dec]
    push ax
    push bx
    mov ax, dx
    xor ah, ah
    mov bx, 10
    div bl
    add ah, '0'
    add al, '0'
    ConPrintCharFore al,0,0
    ConPrintCharFore ah,0,0
    pop bx
    pop ax
    ret
__write_byte_si:; base above, input CL
    push ax
    push bx
    mov ax, cx
    xor ah, ah
    mov bx, 10
    div bl
    add ah, '0'
    add al, '0'
    mov [si], al
    mov [si+1], ah
    pop bx
    pop ax
    ret
__err_ow:
    push cx
    ConCursorGet 0
    sub dl, 2
    ConCursorSet dl,dh,0
    Puts text_ow
    call __getch
    ConCursorGet 0
    ConRollUp 0,COLOR_0,TotalP0C,dh,TotalP1C,dh
    xor dl, dl
    ConCursorSet dl,dh,0
    pop cx
    ret
__new_line:
    ConPrintCharFore 13,0,0
    ConPrintCharFore 10,0,0
    ret
__clear_screen:
    ConRollUp 0,COLOR_0,TotalP0C,TotalP0R,TotalP1C,TotalP1R; Clear the screen
    ConRollUp 0,COLOR_1,TotalP0C,TotalP0R,TotalP1C,TotalP0R; Menu Bar
    ConRollUp 0,COLOR_1,TotalP0C,TotalP1R,TotalP1C,TotalP1R; State Bar Bar
    ConCursorSet 0,TotalP1R,0
    Puts text_state
    ConCursorSet 30,0,0
    Puts LogoMsg
    draw_COL:
        ConCursorSet 80-2-3,0,0
        ConPrintCharFore 'C',0,0
        ConPrintCharFore 'O',0,0
        ConPrintCharFore 'L',0,0
    ret
__new_line_block:
    Save_at_file_buf:
        ConCursorGet 0
        mov cl, dl
        mov ch, LT_COL
        mov si, offset text_buf
        sub cl, ch
        jz _mov_to_buf_out
        xor ch, ch
        push es
            push cx
            CrtLn
            pop cx
            _mov_to_buf:
                mov al, byte ptr es:[bx]
                mov [si], al
                inc si
                add bx, 2
                loop _mov_to_buf
        pop es
        _mov_to_buf_out:
            mov byte ptr [si], 13
            inc si
            mov byte ptr [si], 10
            inc si
            sub si, offset text_buf
            mov word ptr text_ptr, si
            mov cx, si
            DskFileWrite offset(text_buf), cx, FHandle
    push ax
        mov al, LT_COL
        sub al, 2
        mov BYT_TMP, al
        inc al
        mov BY2_TMP, al
    pop ax
    push dx
        ConRollUp 1,COLOR_0,BYT_TMP,LT_ROW,BY2_TMP,RB_ROW; column of lineno
        ConRollUp 1,COLOR_1,LT_COL,LT_ROW,RB_COL,RB_ROW
        ConCursorSet BYT_TMP,RB_ROW,0
        mov dl, LNNUMBER
        add byte ptr LNNUMBER, 1
        cmp dl, 99; now LNNUMBER equals 100
            jae __new_line_block_reset_col
        __new_line_block_reset_col_back:
        call __dbg_byte
    pop dx
            ret
    __new_line_block_reset_col:
        mov LNNUMBER, 00
        jmp __new_line_block_reset_col_back
__init__:
    mov byte ptr LNNUMBER, 1
    ; no for COLOR_0 ..., for change THEME
    cmp word ptr FHandle, 0
        je __init__exit
    DskFileClose FHandle
    mov word ptr FHandle, 0
    __init__exit:
        DskFileCreate OFFSET(FILENAME), 0
        JC __init__exit_next
        mov word ptr FHandle, ax
        DskFileWrite OFFSET(FHeader), FHEADLEN, FHandle
        __init__exit_next:
            ret
__chk_04:
    push es
    CrtLn
    cmp byte ptr es:[bx], 'E'
        ;ConPrintCharFore es:[bx],0,0 {dbg}
        jnz __chk_04_exit1
        add bx, 2
        cmp byte ptr es:[bx], 'X'
            jnz __chk_04_exit1
            add bx, 2
            cmp byte ptr es:[bx], 'I'
                jnz __chk_04_exit1
                add bx, 2
                cmp byte ptr es:[bx], 'T'
                    jnz __chk_04_exit1
                    pop es
                    push es
                    call endo
    __chk_04_exit1:
    pop es
    ret

is_00:; base on _ow
    push cx
    ConCursorGet 0
    sub dl, 2
    ConCursorSet dl,dh,0
    Puts text_00
    ConPrintCharFore '0',0,0
    ConPrintCharFore '0',0,0
    ConGetc
        cmp al, '0'
        jne is_00_quit
        ConPrintCharFore al,0,0
        ; now "000"
        ConGetc
            cmp al, '0'
            jne is_00_quit
            ConPrintCharFore al,0,0
            ; now "0000"
            ConGetc
                cmp al, ','
                jne is_00_quit
                ConPrintCharFore al,0,0
                ; now "0000,"
                ConGetc
                    cmp al, '0'
                    jne is_00_quit
                    ConPrintCharFore al,0,0
                    ; now "0000,0"
                    ConGetc
                        cmp al, '0'
                        jne is_00_quit
                        ConPrintCharFore al,0,0
                        ; now "0000,00"
                        ConGetc
                            cmp al, '0'
                            jne is_00_quit
                            ConPrintCharFore al,0,0
                            ; now "0000,000"
                            ConGetc
                                cmp al, '0'
                                jne is_00_quit
                                ConPrintCharFore al,0,0
                                call endo_real;jmp endo
    is_00_quit:
        ConCursorGet 0
        ConRollUp 0,COLOR_0,TotalP0C,dh,TotalP1C,dh
        xor dl, dl
        ConCursorSet dl,dh,0
        pop cx
        ret

__str_count_trailing_zero:; cx <<< ~ <<< si
    push si
    push ax
    xor cx, cx
    __str_count_trailing_zero_lup:
        mov al, [si]
        or al, al
        jz __str_count_trailing_zero_out
        inc si
        inc cx
        jmp __str_count_trailing_zero_lup
    __str_count_trailing_zero_out:
        or cx, cx
        jz __str_count_trailing_zero_outx
    __str_count_trailing_zero_lupx:
        jmp __str_count_trailing_zero_lupx
    __str_count_trailing_zero_outx:

    pop ax
    pop si
    ret

; ==== ==== ==== ==== ==== ==== ==== ====
CODE ENDS
END START
