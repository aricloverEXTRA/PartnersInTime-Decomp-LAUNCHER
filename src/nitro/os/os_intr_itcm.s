; ARM9 ITCM interrupt dispatcher for the IF2/IE2 interrupt set.
; Native range 0x01FF8000..0x01FF8058 (88 bytes).
;
; WHY THIS UNIT IS ASSEMBLY AND NOT C
; -----------------------------------
; This routine was hand-written assembly in the original NITROSDK and cannot be
; reproduced from C by the MWCC 1.2 toolchain in tools/mwccarm/1.2:
;
;   * The dispatch loop is built on CLZ. MWCC 1.2 has no inline CLZ intrinsic;
;     __builtin_clz lowers to an out-of-line library call instead of the
;     instruction.
;   * MWCC's inline __asm {} dialect has no operand binding, so register values
;     produced inside an asm block are invisible to surrounding C, and it rejects
;     ".word <symbol>" in a literal pool, which this function needs.
;
; Both were verified against the compiler rather than assumed. The standalone
; assembler (mwasmarm, same MWCC 1.2 toolchain) reproduces the original bytes
; exactly, including the R_ARM_ABS32 literal-pool relocations.
;
; Per docs/PROGRESS.md this range is counted as ASM, NOT as C/C++; it must not
; be folded into the C/C++ coverage percentage.
;
; Register map, recovered from the native bytes:
;   REG_IME  0x04000208  master interrupt enable
;   REG_IF   0x04000210  interrupt flags
;   REG_IE2  0x04000214  IF2 enable; writing a bit acknowledges that IF2 flag
;   r12      REG_IF base, used with a +4 offset to reach REG_IE2
;   r3       0x80000000, the bit-mask generator for the scan
;
; The scan extracts the HIGHEST set bit of (REG_IF & REG_IE2), acknowledges it by
; writing it back to REG_IE2, and dispatches through OS_IRQTable.
;
; func_01ff8058 is the fallback entry installed in OS_IRQTable; it is still
; provided by the original object and is pending reconstruction under its
; current address-derived name.

        .section .text
        .arm
        .align 2

        .extern  OS_IRQTable
        .extern  func_01ff8058

        .global OS_IntrMain
        .global func_01ff8000
OS_IntrMain:
func_01ff8000:
        stmdb   sp!, {lr}
        mov     r12, #0x04000000
        add     r12, r12, #0x210
        ldr     r1, [r12, #-8]
        cmp     r1, #0
        ldmeqfd sp!, {pc}
        ldmfd   r12, {r1, r2}
        ands    r1, r1, r2
        ldmeqfd sp!, {pc}
        mov     r3, #0x80000000
scan_:
        clz     r0, r1
        bics    r1, r1, r3, lsr r0
        bne     scan_
        mov     r1, r3, lsr r0
        str     r1, [r12, #4]
        rsbs    r0, r0, #31
        ldr     r1, [pc, #8]
        ldr     r0, [r1, r0, lsl #2]
        ldr     lr, [pc, #4]
        bx      r0
        .word   OS_IRQTable
        .word   func_01ff8058
