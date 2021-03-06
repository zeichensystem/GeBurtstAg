@--------------------------------------------------------------------------------
@ udiv.s
@--------------------------------------------------------------------------------
@ Provides an implementation of unsigned division
https://github.com/JoaoBaptMG/gba-modern/blob/master/source/math/udiv32.s
@--------------------------------------------------------------------------------

@ Source code taken from https://www.chiark.greenend.org.uk/~theom/riscos/docs/ultimate/a252div.txt
@ r0: the numerator / r1: the denominator
@ after it, r0 has the quotient and r1 has the modulo
    .section .iwram, "ax", %progbits
    .align 2
    .arm
    .global __aeabi_uidivmod
    .type __aeabi_uidivmod STT_FUNC
__aeabi_uidivmod:

    .section .iwram, "ax", %progbits
    .align 2
    .arm
    .global __aeabi_uidiv
    .type __aeabi_uidiv STT_FUNC
__aeabi_uidiv:

    @ Check for division by zero
    cmp     r1, #0
    beq     __aeabi_idiv0

    .global udiv32pastzero
udiv32pastzero:
    @ If n < d, just bail out as well
    cmp     r0, r1      @ n, d
    movlo   r1, r0    @ mod = n
    movlo   r0, #0    @ quot = 0
    bxlo    lr

    @ Move the denominator to r2 and start to build a counter that
    @ counts the difference on the number of bits on each numerator
    @ and denominator
    @ From now on: r0 = quot/num, r1 = mod, r2 = denom, r3 = counter
    mov     r2, r1
    mov     r3, #28             @ first guess on difference
    mov     r1, r0, lsr #4      @ r1 = num >> 4

    @ Iterate three times to get the counter up to 4-bit precision
    cmp     r2, r1, lsr #12
    suble   r3, r3, #16
    movle   r1, r1, lsr #16

    cmp     r2, r1, lsr #4
    suble   r3, r3, #8
    movle   r1, r1, lsr #8

    cmp     r2, r1
    suble   r3, r3, #4
    movle   r1, r1, lsr #4

    @ shift the numerator by the counter and flip the sign of the denom
    mov     r0, r0, lsl r3
    adds    r0, r0, r0
    rsb     r2, r2, #0

    @ dynamically jump to the exact copy of the iteration
    add     r3, r3, r3, lsl #1      @ counter *= 3
    add     pc, pc, r3, lsl #2      @ jump
    mov     r0, r0                  @ pipelining issues

    @ here, r0 = num << (r3 + 1), r1 = num >> (32-r3), r2 = -denom
    @ now, the real iteration part
    .global divIteration
divIteration:
    .rept 32
    adcs    r1, r2, r1, lsl #1
    sublo   r1, r1, r2
    adcs    r0, r0, r0
    .endr

    @ and then finally quit
    @ r0 = quotient, r1 = remainder
    bx      lr

@ r0: the number to get the reciprocal of
@ after it, r0 = (1 << 32)/value
    .section .iwram, "ax", %progbits
    .align 2
    .arm
    .global ureciprocal32
    .type ureciprocal32 STT_FUNC
ureciprocal32:
    @ check if r0 is 0 or 1, because it will overflow or divide by 0
    cmp     r0, #1
    bxls    lr

    @ move r0 = 0, r1 = 1 (so r0:r1 = 1 << 32) and r2 = -x
    rsb     r2, r0, #0
    mov     r1, #1
    movs    r0, r1, lsr #32 @ so r0 = 0, and clear the carry flag at the same time
    b       divIteration    @ the iteration is the same as for an unsigned division