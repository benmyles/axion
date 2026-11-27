.section .rodata
.globl axion_journal_script
.hidden axion_journal_script
axion_journal_script:
.incbin "tools/genesis_journal.py"
.byte 10
.incbin "tools/journal.py"
.byte 0
.section .note.GNU-stack,"",@progbits
