/* NitroSDK Thumb interrupt-wait supervisor-call wrappers. */

.syntax unified
.arch armv4t
.thumb
.section .pit_SVC_WaitInterrupt, "ax", %progbits
.balign 2

.global SVC_WaitIntr
.type SVC_WaitIntr, %function
.thumb_func
SVC_WaitIntr:
    ldr r2, .L_register_base
    mov r12, r2
    movs r2, #0
    svc #4
    bx lr
    .short 0x0000
.L_register_base:
    .word 0x04000000
.size SVC_WaitIntr, . - SVC_WaitIntr

.global SVC_WaitVBlankIntr
.type SVC_WaitVBlankIntr, %function
.thumb_func
SVC_WaitVBlankIntr:
    movs r2, #0
    svc #5
    bx lr
.size SVC_WaitVBlankIntr, . - SVC_WaitVBlankIntr
