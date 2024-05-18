#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

struct settings {
	const char* type;
	bool comments;
	long long initSize;
	long long extendSize;
	bool optimize;
};

void writeStart(FILE* output, struct settings settings) {
	fprintf(output, "#include <stdlib.h>\n");
	fprintf(output, "#include <stdio.h>\n");
	fprintf(output, "#include <errno.h>\n");
	fprintf(output, "#include <string.h>\n\n");
	fprintf(output, "#define INITIAL_MEM_SIZE (%lld)\n", settings.initSize);
	fprintf(output, "#define MEM_INC_SIZE (%lld)\n\n", settings.extendSize);
	fprintf(output, "size_t current_size = INITIAL_MEM_SIZE;\n");
	fprintf(output, "%s* mem;\n", settings.type);
	fprintf(output, "long long ptr = 0;\n\n");
	fprintf(output, "void mem_fail() {\n");
	fprintf(output, "\tfprintf(stderr, \"error while allocating memory: %%s\\n\", strerror(errno));\n");
	fprintf(output, "\texit(1);");
	fprintf(output, "}\n\n");
	fprintf(output, "void move(int i) {\n");
	fprintf(output, "\tlong long ptr_new = ptr + i;\n");
	fprintf(output, "\twhile (ptr_new < 0) {\n");
	fprintf(output, "\t\tsize_t new_size = current_size + MEM_INC_SIZE;\n");
	fprintf(output, "\t\tmem = realloc(mem, new_size * sizeof(%s));\n", settings.type);
	fprintf(output, "\t\tif (mem == NULL)\n");
	fprintf(output, "\t\t\tmem_fail();\n");
	fprintf(output, "\t\tmemmove(mem + MEM_INC_SIZE, mem, current_size);\n");
	fprintf(output, "\t\tmemset(mem, 0, MEM_INC_SIZE * sizeof(%s));\n", settings.type);
	fprintf(output, "\t\tcurrent_size = new_size;\n");
	fprintf(output, "\t\tptr_new += MEM_INC_SIZE;\n");
	fprintf(output, "\t}\n");
	fprintf(output, "\twhile (ptr_new >= current_size) {\n");	
	fprintf(output, "\t\tsize_t new_size = current_size + MEM_INC_SIZE;\n");
	fprintf(output, "\t\tmem = realloc(mem, new_size * sizeof(%s));\n", settings.type);
	fprintf(output, "\t\tif (mem == NULL)\n");
	fprintf(output, "\t\t\tmem_fail();\n");
	fprintf(output, "\t\tmemset(mem + current_size, 0, MEM_INC_SIZE * sizeof(%s));\n", settings.type);
	fprintf(output, "\t\tcurrent_size = new_size;\n");
	fprintf(output, "\t}\n");
	fprintf(output, "\tptr = ptr_new;\n");
	fprintf(output, "}\n\n");
	fprintf(output, "void init() {\n");
	fprintf(output, "\tmem = malloc(current_size * sizeof(%s));\n", settings.type);
	fprintf(output, "\tif (mem == NULL)\n");
	fprintf(output, "\t\tmem_fail();\n");
	fprintf(output, "\tmemset(mem, 0, current_size * sizeof(%s));\n", settings.type);
	fprintf(output, "}\n\n");
	fprintf(output, "int main() {\n");
	fprintf(output, "\tinit();\n\n");
}

void writeTimes(FILE* output, const char* string, unsigned int times) {
	for (unsigned int i = 0; i < times; i++) {
		fprintf(output, "%s", string);
	}
}

void writeTimesChar(FILE* output, char c, unsigned int times) {
	for (unsigned int i = 0; i < times; i++) {
		putc(c, output);
	}
}


void writeEnd(FILE* output) {
	fprintf(output, "\n\tfree(mem);\n");
	fprintf(output, "}\n");
} 

void writeChange(FILE* output, int indent, int value) {
	writeTimes(output, "\t", indent + 1);
	fprintf(output, "*(mem + ptr) += %d;\n", value);
}

void writeMove(FILE* output, int indent, int value) {
	writeTimes(output, "\t", indent + 1);
	fprintf(output, "move(%d);\n", value);
}

void writeGet(FILE* output, int indent) {
	writeTimes(output, "\t", indent + 1);
	fprintf(output, "*(mem + ptr) = getchar();\n");
}

void writePut(FILE* output, int indent) {
	writeTimes(output, "\t", indent + 1);
	fprintf(output, "putchar(*(mem + ptr));\n");
}

void writeLoopBegin(FILE* output, int indent) {
	writeTimes(output, "\t", indent + 1);
	fprintf(output, "while(*(mem + ptr)) {\n");
}

void writeLoopEnd(FILE* output, int indent) {
	writeTimes(output, "\t", indent + 1);
	fprintf(output, "}\n");
}

struct parseParams {
	FILE* output;

	int indent;

	bool isComment;

	char rowChar;
	int rowNumber;
	void (*rowFunction)(FILE*, int, int);
	bool rowNeg;
};

void flushComment(struct parseParams* params);
void flushRow(struct parseParams* params);
void addRow(char c, struct settings* settings, struct parseParams* params, void (*f)(FILE*, int, int), bool neg);

inline void flushComment(struct parseParams* params) {
	if (params->isComment) {
		fprintf(params->output, "\n");
		params->isComment = false;
	}
}

inline void flushRow(struct parseParams* params) {
	if (params->rowChar == 0)
		return;

	params->rowFunction(params->output, params->indent, params->rowNumber * (params->rowNeg ? -1 : 1));
	params->rowNumber = 0;
	params->rowChar = 0;
}

inline void addRow(char c, struct settings* settings, struct parseParams* params, void (*f)(FILE*, int, int), bool neg) {
	if (!settings->optimize) {
		f(params->output, params->indent, neg ? -1 : 1);
	} else {
		if (params->rowChar == c) {
			params->rowNumber++;
		} else {
			flushRow(params);

			params->rowChar = c;
			params->rowNumber = 1;
			params->rowFunction = f;
			params->rowNeg = neg;
		}
	}
}

void handleInputChar(char c, struct settings* settings, struct parseParams* params) {

	switch(c) {
		case '+':
			flushComment(params);

			addRow(c, settings, params, &writeChange, false);

			break;
		case '-':
			flushComment(params);

			addRow(c, settings, params, &writeChange, true);

			break;
		case '>':
			flushComment(params);

			addRow(c, settings, params, &writeMove, false);

			break;
		case '<':
			flushComment(params);

			addRow(c, settings, params, &writeMove, true);

			break;
		case '.':
			flushComment(params);
			flushRow(params);

			writePut(params->output, params->indent);
			break;
		case ',':
			flushComment(params);
			flushRow(params);

			writeGet(params->output, params->indent);
			break;
		case '[':
			flushComment(params);
			flushRow(params);

			writeLoopBegin(params->output, params->indent);

			params->indent++;
			break;
		case ']':
			flushComment(params);
			flushRow(params);

			params->indent--;

			writeLoopEnd(params->output, params->indent);
			break;
		default:
			if (!settings->comments)
				break;

			if (c == '\t' || c == ' ' || c == '\n' || c == '\r') {
				if (!params->isComment) {
					break;
				}
				
				putc(c, params->output);

				if (c == '\n') {
					writeTimes(params->output, "\t", params->indent + 1);
					fprintf(params->output, "// ");
				}
			} else {
				if (!params->isComment) {					
					flushRow(params);

					params->isComment = true;
					writeTimes(params->output, "\t", params->indent + 1);
					fprintf(params->output, "// ");
				}
				
				putc(c, params->output);
			}
			break;
	}
}

int main(int argc, char** argv) {
	FILE* input;
	FILE* output;

	struct settings settings;
	settings.type = "char";
	settings.comments = false;
	settings.initSize = (1 << 10);
	settings.extendSize = (1 << 10);
	settings.optimize = false;

	char* endptr;

	int opt;
	while((opt = getopt(argc, argv, "i:e:ct:o")) != -1) {
		switch(opt) {
			case 'i':
				settings.initSize = strtoll(optarg, &endptr, 10);
				if (*endptr != '\0') {
					fprintf(stderr, "-i argument needs to be a number\n");
					return 1;
				}
				break;
			case 'e':
				settings.extendSize = strtoll(optarg, &endptr, 10);
				if (*endptr != '\0') {
					fprintf(stderr, "-e argument needs to be a number\n");
					return 1;
				}
				break;
			case 'c':
				settings.comments = true;
				break;
			case 't':
				settings.type = optarg;
				break;
			case 'o':
				settings.optimize = true;
				break;
			default:
				break;
		}
	}

	if (optind > argc - 2) {
		fprintf(stderr, "Missing file arguments.\n");
		return 1;
	}

	input = fopen(argv[optind], "r");
	if (input == NULL) {
		fprintf(stderr, "error opening input file: %s\n", strerror(errno));
		return 1;
	}

	output = fopen(argv[optind + 1], "w");
	if (output == NULL) {
		fprintf(stderr, "error opening output file: %s\n", strerror(errno));
		return 1;
	}

	writeStart(output, settings);

	struct parseParams params;
	params.output = output;
	params.indent = 0;
	params.isComment = false;
	params.rowChar = 0;
	params.rowNumber = 0;

	int c;
	while((c = fgetc(input)) != EOF) {
		handleInputChar(c, &settings, &params);
	}

	flushRow(&params);

	writeEnd(output);

	return 0;
}
