void remove_slashes_from_line(char* line) {
	i32 i = 0;
	while(line[i] != '\0' && line[i] != '\n') {
		if(line[i] == '/') line[i] = ' ';
		i++;
	}
}

// Returns false if there are no more words after this one.
bool consume_word(char* word, char* line, i32* line_i) {
	if(line[*line_i] == '\0' || line[*line_i] == '\n') {
		return false;
	}

	i32 word_i = 0;
	while(line[*line_i] != ' ' && line[*line_i] != '\0' && line[*line_i] != '\n') {
		word[word_i] = line[*line_i];
		*line_i += 1;
		word_i++;
	}
	while(line[*line_i] == ' ' && line[*line_i] != '\0' && line[*line_i] != '\n') {
		*line_i += 1;
	}
	word[word_i] = '\0';

	return true;
}

void consume_f32(f32* f, char* line, i32* line_i) {
	char word[64];
	consume_word(word, line, line_i);
	*f = (f32)atof(word);
}

void consume_i32(i32* v, char* line, i32* line_i) {
	char word[64];
	consume_word(word, line, line_i);
	*v = (i32)atoi(word);
}
