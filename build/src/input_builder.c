#include "input.c"
#include "GL/glx.h"

typedef enum {
	TYPE_READER_SCAN,
	TYPE_READER_STORE
} TypeReaderState;

void build_default_input_config_file(char* input_header_file, char* button_type_enum_identifier, char* out_input_config_filename, bool force_rebuild) {
	bool config_already_exists = false;
	u32 existing_config_buttons_len = 0;

	if(!force_rebuild) {
		FILE* existing_file = fopen(out_input_config_filename, "r");
		if(existing_file != NULL) {
			printf("Input configuration file '%s' exists.\nVerifying input buttons length is the same as found in '%s'...\n", out_input_config_filename, input_header_file);
			config_already_exists = true;

			u32 existing_mappings_len;
			fread(&existing_mappings_len, sizeof(existing_mappings_len), 1, existing_file);
			for(i32 i = 0; i < existing_mappings_len; i++) {
				GameKeyMapping mapping;
				fread(&mapping, sizeof(GameKeyMapping), 1, existing_file);
				if(mapping.button_type > existing_config_buttons_len) {
					existing_config_buttons_len = mapping.button_type;
				}
			}
			existing_config_buttons_len++;
		}
	}

	// Get the list of buttons from the input.h file.
	FILE* input_file = fopen(input_header_file, "r");
	assert(input_file != NULL);

	TypeReaderState type_reader_state = TYPE_READER_SCAN;
	char button_names[512][128];
	u32 buttons_len = 0;
	char line[4096];
	while(fgets(line, sizeof(line), input_file)) {
		if(type_reader_state == TYPE_READER_SCAN) {
			char* typedef_cmp = "typedef enum";
			if(strncmp(line, typedef_cmp, strlen(typedef_cmp)) == 0) {
				type_reader_state = TYPE_READER_STORE;
			}
		} else {
			i32 line_i = 0;
			while(line[line_i] == '\t' || line[line_i] == ' ') {
				line_i++;
			}
			if(line[line_i] == '}') {
				if(strncmp(&line[line_i + 2], button_type_enum_identifier, strlen(button_type_enum_identifier)) != 0) {
					type_reader_state = TYPE_READER_SCAN;
					printf("Another 'typedef enum' was found in the input builder before the button types. Remove this printf and panic and see if it still works!\n");
					panic();
					continue;
				}
				// To ignore the NUM_BUTTONS item.
				buttons_len--;
				break;
			}
			i32 name_i = 0;
			while(line[line_i] != ',' && line[line_i] != '\0' && line[line_i] != '\n') {
				button_names[buttons_len][name_i] = line[line_i];
				line_i++;
				name_i++;
			}
			buttons_len++;
		}
	}
	fclose(input_file);

	if(config_already_exists) {
	   if(existing_config_buttons_len == buttons_len) {
			printf("Input button lengths (%u) match! Not there could still be a different order or set of buttons which would cause an incorrect input mapping.\n", buttons_len);
		   	return;
	   } else {
		   printf("Input button lengths do not match! Existing config: %u, Current: %u\n", existing_config_buttons_len, buttons_len);
	   }
	}

	// Open X11 window.
	Display* display = XOpenDisplay("");
	i32 screen = DefaultScreen(display);
	unsigned long value_mask = CWBackPixel | CWEventMask;
	XSetWindowAttributes attribs;
	attribs.background_pixel = WhitePixel(display, screen);
	attribs.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	Window window = XCreateWindow(display, RootWindow(display, screen),
		100, 100, 1, 1, 2,
		DefaultDepth(display, screen), InputOutput,
		DefaultVisual(display, screen),
		value_mask, &attribs);
	XMapWindow(display, window);

	GameKeyMapping key_mappings[256];
	u32 key_mappings_len = 0;

	printf("Press a key when prompted to capture its keysym as the default input for the given control.\n");
	for(i32 player = 0; player < 2; player++) {
		i32 button_to_capture = 0;
		for(i32 button_to_capture = 0; button_to_capture < buttons_len; button_to_capture++) {
			printf("Player %i, %s...", player, button_names[button_to_capture]);
			fflush(stdout);
			while(true) {
				while(XPending(display)) {
					XEvent event;
					XNextEvent(display, &event);
					switch(event.type) {
						case KeyPress: {
							unsigned long keysym = XLookupKeysym(&(event.xkey), 0);
							key_mappings[key_mappings_len] = (GameKeyMapping){ .key_id = keysym, .player_index = player, .button_type = button_to_capture };
							key_mappings_len++;
							printf("%x\n", keysym);
							goto button_captured;
						}
						default: break;
					}
				}
				usleep(100000);
			}
button_captured:
		}
	}

	FILE* file = fopen(out_input_config_filename, "w");
	assert(file != NULL);

	fwrite(&key_mappings_len, sizeof(u32), 1, file);
	fwrite(&key_mappings, sizeof(GameKeyMapping) * key_mappings_len, 1, file);

	fclose(file);
}
