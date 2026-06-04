#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <sys/wait.h>
#include "SymbolTable.h"

extern int yydebug;
extern FILE *yyin, *yyout, *prepro_in;
FILE *inputFile;

extern char *linea;
extern int MVL_LINNUM;
int num_errores_prepro = 0;

extern void yyerror(const char *s);
extern int yyparse ();
extern int prepro_parse();
extern void lastLine();
extern void parsePrepro(char *filename);

std::ofstream logFile, errFile, sym_tables, output;

extern int error_count, line_count;

SymbolTable table(38);

int main( int argc, const char* argv[] )
{
  /* missing parameter check */

  /* yydebug=1; */


  /* open files */
  if ((argc!=4) && (argc!=5)) {
      std::cout << "command: ./fparse input.c log.txt error.txt [output.c]" << std::endl;
      return 0;
  }

  if((inputFile=fopen(argv[1],"r"))==NULL) {
      printf("Cannot Open Input File.\n");
      exit(1);
  }

  logFile.open(argv[2]);
  errFile.open(argv[3]);

  if (argc==5){
    output.open(argv[4]);
  }

  sym_tables.open("sym_tables.txt");

  char *filePrepo;
  if ((filePrepo = (char *) malloc(strlen(argv[1]) + 12)) == NULL) {
    errFile << "Error asignando memoria dinámica" << endl;
    exit(254);
  }

  strcpy(filePrepo, "/tmp/");
  strcpy(filePrepo + 5, argv[1]);
  strcpy(filePrepo + strlen(argv[1]) + 5, ".prepo");
  filePrepo[strlen(argv[1]) + 11] = '\0';

  switch (fork()) {
    case -1: errFile << "Error en fork" << endl; exit(255);

    case 0:
          int prepo;

          if ((prepo = open(filePrepo, O_CREAT|O_RDWR|O_TRUNC, 0600)) == -1) {
            perror("ERROR");
            errFile << "Error abriendo archivo para redireccion de preprocesador" << endl;
            exit(253);
          }

          if (dup2(prepo, 1) == -1) {
            errFile << "Error haciendo redireccion" << endl;
            exit(252);
          }

          //execlp("gcc", "gcc", "-E", argv[1], NULL);
          execlp("mpicc", "mpicc", "-E", "-P", "-include", "mpi.h", argv[1], NULL);
          errFile << "Error en exec" << endl;
          exit(251);
  }

  int status;

  wait(&status);

  if (status) {
    errFile << "Error en pasada de preprocesador" << endl;
  }

  FILE *preprocesado = fopen(filePrepo, "r");

  if (!preprocesado) {
    errFile << "Error abriendo fichero de preprocesador: " << filePrepo << endl;
    exit(45);
  }

  prepro_in = preprocesado;

  SymbolInfo *builtvalist = new SymbolInfo("__builtin_va_list", "IDENTIFIER");
  builtvalist->setSymIsType(true);
  SymbolInfo *IOFICH = new SymbolInfo("_IO_FILE", "IDENTIFIER");
  IOFICH->setIsStruct(true);
  SymbolInfo *STRUCTIOFICH = new SymbolInfo("STRUCT__IO_FILE", "IDENTIFIER");
  STRUCTIOFICH->setIsStruct(true);
  SymbolInfo *F128 = new SymbolInfo("_Float128", "IDENTIFIER");
  F128->setSymIsType(true);

  table.insert(builtvalist);
  table.insert(IOFICH);
  table.insert(STRUCTIOFICH);
  table.insert(F128);

  prepro_parse();

  if (num_errores_prepro > 0) {
    errFile << "Error ejecutando preprocesador" << endl;
    exit(46);
  }

  logFile << "Terminada lectura de preprocesador" << endl;

  fclose(preprocesado);

  switch (fork()) {
    case -1: errFile << "Error en fork" << endl; exit(255);

    case 0:
          execlp("rm", "rm", "-f", filePrepo, NULL);
          errFile << "Error en exec" << endl;
          exit(251);
  }

  wait(&status);

  if (status) {
    errFile << "Error en borrado de fichero generado por preprocesador" << endl;
  }

  yyin=inputFile;

  free(filePrepo);

  //yydebug = 1;  // Enable Bison's debug mode

  yyparse();

  lastLine();

  logFile << endl;
  logFile << "Total lines: " << line_count << endl;
  logFile << "Total errors: " << error_count << endl << endl;

  logFile.close();
  output.close();
  sym_tables.close();
  errFile.close();
  fclose(yyin);

  return 0;

}
