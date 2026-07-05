#ifndef WRITER_H
#define WRITER_H

/*
 * Reconstructs translated source output from lexer tokens. The writer buffers
 * source lines, coordinates directive-driven rewrites, manages sequential
 * region state and flushes generated text to the output stream.
 */

#include <iostream>
#include <fstream>
#include "cluster_stack.h"

using namespace std;

extern ofstream output;

/* Stores the current lexer token text for writer processing */
void writer_set_token_text(char *yytext);

/* Processes the current lexer token and updates output buffers */
void writer_process_current_token();

/* Flushes the current source line to the generated output */
void writer_flush_current_line();

/* Marks that generated statement output has started */
void writer_mark_statement_output_started(void);

/* Marks that the current sequential region has been closed */
void writer_mark_sequential_region_closed(void);

/* Returns whether the current sequential region is closed */
int writer_is_sequential_region_closed(void);

/* Marks that final translation output has been emitted */
void writer_mark_translation_finalized(void);

#endif
