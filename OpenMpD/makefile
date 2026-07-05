CC=g++
# CFLAGS=-gx -DBASE_REPRESENTATION
#CFLAGS=-gx 
CFLAGS=-Wall -g # -fsanitize=address
CPPFLAGS=-Iinclude
LDFLAGS=-ll

SRC_DIR=src
INCLUDE_DIR=include
SYNTAX_DIR=syntax
VPATH=$(SRC_DIR):$(INCLUDE_DIR):$(SYNTAX_DIR)

YACC= bison  
#YFLAGS=-Nl1200 -d -v -t
YFLAGS= -g -k -d -v -t

LEX= flex

OBJS=preproparser.o preprolexer.o ompparser.o omplexer.o y.tab.o lex.yy.o main.o cluster_stack.o hash_map.o task_utils.o tasking_region.o task_async_block.o task_body_transform.o translator_state.o pragma_args.o halo_transform.o distribute_transform.o codegen_utils.o scatter_gather_transform.o mpi_lifecycle.o mpi_type_transform.o memory_transform.o output_slots.o reduction_transform.o tasking_emit.o writer.o

all: fparse

fparse: $(OBJS)
	$(CC) -o fparse $(CFLAGS) $(OBJS) $(LDFLAGS)

regression-test: fparse
	python3 tests/regression/run_regression.py

ompparser.o: ompparser.cc ompparser.hh distribute_transform.h pragma_args.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

ompparser.cc ompparser.hh: ompparser.yy distribute_transform.h pragma_args.h
	$(YACC) $(YFLAGS) -o ompparser.cc $<

omplexer.o: omplexer.cc ompparser.hh
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

omplexer.cc: omplexer.ll
	$(LEX) -o omplexer.cc $<

preproparser.o: preproparser.cc preproparser.hh
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

preproparser.cc preproparser.hh: preproparser.yy
	$(YACC) $(YFLAGS) -o preproparser.cc $<

preprolexer.o: preprolexer.cc preproparser.hh
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

preprolexer.cc preprolexer.h: preprolexer.ll
	$(LEX) -o preprolexer.cc $<

y.tab.o: y.tab.cc y.tab.hh symbol_table.h cluster_stack.h mpi_lifecycle.h output_slots.h tasking_region.h task_body_transform.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

y.tab.cc: C99-parser.yacc symbol_table.h cluster_stack.h mpi_lifecycle.h output_slots.h tasking_region.h task_body_transform.h
	$(YACC) $(YFLAGS) -o y.tab.cc $<

lex.yy.o: lex.yy.cc y.tab.hh symbol_table.h writer.h tasking_emit.h cluster_stack.h tasking_region.h distribute_transform.h pragma_args.h reduction_transform.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

lex.yy.cc: C99-scanner.lex symbol_table.h writer.h tasking_emit.h cluster_stack.h tasking_region.h distribute_transform.h pragma_args.h
	$(LEX) -o lex.yy.cc $<

main.o: main.cc mpi_lifecycle.h output_slots.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

cluster_stack.o: cluster_stack.c cluster_stack.h reduction_transform.h scatter_gather_transform.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

hash_map.o: hash_map.c hash_map.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

task_utils.o: task_utils.c task_utils.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

task_async_block.o: task_async_block.c task_async_block.h task_utils.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

tasking_region.o: tasking_region.c tasking_region.h hash_map.h task_utils.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

task_body_transform.o: task_body_transform.c task_body_transform.h task_async_block.h task_utils.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

translator_state.o: translator_state.c translator_state.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

pragma_args.o: pragma_args.c pragma_args.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

halo_transform.o: halo_transform.c halo_transform.h translator_state.h symbol_table.h pragma_args.h scatter_gather_transform.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

distribute_transform.o: distribute_transform.cc distribute_transform.h translator_state.h halo_transform.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

codegen_utils.o: codegen_utils.cc codegen_utils.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

scatter_gather_transform.o: scatter_gather_transform.cc scatter_gather_transform.h symbol_table.h pragma_args.h codegen_utils.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

mpi_lifecycle.o: mpi_lifecycle.cc mpi_lifecycle.h cluster_stack.h writer.h output_slots.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

output_slots.o: output_slots.cc output_slots.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

mpi_type_transform.o: mpi_type_transform.cc mpi_type_transform.h codegen_utils.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

memory_transform.o: memory_transform.cc memory_transform.h symbol_table.h codegen_utils.h pragma_args.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

reduction_transform.o: reduction_transform.cc reduction_transform.h pragma_args.h symbol_table.h codegen_utils.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

tasking_emit.o: tasking_emit.cc tasking_emit.h tasking_region.h cluster_stack.h output_slots.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

writer.o: writer.cc writer.h cluster_stack.h tasking_region.h tasking_emit.h halo_transform.h distribute_transform.h reduction_transform.h scatter_gather_transform.h mpi_lifecycle.h mpi_type_transform.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

clean:
	rm -f $(OBJS) core fparse *.o
	rm -f y.* lex.yy.*
	rm -f ompparser.output ompparser.gv ompparser.dot ompparser.hh ompparser.cc
	rm -f omplexer.cc
	rm -f preproparser.cc preproparser.hh preproparser.gv preproparser.output
	rm -f preprolexer.cc preprolexer.h
	rm -f log.txt error.txt output.c sym_tables.txt
	rm -f *_output.c *_output_reference.c
	rm -f out *_out
	rm -f mandelbrot.ppm julia_set.tga
	rm -f *.pgm g.png s.png lenna.png original.bmp
	rm -f game_of_life_output_version_out.txt kind
