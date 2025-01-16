
ENTRY(_start)

SECTIONS
{
    .multiboot : {
	KEEP(*(.multiboot))
    }

    . = 0x100000;

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss COMMON)
    }

    . = ALIGN(0x1000);
    kernel_end = .;

    _kernel_start = 0x10001000;
}
