/*
 * Battle virtual method thunk (ov002, 0x0206A7A4-0x0206A7B8).
 * Tail-call dispatcher: reads method from object+0x130, adjusts this by +0x1c.
 */

.section .text
.global func_ov002_0206a7a4
func_ov002_0206a7a4:
    ldr r0, [r0, #0x130]
    ldr ip, [pc, #4]
    add r0, r0, #0x1c
    bx ip
    .word 0x02067264