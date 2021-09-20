020A:07B9   sub_1        proc    near
020A:07B9  1E               push    ds
020A:07BA  50               push    ax
; ds = ax = 0040h
020A:07BB  B8 0040          mov     ax,40h
020A:07BE  8E D8            mov     ds,ax
; Сохранение младнего байта регистра флагов в ah
020A:07C0  9F               lahf               ; Load ah from flags
020A:07C1  F7 06 0314 2400  test    word ptr ds:data_2e,2400h   ; (0040:0314=3200h)
020A:07C7  75 0C            jnz     loc_7      ; Jump if not zero
; Сброс флага IF через зануления 9го бита

; (0040:0314=3200h)
020A:07C9  F0> 81 26 0314 FDFF  lock    and    word ptr ds:data_2e,0FDFFh
; Установка младшему байту регистра флагов значения ah
020A:07D0            loc_6:                        ;  xref 020A:07D6
020A:07D0  9E               sahf               ; Store ah into flags
020A:07D1  58               pop     ax
020A:07D2  1F               pop     ds
020A:07D3  EB 03            jmp     short loc_ret_8     ; (07D8)
020A:07D5            loc_7:                        ;  xref 020A:07C7
; Сброс флага IF
020A:07D5  FA               cli                ; Disable interrupts
020A:07D6  EB F8            jmp     short loc_6        ; (07D0)

020A:07D8            loc_ret_8:                    ;  xref 020A:07D3
020A:07D8  C3               retn
            sub_1        endp
