CC=g++
# CFLAGS=-gx -DBASE_REPRESENTATION
#CFLAGS=-gx 
CFLAGS=-Wall -g # -fsanitize=address
LDFLAGS=-ll

YACC= bison  
#YFLAGS=-Nl1200 -d -v -t
YFLAGS= -g -k -d -v -t

LEX= flex -p 

OBJS=preproparser.o preprolexer.o ompparser.o omplexer.o y.tab.o lex.yy.o main.o cluster_stack.o hash_map.o tasking_region.o task_async_block.o task_body_transform.o

all: fparse

fparse: $(OBJS)
	$(CC) -o fparse $(CFLAGS) $(OBJS) $(LDFLAGS)

ompparser.o: ompparser.cc ompparser.hh
	$(CC) -c $(CFLAGS) ompparser.cc

ompparser.cc ompparser.hh: ompparser.yy
	$(YACC) $(YFLAGS) -o ompparser.cc ompparser.yy

omplexer.o: omplexer.cc ompparser.hh
	$(CC) -c $(CFLAGS) omplexer.cc

omplexer.cc: omplexer.ll
	$(LEX) -o omplexer.cc omplexer.ll

preproparser.o: preproparser.cc preproparser.hh
	$(CC) -c $(CFLAGS) preproparser.cc

preproparser.cc preproparser.hh: preproparser.yy
	$(YACC) $(YFLAGS) -o preproparser.cc preproparser.yy

preprolexer.o: preprolexer.cc preproparser.hh
	$(CC) -c $(CFLAGS) preprolexer.cc

preprolexer.cc preprolexer.h: preprolexer.ll
	$(LEX) -o preprolexer.cc preprolexer.ll

y.tab.o: y.tab.cc y.tab.hh
	$(CC) -c $(CFLAGS) y.tab.cc

y.tab.cc: C99-parser.yacc
	$(YACC) $(YFLAGS) -o y.tab.cc C99-parser.yacc

lex.yy.o: lex.yy.cc y.tab.hh
	$(CC) -c $(CFLAGS) lex.yy.cc

lex.yy.cc: C99-scanner.lex
	$(LEX) -o lex.yy.cc C99-scanner.lex

main.o: main.cc
	$(CC) -c $(CFLAGS) main.cc

cluster_stack.o: cluster_stack.c cluster_stack.h 
	$(CC) -c $(CFLAGS) cluster_stack.c

hash_map.o: hash_map.c hash_map.h
	$(CC) -c $(CFLAGS) hash_map.c

task_async_block.o: task_async_block.c task_async_block.h
	$(CC) -c $(CFLAGS) task_async_block.c

tasking_region.o: tasking_region.c tasking_region.h hash_map.h
	$(CC) -c $(CFLAGS) tasking_region.c

task_body_transform.o: task_body_transform.c task_body_transform.h task_async_block.h
	$(CC) -c $(CFLAGS) task_body_transform.c

clean:
	cat /dev/null > log.txt
	cat /dev/null > error.txt
	cat /dev/null > output.c
	rm -f $(OBJS) core y.* lex.yy.* ompparser.output ompparser.gv ompparser.dot ompparser.hh ompparser.cc omplexer.cc preproparser.cc preproparser.hh preprolexer.cc preproparser.gv preproparser.output fparse *.o
