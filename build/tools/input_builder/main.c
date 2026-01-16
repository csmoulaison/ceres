#define NUM_MAPPINGS 14

#include "core/core.h"
#include "input.c"
#include "GL/glx.h"

typedef enum {
	TYPE_READER_SCAN,
	TYPE_READER_STORE
} TypeReaderState;

int main(int argc, char** argv) {
	if(argc < 4) {
		printf("Usage: input_builder <input.h> <ButtonTypeEnumIdentifier> <data.conf> <flags...>\nFlags: -f to force input build\nSee main.c for more info.\n");
		return 1;
	}

	bool config_already_exists = false;
	unsigned int existing_config_buttons_len = 0;

	if(argc < 5 || strcmp(argv[4], "-f") != 0) {
		FILE* existing_file = fopen(argv[3], "r");
		if(existing_file != NULL) {
			printf("Input configuration file '%s' exists.\nVerifying input buttons length is the same as found in '%s'...\n", argv[3], argv[1]);
			config_already_exists = true;

			unsigned int existing_mappings_len;
			fread(&existing_mappings_len, sizeof(existing_mappings_len), 1, existing_file);
			for(int i = 0; i < existing_mappings_len; i++) {
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
	FILE* input_file = fopen(argv[1], "r");
	assert(input_file != NULL);

	TypeReaderState type_reader_state = TYPE_READER_SCAN;
	char button_names[512][128];
	unsigned int buttons_len = 0;
	char line[4096];
	while(fgets(line, sizeof(line), input_file)) {
		if(type_reader_state == TYPE_READER_SCAN) {
			char* typedef_cmp = "typedef enum";
			if(strncmp(line, typedef_cmp, strlen(typedef_cmp)) == 0) {
				type_reader_state = TYPE_READER_STORE;
			}
		} else {
			int line_i = 0;
			while(line[line_i] == '\t' || line[line_i] == ' ') {
				line_i++;
			}
			if(line[line_i] == '}') {
				if(strncmp(&line[line_i + 2], argv[2], strlen(argv[2])) != 0) {
					type_reader_state = TYPE_READER_SCAN;
					printf("wat??\n");
					assert(false);
					continue;
				}
				// To ignore the NUM_BUTTONS item.
				buttons_len--;
				break;
			}
			int name_i = 0;
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
		   	return 0;
	   } else {
		   printf("Input button lengths do not match! Existing config: %u, Current: %u\n", existing_config_buttons_len, buttons_len);
	   }
	}

	// Open X11 window.
	Display* display = XOpenDisplay("");
	int screen = DefaultScreen(display);
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
	unsigned int key_mappings_len = 0;

	printf("Press a key when prompted to capture its keysym as the default input for the given control.\n");
	for(int player = 0; player < 2; player++) {
		int button_to_capture = 0;
		for(int button_to_capture = 0; button_to_capture < buttons_len; button_to_capture++) {
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

	FILE* file = fopen(argv[3], "w");
	assert(file != NULL);

	fwrite(&key_mappings_len, sizeof(unsigned int), 1, file);
	fwrite(&key_mappings, sizeof(GameKeyMapping) * key_mappings_len, 1, file);

	fclose(file);
}
