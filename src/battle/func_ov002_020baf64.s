/*
 * Battle queue flag bit 1 set (ov002, 0x020BAF64-0x020BAF90).
 * Sets bit 1 of queue->next->field_104 based on condition.
 */

.section .text
.global func_ov002_020baf64
func_ov002_020baf64:
    ldr r0, [r0, #0xc8]
    cmp r1, #0
    ldr r2, [r0, #4]
    movne r0, #1
    ldr r1, [r2, #0x104]
    moveq r0, #0
    and r0, r0, #1
    bic r1, r1, #2
    orr r0, r1, r0, lsl #1
    str r0, [r2, #0x104]
    bx lr