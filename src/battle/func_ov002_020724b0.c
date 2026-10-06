/*
 * Battle entity field initializer (ov002, 0x020724b0-0x020724c8).
 * Initializes four fields of an entity struct from register and stack arguments.
 */

#include <nitro.h>

void func_ov002_020724b0(void *entity, unsigned int a, unsigned short b, unsigned int c, unsigned int stack_arg)
{
    unsigned int *fields = (unsigned int *)entity;
    fields[0] = a;
    ((unsigned short *)fields)[2] = b;
    fields[2] = c;
    fields[3] = stack_arg;
}