#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COMMAND_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEY_COUNT 128

#define WHITE_ON_BLACK 0x0F

#define KEYBOARD_IRQ 33

#define BUFFER_SIZE 128


char newline[] = "\n";

char input_buffer[BUFFER_SIZE];
int input_index=0;

uint16_t* vga_buffer = (uint16_t*)VGA_ADDRESS;
int cursor_x = 0;
int cursor_y = 0;
int quit=0;

static inline uint8_t inb(uint16_t port) {
	uint8_t result;
	__asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
	return result;
}

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// utility functions
char command[];
/*
void sliceCommand(char* str[], char strlen) {
	char substr[];
	int start=0
	bool addToSubstr=false;
	int commandCounter=0
	for (int i=0; i<strlen; i++) {
		if (str[i]!=' ') {
			if (str[i-1]==' ' || str[i-1]==NULL) {
				start=i;
				addToSubstr=true;
			}
			if (addToSubstr && (str[i]==' ' || str[i]==NULL) {
				addToSubstr=false;
				substr={};
				command[commandCounter]=substr
			}
			if (addToSubstr) {
				substr[i-start]=str[i];
			}
		}
	}
}
*/
int len(char str[]) {
	int counter=0;
	bool done=false;
	while (!done) {
		if (str[counter]!=NULL) {
			counter++;
		} else {
			done=true;
		}
	}
	return counter;
}

// screen management
void clear_screen() {
	for (size_t y=0; y<VGA_HEIGHT; y++) {
		for (size_t x=0; x<VGA_WIDTH; x++) {
			const size_t index = y*VGA_WIDTH+x;
			vga_buffer[index] = (uint16_t)(' ' | (0x07 << 8));
		}
	}
}
void set_cursor_position(uint16_t position) {
	outb(VGA_COMMAND_PORT, 0x0F);
	outb(VGA_DATA_PORT, (uint16_t)(position & 0xFF));
	outb(VGA_COMMAND_PORT, 0x0E);
	outb(VGA_DATA_PORT, (uint16_t)((position >> 8) & 0xFF));
}
void update_cursor() {
	set_cursor_position(cursor_y*80+cursor_x);
}

// text and terminal
void terminal_putchar(char c) {
	bool backsp=false;
	if (c == '\n') {
		cursor_x = 0;
		if (cursor_y>=VGA_HEIGHT) {
			cursor_y=0;
			clear_screen();
		} else {
			cursor_y++;
		}
	} else {
		if (c=='\b') {
			cursor_x--;
			c=' ';
			backsp=true;
		}
		uint16_t color = WHITE_ON_BLACK << 8 | c;
		vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = color;
		backsp ? cursor_x : cursor_x++;
		if (cursor_x == VGA_WIDTH) {
			cursor_x = 0;
			cursor_y++;
		}
	}
	if (cursor_y == VGA_HEIGHT) {
		cursor_y = 0;
	}
}

void write(const char* str) {
	while (*str) {
		terminal_putchar(*str++);
	}
}


void appendToInputBuffer(char newChar) {
	for (int i=0; i<BUFFER_SIZE; i++) {
		if (input_buffer[i]==NULL || input_buffer[i]=='\0') {
			input_buffer[i]=newChar;
			break;
		}
	}
}

void clearBuffer() {
	for (int i=0; i < BUFFER_SIZE; i++) {
		input_buffer[i]='\0';
	}
}

bool inputBufferContains(const char* str, int strlen) {
	bool match=false;
	for (int i=0; i<BUFFER_SIZE; i++) {
		if (input_buffer[i] == str[0] && (input_buffer[i-1] == ' ' || input_buffer[i-1] == NULL)) {
			match=true;
			for (int j=0; j<strlen; j++) {
				if (input_buffer[i+j]!=str[j]) {
					match=false;
				}
			}
			if (match) {
				break;
			}
		}
	}
	return match;
}

void terminal_get_input() {
	if (inputBufferContains("hello",5)) {
		write("Hi :D\n");
	} else if (inputBufferContains("exit",4)) {
		quit=1;
	} else if (inputBufferContains("info",4)) {
		write("Terminal from scratch, written in C and assembly\n");
		write("Author: Sam Poyner\n");
		write("Commands: hello exit info\n");
	} else if (inputBufferContains("cls",3)) {
		cursor_x=0;
		cursor_y=0;
		clear_screen();
	}
	//write(input_buffer);
	clearBuffer();
	write("$ ");
}

static char keymap[KEY_COUNT] = {
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',  // Backspace
	'\t', // Tab
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',          // Enter
	0,    // Control
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0,    // Left Shift
	'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,                 // Right Shift
	'*',
	0,    // Alt
	' ',  // Spacebar
	0,    // Caps Lock
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                           // F1-F10
	0,    // Num Lock
	0,    // Scroll Lock
	0,    // Home
	0,    // Up Arrow
	0,    // Page Up
	'-',
	0,    // Left Arrow
	0,
	0,    // Right Arrow
	'+',
	0,    // End
	0,    // Down Arrow
	0,    // Page Down
	0,    // Insert
	0,    // Delete
	0, 0, 0, 0,                          // F11-F12
	0,                                    // Undefined
};
bool shift_pressed = false;

char scancode_to_char(unsigned char scancode) {
	static char keymap[] = {
		[0x1C] = '\n', // Enter
		[0x0E] = '\n', //Backspace
		[0x02] = '1'
	};
	return keymap[scancode];
}

bool key_state[KEY_COUNT] = { false };

void keyboard_handler() {
	uint8_t scancode=inb(KEYBOARD_DATA_PORT);
	
	
	// Check for shift keys
	if (scancode == 0x2A || scancode == 0x36) {
		shift_pressed = true;
		return;
	} else if (scancode == 0xAA || scancode == 0xB6) {
		shift_pressed=false;
		return;
	}
	
	// only take the make codes aka keydowns (scancode < 0x80)
	
	if (scancode < 0x80) {
		if (!key_state[scancode]) {
			key_state[scancode] = true; // mark as pressed
			char c = shift_pressed ? keymap[scancode] - 32 : keymap[scancode];
			if (scancode == 0x39 || scancode == 0x0e) {
				c=keymap[scancode];
			}
			if (c) {
				terminal_putchar(c);
				appendToInputBuffer(c);
				// terminal_putchar(input_buffer[0]);
			}
			if (c=='\n') {
				terminal_get_input();
				clearBuffer();
			}/*
			write("(");
			write(input_buffer);
			write(")");*/
		}
	} else {
		uint8_t make_code = scancode - 0x80;
		key_state[make_code] = false; // mark as released
	}
}


void kernel_main() {
	// TODO : backspace adds an \b to the input buffer. Fix.
	//        Likely solution: handle backspacing in the keyboard manager code.
	clearBuffer();
	write("Welcome to Sam P's Kernel!");
	write(newline);
	write("Don't forget to say hello!");
	// TODO: Map regular shift+num and shift+symbol scancodes
	write(newline);
	write("Available commands: hello info exit");
	write(newline);
	write(newline);
	write("$ ");
	/*
	terminal_get_input();
	write("You typed: ");
	write(input_buffer);
	write(newline);
	*/
	while (quit==0) {
		keyboard_handler();
		update_cursor();
	}
	clear_screen();
	cursor_y=0;
	cursor_x=0;
	update_cursor();
	write("Process completed");
}
