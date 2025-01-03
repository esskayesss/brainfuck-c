#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define SUCCESS   0
#define FAILURE   1

#define DUMP_HEX  0
#define DUMP_ASC  1
#define DUMP_NON  2


#define RED   "\x1B[41m\x1B[30m"
#define YLW   "\x1B[43m\x1B[30m"
#define BLU   "\x1B[48;5;12m\x1B[30m"
#define GRAY  "\x1B[48;5;8m\x1B[37m"
#define RESET "\x1B[0m"

#define handle_err(...) do { \
  fprintf(stderr, RED " ERROR " RESET " " __VA_ARGS__); \
  exit(1);\
} while(0)

#define log_err(...) do { \
  fprintf(stderr, RED " ERROR " RESET " " __VA_ARGS__); \
} while(0)

#define log_info(...) do {\
  if (!verbose) break;\
  fprintf(stderr, BLU " INFO " RESET " " __VA_ARGS__);\
} while(0)

#define log_warn(...) do {\
  if (!verbose) break;\
  fprintf(stderr, YLW " WARN " RESET " " __VA_ARGS__);\
} while(0)

#define log_dbg(...) do {\
  if (!verbose) break;\
  fprintf(stderr, GRAY " DBUG " RESET " " __VA_ARGS__);\
} while(0)


#define STACK_SIZE 512

static unsigned char *MEMORY;
static unsigned int ptr = 0;
int max_ptr = 0;
static unsigned short STACK[STACK_SIZE];
static unsigned int SP = 0;

#define NEST(A) (STACK[SP++] = A)
#define POP()   (STACK[--SP])


static size_t memory_size = 512;
static bool dump = false;
static bool verbose = false;

void
print_config() {
  log_info("parsed config:\n\
memory size: %lu cells\n\
dump_mem: %s\n\
verbose: %s\n", memory_size, dump ? "true":"false", verbose ? "true" : "false");
}

void
usage() {
  printf("\
Usage: %s filename [OPTIONS]...\n\
toy brainfuck interpreter written in c.\n\
\n\
OPTIONS: \n\
  -m, --memory (default: 512)     set the tape size in cells\n\
  -d, --dump (default: none)      set the memory dump format [none, hex, ascii]\n\
  -v, --verbose                   increase verbosity\n\
  -h, --help                      print usage\n\
", "bf");
}

void
argparse(int argc, char **argv) {
  char *end_ptr;

  for (int i = 0; i < argc;){
    if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--memory")){
      // memory size
      memory_size = strtoul(argv[i+1], &end_ptr, 10);
    } else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dump")){
      // memory dump type
      dump = true;
      i += 1;
      continue;
    } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
      // verbosity
      verbose = true;
      i += 1;
      continue;
    } else {
      fprintf(stderr, "invalid option %s\n", argv[i]);
      usage();
      exit(1);
    }

    i += 2;
  }
}

void
dump_mem(){
  log_info("printing memory dump\n");
  int width = 16;
  for (int i = 0; i < memory_size && i < max_ptr; i += width) {
    for (int j = 0; j < width; j++) {
      printf("%2X ", MEMORY[i + j]);
    }
    printf("\t");
    for (int j = 0; j < width; j++) {
      if (isprint(MEMORY[i + j])) {
        printf(" %c ", MEMORY[i + j]);
      } else if (MEMORY[i + j] == '\n') {
        printf("\\n ");
      } else {
        printf(" . ");
      }
    }
    printf("\n");
  }
}

void
interpret(char *filepath) {
  FILE *file = fopen(filepath, "r");
  if(!file){
    handle_err("%s: %s\n", filepath, strerror(errno));
  }

  char ch;

  while((ch = fgetc(file)) != EOF){
    // for possible memory dumps later
    if (ptr > max_ptr) {
      max_ptr = ptr;
    }

    int seek_pos;
    int nest = 0;

    switch(ch){
      case '<':
        ptr = (ptr - 1 + memory_size) % memory_size;
        break;
      case '>':
        ptr = (ptr + 1 + memory_size) % memory_size;
        break;
      case '+':
        MEMORY[ptr]++;
        break;
      case '-':
        MEMORY[ptr]--;
        break;
      case '.':
        printf("%c", MEMORY[ptr]);
        break;
      case '[':
        if (MEMORY[ptr]) {
          NEST(ftell(file));
        } else {
          // skip to end of loop
          nest = 1;
          do {
            ch = fgetc(file);
            if (ch == '[') nest += 1;
            if (ch == ']') nest -= 1;
          } while(nest);
        }
        break;
      case ']':
        if (SP == 0) {
          printf("stack underflow\n");
          exit(1);
        }
        seek_pos = POP();
        fseek(file, seek_pos - 1, SEEK_SET);
        break;
    }
  }

  fclose(file);
}

int 
main(int argc, char **argv){
  if(argc < 2) {
    handle_err("no file specified\n");
  }
  if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    usage();
    exit(0);
  }

  char *filepath = argv[1];
  argparse(argc - 2, argv + 2);
  if (verbose) {
    print_config();
  }

  MEMORY = (unsigned char *)malloc(sizeof(unsigned char) * memory_size);
  memset(MEMORY, 0, sizeof(unsigned char) * memory_size);
  log_info("interpreting file %s\n", filepath);
  interpret(filepath);

  if(dump){
    dump_mem();
  }

  return 0;
}
