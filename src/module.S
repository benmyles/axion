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
MODULE foraging
MODULE lessons
MODULE routes
MODULE drawing
MODULE life
MODULE assays
MODULE actors
MODULE memories
MODULE outcomes
MODULE appraisals
MODULE foodactors
MODULE trails
MODULE travel
MODULE mazes
MODULE springbeds
MODULE mapactors
MODULE scenes
MODULE dialogue
MODULE vocal_brood
MODULE recollection
MODULE causes
MODULE development
MODULE composition
MODULE procedure
MODULE grounding
MODULE floodplain
MODULE siphons
MODULE runnels
MODULE mind
MODULE genesis
MODULE sequence_field
MODULE sequence_actors
MODULE sequence_routes
MODULE sequence_mazes
MODULE sequence_springs
MODULE sequence_appraisals
MODULE junctions
.section .note.GNU-stack,"",@progbits
