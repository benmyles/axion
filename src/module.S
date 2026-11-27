.section .rodata
.macro MODULE family
#if AX_NATIVE
// Page-aligned cubins let the filesystem share identical extents between
// the executable and its independently rebuildable device images.
.balign 4096
#else
.balign 16
#endif
.globl axion_\family
.hidden axion_\family
axion_\family:
#if AX_NATIVE
.incbin "\family\().cubin"
#else
.incbin "\family\().ptx"
.byte 0
#endif
.endm
MODULE genesis
.section .note.GNU-stack,"",@progbits
