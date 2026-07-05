#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include "symbol_table.h"
#include "mpi_lifecycle.h"
#include "output_slots.h"
#include "writer.h"

extern void yyerror(const char *s);
extern int yyparse ();
extern int prepro_parse();
extern void parsePrepro(char *filename);

extern int yydebug;
extern FILE *yyin, *yyout, *prepro_in;
extern int MVL_LINNUM;
extern int error_count, line_count;
extern int use_ompd_runtime_main;

/* Returns whether the source file contains any OMPD tasking pragma */
static int source_uses_ompd_tasking(FILE *f)
{
  char buffer[4096];
  long pos = ftell(f);
  int found = 0;

  rewind(f);

  while (fgets(buffer, sizeof(buffer), f) != NULL) {
    char *p = buffer;

    while (*p == ' ' || *p == '\t') {
      ++p;
    }

    if (strncmp(p, "#pragma", 7) == 0 &&
        strstr(p, "omp") != NULL &&
        (strstr(p, "task_async") != NULL ||
         strstr(p, "taskwait") != NULL ||
         strstr(p, "taskgroup") != NULL ||
         strstr(p, "taskyield") != NULL)) {
      found = 1;
      break;
    }
  }

  if (pos >= 0) {
    fseek(f, pos, SEEK_SET);
  } else {
    rewind(f);
  }

  return found;
}

FILE *inputFile;
int num_errores_prepro = 0;
std::ofstream logFile, errFile, sym_tables, output;
symbol_table table(38);

static const int OMPD_EXIT_USAGE = 2;

int main(int argc, const char* argv[]){

  if ((argc!=4) && (argc!=5)) {
      std::cout << "command: ./fparse input.c log.txt error.txt [output.c]" << std::endl;
      return OMPD_EXIT_USAGE;
  }

  if((inputFile=fopen(argv[1],"r"))==NULL) {
      printf("Cannot Open Input File.\n");
      exit(EXIT_FAILURE);
  }

  logFile.open(argv[2]);
  errFile.open(argv[3]);

  if (argc==5){
    output.open(argv[4]);
    mpi_runtime_reserve_output_prelude_slots();
  }

  sym_tables.open("sym_tables.txt");

  std::string preprocessor_temp_path = "/tmp/ompd_prepro_XXXXXX";
  int preprocessor_fd = mkstemp(&preprocessor_temp_path[0]);
  if (preprocessor_fd == -1) {
    perror("mkstemp");
    errFile << "Error creating preprocessor temporary file" << endl;
    exit(EXIT_FAILURE);
  }

  switch (fork()) {

    case -1:
      errFile << "Error during fork" << endl;
      exit(EXIT_FAILURE);

    case 0:
      if (dup2(preprocessor_fd, 1) == -1) {
        errFile << "Error redirecting preprocessor output" << endl;
        exit(EXIT_FAILURE);
      }

      close(preprocessor_fd);
      execlp("mpicc", "mpicc", "-E", "-P", "-include", "mpi.h", argv[1], NULL);
      errFile << "Error executing preprocessor command" << endl;
      exit(EXIT_FAILURE);
  }

  int status;
  wait(&status);
  close(preprocessor_fd);
  if (status) {
    errFile << "Error during preprocessor pass" << endl;
  }

  FILE *preprocessed_file = fopen(preprocessor_temp_path.c_str(), "r");
  if (!preprocessed_file) {
    errFile << "Error opening preprocessor file: " << preprocessor_temp_path << endl;
    exit(EXIT_FAILURE);
  }
  prepro_in = preprocessed_file;

  symbol_info *builtin_va_list_symbol = new symbol_info("__builtin_va_list", "IDENTIFIER");
  builtin_va_list_symbol->set_is_type_symbol(true);
  symbol_info *io_file_symbol = new symbol_info("_IO_FILE", "IDENTIFIER");
  io_file_symbol->set_is_struct(true);
  symbol_info *struct_io_file_symbol = new symbol_info("STRUCT__IO_FILE", "IDENTIFIER");
  struct_io_file_symbol->set_is_struct(true);
  symbol_info *float128_symbol = new symbol_info("_Float128", "IDENTIFIER");
  float128_symbol->set_is_type_symbol(true);

  table.insert(builtin_va_list_symbol);
  table.insert(io_file_symbol);
  table.insert(struct_io_file_symbol);
  table.insert(float128_symbol);

  prepro_parse();

  if (num_errores_prepro > 0) {
    errFile << "Error running preprocessor" << endl;
    exit(EXIT_FAILURE);
  }

  logFile << "Finished reading preprocessor" << endl;

  fclose(preprocessed_file);

  if (unlink(preprocessor_temp_path.c_str()) != 0) {
    errFile << "Error deleting generated preprocessor file" << endl;
  }

  if (source_uses_ompd_tasking(inputFile)) {
    use_ompd_runtime_main = 1;
  }

  rewind(inputFile);

  yyin=inputFile;

  yyparse();

  writer_flush_current_line();

  logFile << endl;
  logFile << "Total lines: " << line_count << endl;
  logFile << "Total errors: " << error_count << endl << endl;

  logFile.close();
  output.close();
  if (argc == 5 && output_slots_apply(argv[4]) != 0) {
    errFile << "Error applying deferred insertions to output file" << endl;
    exit(EXIT_FAILURE);
  }
  sym_tables.close();
  errFile.close();
  fclose(yyin);

  return 0;
}
