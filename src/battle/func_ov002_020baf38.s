/*
 * Battle queue flag bit 0 set (ov002, 0x020BAF38-0x020BAF64).
 * Sets bit 0 of queue->next->field_104 based on condition.
 */

.section .text
.global func_ov002_020baf38
func_ov002_020baf38:
    ldr r0, [r0, #0xc8]
    cmp r1, #0
    ldr r2, [r0, #4]
    movne r0, #1
    ldr r1, [r2, #0x104]
    moveq r0, #0
    and r0, r0, #1
    bic r1, r1, #1
    orr r0, r1, r0
    str r0, [r2, #0x104]
    bx lr