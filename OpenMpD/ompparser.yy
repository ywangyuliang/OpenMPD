/*
 * Copyright (c) 2018-2020, High Performance Computing Architecture and System
 * research laboratory at University of North Carolina at Charlotte (HPCAS@UNCC)
 * and Lawrence Livermore National Security, LLC.
 *
 * SPDX-License-Identifier: (BSD-3-Clause)
 */

/* OpenMP C/C++/Fortran Grammar */

%define api.prefix {openmp_}
%defines
%define parse.error verbose

%{
using namespace std;
/* DQ (2/10/2014): IF is conflicting with Boost template IF. */
#undef IF

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include <ostream>
#include <iostream>
#include <fstream>
#include "cluster_stack.h"
#include "SymbolTable.h"
#include "tasking_region.h"
#include "task_async_block.h"

/*the scanner function*/
extern int openmp_lex();

extern SymbolTable table;

/*A customized initialization function for the scanner, str is the string to be scanned.*/
extern void openmp_lexer_init(const char* str);
/* Standalone ompparser */
extern void start_lexer(const char* input);
extern void end_lexer(void);

extern void openmp_parse_expr();
static int openmp_error(const char*);

int chunk = 0;
int update = 0;
extern int MPIInitDone;
extern int main_end;
extern int main_init;
extern int enMain;
extern int MPIInitMainDone;
extern int enFuncion;
extern int use_ompd_runtime_main;
extern void MPIInit();
extern void OMPDInitRuntimeHere();
void MPIInitParte2();

int includeStringDone = 0;
int enDistribute = 0;
int enDistributeExtendido = 0;
int meterEnClause = 0;
int enReductionCluster = 0;
int enReductionDistribute = 0;
int enAllReductionCluster = 0;
int enAllReductionDistribute = 0;
int enScatter = 0;
int enGather = 0;
int enAllGather = 0;
int enGatherInst = 0;
int enAllGatherInst = 0;
int chunk_pos;
int enDeclare = 0;

int enMaster = 0;
int master_n_llaves = 0;
int master_closes_by_braces = 0;

int enNumTeams = 0;
std::string numTeamsExpr = "";

extern int dist_n_llaves;
extern string scheduleDist;
extern string guardarLineasDist;
extern ofstream output;
extern int enSecuencial;

void * (*exprParse)(const char*) = NULL;

//From template
extern void addArg(const char *arg);
extern void MPIAlloc();
extern void MPIBroad();
extern void MPIScatterHalo();
extern void MPIUpdateHalo();
extern void MPIWriteCluster();
extern void addReduce(bool vars, const char *arg);
extern void addAllReduce(bool vars, const char *arg);
extern void aumentarScatter();
extern void aumentarGather();
extern void aumentarAllGather();
extern void addArgScatter(const char *arg);
extern void addArgGather(const char *arg);
extern void addArgAllGather(const char *arg);
extern void aumentarAllReduction();
extern void aumentarReduction();
extern void IncludeString();
extern void MPIDeclareCluster();
extern void MPIScatter();
extern void MPIEmpezarSecuencial();

extern std::string translateTypes(std::string type);

/* ------------------ task ------------------ */
task_depend_kind_t ta_current_kind = TD_DEPEND_IN;
int ta_expect_task_body = 0;
int ta_in_task_body_parse = 0;
int tasking_region_id_counter = 0;
/* ----------------------------------------------- */

%}

%locations

/* The %union declaration specifies the entire collection of possible data types for semantic values.
these names are used in the %token and %type declarations to pick one of the types for a terminal or nonterminal symbol
corresponding C type is union name defaults to YYSTYPE.
*/

%union {  int itype;
          double ftype;
          const char* stype;
          void* ptype; /* For expressions or variables */
        }


%token  OMP FOR DO DECLARE DISTRIBUTE LOOP SCAN SECTIONS SECTION SINGLE CANCEL TASKGROUP CANCELLATION POINT THREAD VARIANT THREADPRIVATE METADIRECTIVE MAPPER
        IF NUM_THREADS DEFAULT COPYIN REDUCTION PROC_BIND ALLOCATE SIMD TASK LASTPRIVATE WHEN MATCH
        LINEAR SCHEDULE COLLAPSE NOWAIT ORDER ORDERED STATIC DYNAMIC GUIDED AUTO RUNTIME
        SAFELEN SIMDLEN ALIGNED ALIGN NONTEMPORAL UNIFORM INBRANCH NOTINBRANCH DIST_SCHEDULE BIND INCLUSIVE EXCLUSIVE COPYPRIVATE ALLOCATOR INITIALIZER OMP_PRIV IDENTIFIER_DEFAULT WORKSHARE/*YAYING*/
        PLUS MINUS STAR BITAND BITOR BITXOR LOGAND LOGOR EQV NEQV MAX MIN
        DEFAULT_MEM_ALLOC LARGE_CAP_MEM_ALLOC CONST_MEM_ALLOC HIGH_BW_MEM_ALLOC LOW_LAT_MEM_ALLOC CGROUP_MEM_ALLOC
        PTEAM_MEM_ALLOC THREAD_MEM_ALLOC
        TEAMS
        NUM_TEAMS THREAD_LIMIT
        END USER CONSTRUCT DEVICE IMPLEMENTATION CONDITION SCORE VENDOR
        KIND HOST NOHOST ANY CPU GPU FPGA ISA ARCH EXTENSION
        AMD ARM BSC CRAY FUJITSU GNU IBM INTEL LLVM PGI TI UNKNOWN
        FINAL UNTIED MERGEABLE IN_REDUCTION DEPEND PRIORITY AFFINITY DETACH MODIFIER_ITERATOR DEPOBJ FINAL_CLAUSE IN INOUT MUTEXINOUTSET OUT
        TASKLOOP GRAINSIZE NUM_TASKS NOGROUP TASKYIELD REQUIRES REVERSE_OFFLOAD UNIFIED_ADDRESS UNIFIED_SHARED_MEMORY ATOMIC_DEFAULT_MEM_ORDER DYNAMIC_ALLOCATORS SEQ_CST ACQ_REL RELAXED
        USE_DEVICE_PTR USE_DEVICE_ADDR TARGET DATA ENTER EXIT ANCESTOR DEVICE_NUM IS_DEVICE_PTR HAS_DEVICE_ADDR
        DEFAULTMAP BEHAVIOR_ALLOC BEHAVIOR_TO BEHAVIOR_FROM BEHAVIOR_TOFROM BEHAVIOR_FIRSTPRIVATE BEHAVIOR_NONE BEHAVIOR_DEFAULT CATEGORY_SCALAR CATEGORY_AGGREGATE CATEGORY_POINTER CATEGORY_ALLOCATABLE UPDATE TO FROM TO_MAPPER FROM_MAPPER USES_ALLOCATORS
 LINK DEVICE_TYPE MAP MAP_MODIFIER_ALWAYS MAP_MODIFIER_CLOSE MAP_MODIFIER_MAPPER MAP_TYPE_TO MAP_TYPE_FROM MAP_TYPE_TOFROM MAP_TYPE_ALLOC MAP_TYPE_RELEASE MAP_TYPE_DELETE EXT_ BARRIER TASKWAIT FLUSH RELEASE ACQUIRE ATOMIC READ WRITE CAPTURE HINT CRITICAL SOURCE SINK DESTROY THREADS
        CLUSTER ALLOC BROAD SCATTER GATHER ALLGATHER ALLREDUCTION CHUNK HALO TASK_ASYNC 
%token <itype> ICONSTANT
%token <stype> EXPRESSION ID_EXPRESSION EXPR_STRING VAR_STRING TASK_REDUCTION MODIFIER_CONDITIONAL MODOFIER_VAL MODOFIER_REF MODOFIER_UVAL
                MODIFIER_INSCAN MODIFIER_TASK MODIFIER_DEFAULT CONCURRENT PARALLEL SHARED NONE PRIVATE FIRSTPRIVATE MASTER CLOSE SPREAD
                MODIFIER_MONOTONIC MODIFIER_NONMONOTONIC MODIFIER_SIMD
/* associativity and precedence */
%left '<' '>' '=' "!=" "<=" ">="
%left '+' '-'
%left '*' '/' '%'

%type <stype> expression
%type <stype> variable
%type <stype> reduction_enum_identifier lastprivate_modifier

/* start point for the parsing */
%start openmp_directive

%%

/* lang-dependent expression is only used in clause, at this point, the current_clause object should already be created. */
expression : EXPR_STRING {$$=$1; }
variable :   EXPR_STRING {$$=$1; } 

/*expr_list : expression
        | expr_list ',' expression
        ;
*/

var_list : variable {if (meterEnClause) {guardarLineasDist += $1;}}
          | var_list ',' variable {if (meterEnClause) {guardarLineasDist += ","; guardarLineasDist += $3;}}
          ;

var_list_cluster : variable {
                      addArg($1);
                    }
        | var_list_cluster ',' variable 
                    {
                      addArg($3);
                    }
        ;

var_list_reduction : variable {if (meterEnClause){guardarLineasDist += $1;} if(enReductionCluster || enReductionDistribute){addReduce(true, $1);}}
        | var_list_reduction {if (meterEnClause){guardarLineasDist += ",";}} ',' variable {if (meterEnClause){guardarLineasDist += $4;} if(enReductionCluster || enReductionDistribute){addReduce(true, $4);}}
        ;

var_list_allreduction : variable { if(enAllReductionCluster || enAllReductionDistribute){addAllReduce(true, $1);}}
        | var_list_allreduction ',' variable { if(enAllReductionCluster || enAllReductionDistribute){addAllReduce(true, $3);}}
        ;

var_chunk : variable   { addArg($1); } ':' expression { addArg($4); }
  ;

var_chunk_list_cluster_aux_aux :  /*empty*/
                                | ',' 
                                    {
                                      chunk_pos += 1;
                                      if (enScatter) {
                                        aumentarScatter();
                                      }
                                      else if (enGatherInst) {
                                        aumentarGather();
                                      }
                                      else if (enAllGatherInst) {
                                        aumentarAllGather();
                                      }
                                    } var_chunk_list_cluster
                                ;

var_chunk_list_cluster_aux :  var_chunk_list_cluster_aux_aux
                            | ':' CHUNK '(' variable ')'
                                {
                                  if (enScatter) {
                                    addArgScatter($4);
                                  }
                                  else if (enGatherInst) {
                                    addArgGather($4);
                                  }
                                  else if (enAllGatherInst) {
                                    addArgAllGather($4);
                                  }
                                }
                              var_chunk_list_cluster_aux_aux
                            ;

var_chunk_list_cluster : variable
                          {
                            if (enScatter) {
                              addArgScatter($1);
                            }
                            else if (enGatherInst) {
                              addArgGather($1);
                            }
                            else if (enAllGatherInst) {
                              addArgAllGather($1);
                            }
                          }
                        var_chunk_list_cluster_aux

dist_schedule_clause_cluster : DIST_SCHEDULE '(' dist_schedule_parameter_cluster ')'
                            ;

dist_schedule_parameter_cluster : STATIC
                                | STATIC ',' EXPR_STRING
                                  {
                                    if (enDistribute) {
                                      if (scheduleDist.size() != 0) {
                                        fprintf(stderr, "Doble shcedule en cluster distribute\n");
                                        exit(245);
                                      }

                                      scheduleDist = $3;
                                    }
                                  }
                                ;

openmp_directive : parallel_directive
                 | metadirective_directive
                 | declare_variant_directive
                 | for_directive
                 | do_directive
                 | simd_directive
                 | teams_directive
                 | for_simd_directive
                 | do_simd_directive
                 | parallel_for_simd_directive
                 | parallel_do_simd_directive
                 | declare_simd_directive
                 | declare_simd_fortran_directive
                 | distribute_directive {dist_n_llaves = 0;}
                 | distribute_simd_directive {dist_n_llaves = 0; guardarLineasDist += "\n";}
                 | distribute_parallel_for_directive {dist_n_llaves = 0; guardarLineasDist += "\n";}
                 | distribute_parallel_do_directive {dist_n_llaves = 0; guardarLineasDist += "\n";}
                 | distribute_parallel_for_simd_directive {dist_n_llaves = 0; guardarLineasDist += "\n";}
                 | distribute_parallel_do_simd_directive {dist_n_llaves = 0; guardarLineasDist += "\n";}
                 | parallel_for_directive
                 | parallel_do_directive
                 | parallel_loop_directive
                 | parallel_sections_directive
                 | parallel_workshare_directive
                 | parallel_master_directive
                 | master_taskloop_directive
                 | master_taskloop_simd_directive
                 | parallel_master_taskloop_directive
                 | parallel_master_taskloop_simd_directive
                 | loop_directive
                 | scan_directive
                 | sections_directive
                 | section_directive
                 | single_directive
                 | workshare_directive
                 | cancel_directive
//                 | cancel_fortran_directive
                 | cancellation_point_directive
//                 | cancellation_point_fortran_directive
                 | allocate_directive
                 | task_directive
                 | taskloop_directive
                 | taskloop_simd_directive
                 | taskyield_directive
                 | requires_directive
                 | target_data_directive
                 | target_enter_data_directive
                 | target_exit_data_directive
                 | target_directive
                 | target_update_directive
                 | declare_target_directive
                 | end_declare_target_directive
                 | master_directive
                 | threadprivate_directive
                 | declare_reduction_directive
                 | declare_mapper_directive
                 | end_directive
                 | barrier_directive
                 | taskwait_directive
                 | taskgroup_directive
                 | flush_directive
                 | atomic_directive
                 | critical_directive
                 | depobj_directive
                 | ordered_directive
                 | teams_distribute_directive {dist_n_llaves = 0;}
                 | teams_distribute_simd_directive {dist_n_llaves = 0; guardarLineasDist += "\n";}
                 | teams_distribute_parallel_for_directive {dist_n_llaves = 0; guardarLineasDist += "\n";}
                 | teams_distribute_parallel_for_simd_directive {dist_n_llaves = 0; guardarLineasDist += "\n";}
                 | teams_loop_directive
                 | target_parallel_directive
                 | target_parallel_for_directive
                 | target_parallel_for_simd_directive
                 | target_parallel_loop_directive
                 | target_simd_directive
                 | target_teams_directive
                 | target_teams_distribute_directive
                 | target_teams_distribute_simd_directive
                 | target_teams_loop_directive
                 | target_teams_distribute_parallel_for_directive
                 | target_teams_distribute_parallel_for_simd_directive
                 | teams_distribute_parallel_do_directive
                 | teams_distribute_parallel_do_simd_directive
                 | target_parallel_do_directive
                 | target_parallel_do_simd_directive
                 | target_teams_distribute_parallel_do_directive
                 | target_teams_distribute_parallel_do_simd_directive
                 | declare_cluster_directive
                 | end_declare_cluster_directive
                 | {
                      if(MPIInitDone == 0){
                        MPIInit();

                        if (use_ompd_runtime_main) {
                          if (!enMain) {
                            MPIInitMainDone = 1;
                          }
                          else {
                            OMPDInitRuntimeHere();
                            MPIInitMainDone = 0;
                          }
                        }
                        else {
                          if (!enMain) {
                            MPIInitMainDone = 1;
                          }
                          else {
                            long posActual = output.tellp();
                            MPIInitParte2();
                            output.seekp(posActual);
                          }
                        }

                        main_init = 0;
                        MPIInitDone = 1;
                        main_end = 1;
                      }
                      else if (use_ompd_runtime_main && enMain && MPIInitMainDone == 1) {
                        OMPDInitRuntimeHere();
                        MPIInitMainDone = 0;
                      }

                      if (enSecuencial && cluster_stack_isEmpty() && (enFuncion >= 2 || enMain >= 1)) {
                        MPIWriteCluster();
                      }
                    }
                    directiveAuxCluster
                  ;

directiveAuxCluster
                  : cluster_directive 
				          | cluster_data_directive {MPIEmpezarSecuencial();}
				          | cluster_update_directive
				          | cluster_teams_directive
				          | cluster_distribute_directive {dist_n_llaves = 0;}
				          | cluster_teams_distribute_directive {dist_n_llaves = 0;}
				          | cluster_teams_master_directive
				          | task_async_directive
                  ;

variant_directive : parallel_directive
                  | metadirective_directive
                  | declare_variant_directive
                  | for_directive
                  | simd_directive
                  | teams_directive
                  | for_simd_directive
                  | declare_simd_directive
                  | distribute_directive
                  | distribute_simd_directive
                  | distribute_parallel_for_directive
                  | distribute_parallel_for_simd_directive
                  | loop_directive
                  | scan_directive
                  | sections_directive
                  | section_directive
                  | single_directive
                  | cancel_directive
                  | cancellation_point_directive
                  | allocate_directive
                  ;

fortran_paired_directive : parallel_directive
                         | do_paired_directive
                         | metadirective_directive
                         | master_directive
                         | teams_directive
                         | section_directive
                         | sections_paired_directive
                         | simd_directive
                         | do_simd_paired_directive
                         | distribute_directive
                         | distribute_simd_directive
                         | distribute_parallel_do_directive
                         | distribute_parallel_do_simd_directive
                         | parallel_do_directive
                         | parallel_loop_directive
                         | parallel_workshare_directive
                         | parallel_do_simd_directive
                         | parallel_master_directive
                         | master_taskloop_directive
                         | master_taskloop_simd_directive
                         | parallel_master_taskloop_directive
                         | parallel_master_taskloop_simd_directive
                         | loop_directive
                         | single_paired_directive
                         | workshare_paired_directive
                         | task_directive
                         | taskloop_directive
                         | taskloop_simd_directive
                         | target_directive
                         | target_data_directive
                         | critical_directive
                         | taskgroup_directive
                         | atomic_directive
                         | ordered_directive
                         | teams_distribute_directive
                         | teams_distribute_simd_directive
                         | teams_distribute_parallel_do_directive
                         | teams_distribute_parallel_do_simd_directive
                         | teams_loop_directive
                         | target_parallel_directive
                         | target_parallel_do_directive
                         | target_parallel_do_simd_directive
                         | target_parallel_loop_directive
                         | target_simd_directive
                         | target_teams_directive
                         | target_teams_distribute_directive
                         | target_teams_distribute_simd_directive
                         | target_teams_loop_directive
                         | target_teams_distribute_parallel_do_directive
                         | target_teams_distribute_parallel_do_simd_directive
                         ;

end_directive : END { } end_clause_seq { }
              ;

end_clause_seq : fortran_paired_directive
               ;

metadirective_directive : METADIRECTIVE { }
                          metadirective_clause_optseq
                        ;

metadirective_clause_optseq : /* empty */
                            | metadirective_clause_seq
                            ;

metadirective_clause_seq : metadirective_clause
                         | metadirective_clause_seq metadirective_clause
                         | metadirective_clause_seq ',' metadirective_clause
                         ;

metadirective_clause : when_clause
                     | default_variant_clause
                     ;

when_clause : WHEN { } '(' context_selector_specification ':' { } when_variant_directive { } ')'
            ;

when_variant_directive : variant_directive { }
                | { ; }
                ;

context_selector_specification : trait_set_selector
                | context_selector_specification trait_set_selector
                | context_selector_specification ',' trait_set_selector
                ;

trait_set_selector : trait_set_selector_name { } '=' '{' trait_selector_list { } '}'
                   ;

trait_set_selector_name : USER { }
                | CONSTRUCT { }
                | DEVICE { }
                | IMPLEMENTATION { }
                ;

trait_selector_list : trait_selector { }
                | trait_selector_list trait_selector { }
                | trait_selector_list ',' trait_selector { }
                ;

trait_selector : condition_selector
                | construct_selector { }
                | device_selector
                | implementation_selector
                ;

condition_selector : CONDITION '(' trait_score EXPR_STRING { } ')'
                ;

device_selector : context_kind
                | context_isa
                | context_arch
                ;

context_kind : KIND '(' trait_score context_kind_name ')'
             ;

context_kind_name : HOST { }
                  | NOHOST { }
                  | ANY { }
                  | CPU { }
                  | GPU { }
                  | FPGA { }
                  ;

context_isa : ISA '(' trait_score EXPR_STRING { }
            ;

context_arch : ARCH '(' trait_score EXPR_STRING { }
             ;

implementation_selector : VENDOR '(' trait_score context_vendor_name ')'
                        | EXTENSION '(' trait_score EXPR_STRING { }
                        | EXPR_STRING { }
                        | EXPR_STRING '(' trait_score ')' { }
                        ;

context_vendor_name : AMD { }
                    | ARM { }
                    | BSC { }
                    | CRAY { }
                    | FUJITSU { }
                    | GNU { }
                    | IBM { }
                    | INTEL { }
                    | LLVM { }
                    | PGI { }
                    | TI { }
                    | UNKNOWN { }
                    ;

construct_selector : parallel_selector
                   ;

parallel_selector : PARALLEL { }
                | PARALLEL '(' { } parallel_selector_parameter ')'
                ;

parallel_selector_parameter : trait_score parallel_clause_optseq
                            ;

trait_score : /* empty */
            | SCORE '(' EXPR_STRING { } ')' ':'
            ;

declare_variant_directive : DECLARE VARIANT { } variant_func_id
                     declare_variant_clause_optseq
                   ;

variant_func_id : '(' EXPR_STRING { } ')'
                ;

declare_variant_clause_optseq : /* empty */
                       | declare_variant_clause_seq
                       ;

declare_variant_clause_seq : declare_variant_clause
                    | declare_variant_clause_seq declare_variant_clause
                    | declare_variant_clause_seq ',' declare_variant_clause
                    ;

declare_variant_clause : match_clause
                ;

match_clause : MATCH { }
                '(' context_selector_specification ')' { }
             ;


parallel_directive : PARALLEL { } parallel_clause_optseq
                   ;
				   
cluster_directive : CLUSTER { cluster_stack_push(CL_ONE_STMT); } cluster_clause_optseq
				  ;
				  
declare_cluster_directive : DECLARE CLUSTER {
                                enDeclare = 1;
                            }
						  ;
						  
end_declare_cluster_directive : END DECLARE CLUSTER { enDeclare = 0; MPIDeclareCluster(); }
							  ;
			   
cluster_data_directive : CLUSTER DATA {
                          cluster_stack *c = cluster_stack_peek();
                          if(c != NULL){
                            if(c->has_task_pragma){
                              fprintf(stderr, "Error: task cannot coexist with other pragmas at the same level in the cluster\n");
                              exit(901);
                            }
                            c->has_non_task_pragma = 1;
                          }
                        } cluster_data_clause_optseq { }
					            ;

cluster_update_directive : CLUSTER UPDATE { 
                            cluster_stack *c = cluster_stack_peek();
                            if(c != NULL){
                              if(c->has_task_pragma){
                                fprintf(stderr, "Error: task cannot coexist with other pragmas at the same level in the cluster\n");
                                exit(901);
                              }
                              c->has_non_task_pragma = 1;
                            }
                            cluster_stack_push(CL_NOW); update = 1; 
                            } cluster_update_clause_optseq { cluster_stack_close(); }
			                    ; 

cluster_teams_directive : CLUSTER TEAMS {
                          cluster_stack *c = cluster_stack_peek();
                          if(c != NULL){
                            if(c->has_task_pragma){
                              fprintf(stderr, "Error: task cannot coexist with other pragmas at the same level in the cluster\n");
                              exit(901);
                            }
                            c->has_non_task_pragma = 1;
                          } 
                          cluster_stack_push(CL_ONE_STMT); 
                          } cluster_teams_clause_optseq
			                  ;

cluster_distribute_directive : CLUSTER DISTRIBUTE {
                              cluster_stack *c = cluster_stack_peek();
                              if(c != NULL){
                                if(c->has_task_pragma){
                                  fprintf(stderr, "Error: task cannot coexist with other pragmas at the same level in the cluster\n");
                                  exit(901);
                                }
                                c->has_non_task_pragma = 1;
                              }
                              enDistribute = 1;
                              } cluster_distribute_clause_optseq
			                      ;

cluster_teams_distribute_directive : CLUSTER TEAMS DISTRIBUTE {
                                      cluster_stack *c = cluster_stack_peek();
                                      if(c != NULL){
                                        if(c->has_task_pragma){
                                          fprintf(stderr, "Error: task cannot coexist with other pragmas at the same level in the cluster\n");
                                          exit(901);
                                        }
                                        c->has_non_task_pragma = 1;
                                      }
                                      enDistribute = 1;
                                      } cluster_teams_distribute_clause_optseq
				                            ;
				   
cluster_teams_master_directive : CLUSTER TEAMS MASTER {
                                  cluster_stack *c = cluster_stack_peek();
                                  if(c != NULL){
                                    if(c->has_task_pragma){
                                      fprintf(stderr, "Error: task cannot coexist with other pragmas at the same level in the cluster\n");
                                      exit(901);
                                    }
                                    c->has_non_task_pragma = 1;
                                  }
                                  enMaster = 1; master_n_llaves = 0; master_closes_by_braces = 0; MPIEmpezarSecuencial();
                                  } 
							                  ;

task_async_directive : TASK_ASYNC {

  use_ompd_runtime_main = 1;
  
  cluster_stack* c = cluster_stack_peek();
  if(c == NULL){
    fprintf(stderr, "Error: task_async out of cluster\n");
    exit(901);
  }

  if(c->has_non_task_pragma){
    fprintf(stderr, "Error: task cannot coexist with other pragmas at the same level in the cluster\n");
    exit(901);
  }
  c->has_task_pragma = 1;

  if(c->tasking_region == NULL){
    c->tasking_region = tasking_region_create(tasking_region_id_counter++);
    if(c->tasking_region == NULL){
      fprintf(stderr, "Error: tasking_region_create\n");
      exit(901);
    }
  }

  if(tasking_region_begin_task_async_block(c->tasking_region) != 0){
    fprintf(stderr, "Error: tasking_region_begin_task_async_block\n");
    exit(901);
  }

} task_async_clause_optseq { 

  ta_expect_task_body = 1;

}
;
					   
task_directive : TASK { } task_clause_optseq
               ;
               
taskloop_directive : TASKLOOP { } taskloop_clause_optseq
                   ;
taskloop_simd_directive : TASKLOOP SIMD { } taskloop_simd_clause_optseq 
                        ;
taskyield_directive : TASKYIELD { }
                    ;
requires_directive : REQUIRES { } requires_clause_optseq
                   ;
target_data_directive :  TARGET DATA { } target_data_clause_optseq 
                      ;
target_enter_data_directive :  TARGET ENTER DATA { } target_enter_data_clause_optseq 
                            ;
target_exit_data_directive :  TARGET EXIT DATA { } target_exit_data_clause_optseq 
                   ;
target_directive :  TARGET { } target_clause_optseq 
                   ;
target_update_directive :  TARGET UPDATE{ } target_update_clause_optseq 
                   ;
declare_target_directive : DECLARE TARGET { } declare_target_clause_optseq 
                   ;
flush_directive : FLUSH { } flush_clause_optseq 
                ;

end_declare_target_directive : END DECLARE TARGET { }
                             ;

master_directive : MASTER {
  cluster_stack *c = cluster_stack_peek();
  if(c != NULL){
    enMaster = 1;
    master_n_llaves = 0;
    master_closes_by_braces = 0;
    MPIEmpezarSecuencial();
  }
}
;

barrier_directive : BARRIER { }
                  ;
                  
taskwait_directive : TASKWAIT {
  cluster_stack* c = cluster_stack_peek();
  if(c == NULL){
    fprintf(stderr, "Error: taskwait out of cluster\n");
    exit(901);
  }

  c->has_task_pragma = 1;

  if(c->tasking_region == NULL){
    c->tasking_region = tasking_region_create(tasking_region_id_counter++);
    if(c->tasking_region == NULL){
      fprintf(stderr, "Error: tasking_region_create\n");
      exit(901);
    }
  }

  if(tasking_region_begin_taskwait_block(c->tasking_region, "#pragma omp taskwait") != 0){
    fprintf(stderr, "Error: tasking_region_begin_taskwait_block\n");
    exit(901);
  }

} taskwait_clause_optseq
;

taskgroup_directive : TASKGROUP { } taskgroup_clause_optseq
                    ;
critical_directive : CRITICAL { } critical_clause_optseq
                   ;
depobj_directive : DEPOBJ { } depobj_clause_optseq 
                 ;
ordered_directive : ORDERED { } ordered_clause_optseq 
                  ;
critical_clause_optseq : /*empty*/
                       | '(' critical_name')'
                       | '(' critical_name')' hint_clause
                       | '(' critical_name')' ',' hint_clause
                       ;
depobj_clause_optseq : '(' depobj ')' depobj_clause
                     ;
depobj : EXPR_STRING { }
       ;

depobj_clause : depend_depobj_clause
              | destroy_clause
              | depobj_update_clause
              ;
destroy_clause : DESTROY{}
               ;

depobj_update_clause : UPDATE '(' update_dependence_type ')'
                     ;
update_dependence_type : SOURCE { }
                       | IN { }
                       | OUT { }
                       | INOUT { }
                       | MUTEXINOUTSET { }
                       | DEPOBJ { }
                       | SINK { }
                       ;

critical_name : EXPR_STRING { }
              ;
task_clause_optseq : /* empty */
                   | task_clause_seq
                   ;
taskloop_clause_optseq : /* empty */
                       | taskloop_clause_seq
                       ;
taskloop_simd_clause_optseq : /* empty */
                            | taskloop_simd_clause_seq
                            ;
requires_clause_optseq : requires_clause_seq
                       ;
target_data_clause_optseq :target_data_clause_seq
                          ;
target_enter_data_clause_optseq :/* empty */
                                |target_enter_data_clause_seq
                                ;
target_exit_data_clause_optseq :/* empty */
                               |target_exit_data_clause_seq
                               ;
target_clause_optseq :/* empty */
                     |target_clause_seq
                     ;
target_update_clause_optseq :target_update_clause_seq
                            ;
declare_target_clause_optseq : /* empty */
                             | '(' declare_target_extended_list ')'
                             | declare_target_seq
                             ;

extended_variable : EXPR_STRING { }
                  ;
declare_target_extended_list : extended_variable
                             | declare_target_extended_list ',' extended_variable
                             ;
flush_clause_optseq : /* empty */
                    | '(' flush_list ')'
                    | flush_clause_seq
                    ;
flush_list : flush_variable
           | flush_list ',' flush_variable
           ;
flush_variable : EXPR_STRING { }
               ;
flush_clause_seq : flush_memory_order_clause
                 | flush_memory_order_clause '(' flush_list ')'
                 ;
flush_memory_order_clause : acq_rel_clause
                          | release_clause
                          | acquire_clause
                          ;

atomic_directive : ATOMIC { } atomic_clause_optseq 
                 ;
atomic_clause_optseq : memory_order_clause_seq
                     | memory_order_clause_seq atomic_clause_seq
                     | hint_clause ',' memory_order_clause ',' atomic_clause_seq
                     | memory_order_clause ',' hint_clause ',' atomic_clause_seq
                     | memory_order_clause ','atomic_clause_seq
                     | hint_clause ',' memory_order_clause atomic_clause_seq
                     | memory_order_clause ',' hint_clause atomic_clause_seq
                     | hint_clause ','atomic_clause_seq
                     ;

atomic_clause_seq : atomic_clause memory_order_clause_seq_after
                  | atomic_clause ',' memory_order_clause_seq_after
                  ;

memory_order_clause_seq : 
                        | memory_order_clause hint_clause
                        | hint_clause memory_order_clause
                        | memory_order_clause
                        | hint_clause
                        ;
memory_order_clause_seq_after : 
                              | memory_order_clause hint_clause
                              | hint_clause memory_order_clause
                              | memory_order_clause ',' hint_clause
                              | hint_clause ',' memory_order_clause
                              | memory_order_clause
                              | hint_clause
                              ;
atomic_clause : read_clause
              | write_clause
              | update_clause
              | capture_clause
              ;

memory_order_clause : seq_cst_clause
                    | acq_rel_clause
                    | release_clause
                    | acquire_clause
                    | relaxed_clause
                    ; 

hint_clause : HINT{ 
                     } '(' expression ')' 
            ;
read_clause : READ { } 
            ;
write_clause : WRITE { } 
             ;
update_clause : UPDATE { } 
              ;
capture_clause : CAPTURE { } 
               ;

seq_cst_clause : SEQ_CST { }
               ;
acq_rel_clause : ACQ_REL { }
               ;
release_clause : RELEASE { }
               ;
acquire_clause : ACQUIRE { }
               ;
relaxed_clause : RELAXED { }
               ;

taskwait_clause_optseq : /* empty */
                       | taskwait_clause_seq
                       ;
taskgroup_clause_optseq : /* empty */
                        | taskgroup_clause_seq
                        ;

task_clause_seq : task_clause
                | task_clause_seq task_clause
                | task_clause_seq ',' task_clause
                ;
taskloop_clause_seq : taskloop_clause
                    | taskloop_clause_seq taskloop_clause
                    | taskloop_clause_seq ',' taskloop_clause
                    ;
taskloop_simd_clause_seq : taskloop_simd_clause
                         | taskloop_simd_clause_seq taskloop_simd_clause
                         | taskloop_simd_clause_seq ',' taskloop_simd_clause
                         ;
requires_clause_seq : requires_clause
                    | requires_clause_seq requires_clause
                    | requires_clause_seq ',' requires_clause
                    ;

target_data_clause_seq : target_data_clause
                       | target_data_clause_seq target_data_clause
                       | target_data_clause_seq ',' target_data_clause
                       ;
target_enter_data_clause_seq : target_enter_data_clause
                             | target_enter_data_clause_seq target_enter_data_clause
                             | target_enter_data_clause_seq ',' target_enter_data_clause
                             ;
target_exit_data_clause_seq : target_exit_data_clause
                            | target_exit_data_clause_seq target_exit_data_clause
                            | target_exit_data_clause_seq ',' target_exit_data_clause
                            ;
target_clause_seq : target_clause
                  | target_clause_seq target_clause
                  | target_clause_seq ',' target_clause
                  ;
target_update_clause_seq : target_update_clause
                         | target_update_clause_seq target_update_clause
                         | target_update_clause_seq ',' target_update_clause
                         ;
declare_target_seq : declare_target_clause
                   | declare_target_seq declare_target_clause
                   | declare_target_seq ',' declare_target_clause
                   ;
taskwait_clause_seq : taskwait_clause
                    | taskwait_clause_seq taskwait_clause
                    | taskwait_clause_seq ',' taskwait_clause
                    ;
taskgroup_clause_seq : taskgroup_clause
                     | taskgroup_clause_seq taskgroup_clause
                     | taskgroup_clause_seq ',' taskgroup_clause
                     ;

task_clause : if_task_clause
            | final_clause
            | untied_clause
            | default_clause
            | mergeable_clause
            | private_clause
            | firstprivate_clause
            | shared_clause
            | in_reduction_clause
            | depend_with_modifier_clause
            | priority_clause
            | allocate_clause
            | affinity_clause
            | detach_clause
            ;
taskloop_clause : if_taskloop_clause
                | shared_clause
                | private_clause
                | firstprivate_clause
                | lastprivate_clause
                | reduction_default_only_clause
                | in_reduction_clause
                | default_clause
                | grainsize_clause
                | num_tasks_clause
                | collapse_clause
                | final_clause
                | priority_clause
                | untied_clause
                | mergeable_clause
                | nogroup_clause
                | allocate_clause
                ;
taskloop_simd_clause : if_taskloop_simd_clause
                     | shared_clause
                     | private_clause
                     | firstprivate_clause
                     | lastprivate_clause
                     | reduction_clause 
                     | in_reduction_clause
                     | default_clause
                     | grainsize_clause
                     | num_tasks_clause
                     | collapse_clause
                     | final_clause
                     | priority_clause
                     | untied_clause
                     | mergeable_clause
                     | nogroup_clause
                     | allocate_clause               
                     | safelen_clause
                     | simdlen_clause
                     | linear_clause
                     | aligned_clause
                     | nontemporal_clause
                     | order_clause 
                     ;
requires_clause : reverse_offload_clause
                | unified_address_clause
                | unified_shared_memory_clause   
                | atomic_default_mem_order_clause 
                | dynamic_allocators_clause
                | ext_implementation_defined_requirement_clause       
                ;
target_data_clause : if_target_data_clause
                   | device_clause
                   | map_clause
                   | use_device_ptr_clause
                   | use_device_addr_clause
                   ;
target_enter_data_clause: if_target_enter_data_clause
                        | device_clause
                        | map_clause
                        | depend_with_modifier_clause
                        | nowait_clause
                        ;
target_exit_data_clause: if_target_exit_data_clause
                       | device_clause
                       | map_clause
                       | depend_with_modifier_clause
                       | nowait_clause
                       ;
target_clause: if_target_clause
             | device_clause
             | thread_limit_clause
             | private_clause
             | firstprivate_clause
             | in_reduction_clause
             | map_clause
             | is_device_ptr_clause
             | has_device_addr_clause
             | defaultmap_clause
             | nowait_clause
             | allocate_clause
             | depend_with_modifier_clause
             | uses_allocators_clause
             ;
target_update_clause: motion_clause
                    | target_update_other_clause
                    ;
motion_clause: to_clause
             | from_clause
             ;
target_update_other_clause: if_target_update_clause
                          | device_without_modifier_clause
                          | depend_with_modifier_clause
                          | nowait_clause
                          ;
declare_target_clause : to_clause
                      | link_clause
                      | device_type_clause
                      ;
taskwait_clause : depend_with_modifier_clause
                ;
taskgroup_clause : task_reduction_clause
                 | allocate_clause
                 ;
final_clause: FINAL { } '(' expression ')'
            ;
untied_clause: UNTIED { } 
             ;
mergeable_clause: MERGEABLE { } 
                ;
in_reduction_clause : IN_REDUCTION '(' in_reduction_identifier ':' var_list ')' { }
                    ;
in_reduction_identifier : in_reduction_enum_identifier
                        | EXPR_STRING { }
                        ;

in_reduction_enum_identifier :  '+'{ }
                             | '-'{ }
                             | '*'{ }
                             | '&'{ }
                             | '|'{ }
                             | '^'{ }
                             | LOGAND{ }
                             | LOGOR{ }
                             | MAX{ }
                             | MIN{ }
                             ;

depend_with_modifier_clause : DEPEND { } '(' depend_parameter ':' var_list ')' { }
                            ;

depend_parameter : dependence_type
                 | depend_modifier ',' dependence_type { }
                 ;
dependence_type : depend_enum_type 
                ;
depend_modifier : MODIFIER_ITERATOR { } '('depend_iterators_definition ')'
                ;
depend_iterators_definition : depend_iterator_specifier
                            | depend_iterators_definition ',' depend_iterator_specifier
                            ;
depend_iterator_specifier : EXPR_STRING EXPR_STRING { } '=' depend_range_specification
                          | EXPR_STRING { } '=' depend_range_specification
                          ;
depend_range_specification : EXPR_STRING { } ':' EXPR_STRING { } depend_range_step { }
                           ;
depend_range_step : /*empty*/ { }
                  | ':' EXPR_STRING { }
                  ;
depend_enum_type : IN { }
                 | OUT { }
                 | INOUT { }
                 | MUTEXINOUTSET { }
                 | DEPOBJ { }
                 ;

depend_depobj_clause : DEPEND { }'(' dependence_depobj_parameter ')' {
}
                     ;
dependence_depobj_parameter : dependence_depobj_type ':' expression
                            ;
dependence_depobj_type : IN             { }
                       | OUT            { }
                       | INOUT          { }
                       | MUTEXINOUTSET  { }
                       ;
depend_ordered_clause : DEPEND { }'(' dependence_ordered_parameter ')' {
}
                      ;
dependence_ordered_parameter : dependence_ordered_type
                             ;
dependence_ordered_type :  SOURCE { }
                        | SINK { } ':' var_list
                        ;

priority_clause: PRIORITY { } '(' expression ')'
               ;

affinity_clause: AFFINITY '(' affinity_parameter ')' ;

affinity_parameter : EXPR_STRING { }
                   | EXPR_STRING ',' { } var_list
                   | affinity_modifier ':' var_list
                   ;

affinity_modifier : MODIFIER_ITERATOR { }'('iterators_definition')'{}
                  ;
iterators_definition : iterator_specifier
                     | iterators_definition ',' iterator_specifier
                     ;
iterator_specifier : EXPR_STRING EXPR_STRING { } '=' range_specification
                   | EXPR_STRING{ }  '=' range_specification
                   ;
range_specification : EXPR_STRING { } ':' EXPR_STRING { } range_step { }

                    ;
range_step : /*empty*/ { }
           | ':' EXPR_STRING { }
           ;

detach_clause: DETACH { } '(' expression ')'
             ;
grainsize_clause: GRAINSIZE { } '(' expression ')'
                ;
num_tasks_clause: NUM_TASKS { } '(' expression ')'
                ;
nogroup_clause: NOGROUP { } 
              ;
reverse_offload_clause: REVERSE_OFFLOAD { } 
                      ;
unified_address_clause: UNIFIED_ADDRESS { } 
                      ;
unified_shared_memory_clause: UNIFIED_SHARED_MEMORY { } 
                      ;
atomic_default_mem_order_clause : ATOMIC_DEFAULT_MEM_ORDER '(' atomic_default_mem_order_parameter ')' { } ;

atomic_default_mem_order_parameter : SEQ_CST { }
                                   | ACQ_REL { }
                                   | RELAXED { }
                                   ;
dynamic_allocators_clause: DYNAMIC_ALLOCATORS { } 
                         ;
ext_implementation_defined_requirement_clause: EXT_ EXPR_STRING { }
                                             ;
device_clause : DEVICE '(' device_parameter ')' ;

device_parameter : EXPR_STRING  { }
                 | EXPR_STRING ',' { } var_list
                 | device_modifier_parameter ':' var_list
                 ;

device_modifier_parameter : ANCESTOR { }
                          | DEVICE_NUM { }
                          ;
                          
device_without_modifier_clause : DEVICE '(' device_without_modifier_parameter ')' ;

device_without_modifier_parameter : EXPR_STRING  { }
                                  | EXPR_STRING ',' { } var_list
                                  ;

use_device_ptr_clause : USE_DEVICE_PTR { } '(' var_list ')'
                      ;

use_device_addr_clause : USE_DEVICE_ADDR { } '(' var_list ')'
                       ;
is_device_ptr_clause : IS_DEVICE_PTR { } '(' var_list ')' {
}
                     ;
                     
has_device_addr_clause : HAS_DEVICE_ADDR { } '(' var_list ')' {
}
                     ;
defaultmap_clause : DEFAULTMAP{ } '('  defaultmap_parameter ')'
                  ;
defaultmap_parameter : defaultmap_behavior { }
                     | defaultmap_behavior ':' defaultmap_category
                     ;

defaultmap_behavior : BEHAVIOR_ALLOC { }
                    | BEHAVIOR_TO { }
                    | BEHAVIOR_FROM { }
                    | BEHAVIOR_TOFROM {}
                    | BEHAVIOR_FIRSTPRIVATE { }
                    | BEHAVIOR_NONE { }
                    | BEHAVIOR_DEFAULT { }
                    ;
defaultmap_category : CATEGORY_SCALAR { }
                    | CATEGORY_AGGREGATE { }
                    | CATEGORY_POINTER { }
                    | CATEGORY_ALLOCATABLE { }
                    ;
uses_allocators_clause : USES_ALLOCATORS  { } '(' uses_allocators_parameter ')' ;
uses_allocators_parameter : allocators_list
                          | allocators_list ',' uses_allocators_parameter
                          ;

allocators_list : allocators_list_parameter_enum { }
                | allocators_list_parameter_enum '(' EXPR_STRING ')' { }
                | allocators_list_parameter_user { }
                | allocators_list_parameter_user '(' EXPR_STRING ')' { }
                ;

allocators_list_parameter_enum : DEFAULT_MEM_ALLOC { }
                               | LARGE_CAP_MEM_ALLOC { }
                               | CONST_MEM_ALLOC { }
                               | HIGH_BW_MEM_ALLOC { }
                               | LOW_LAT_MEM_ALLOC { }
                               | CGROUP_MEM_ALLOC { }
                               | PTEAM_MEM_ALLOC { }
                               | THREAD_MEM_ALLOC { }
                               ;
allocators_list_parameter_user : EXPR_STRING { }
                               ;
to_clause: TO '(' to_parameter ')' ;
to_parameter : EXPR_STRING  { }
             | EXPR_STRING ',' { } var_list
             | to_mapper ':' var_list
             ;
to_mapper : TO_MAPPER { }'('EXPR_STRING')' { }
          ;

from_clause: FROM '(' from_parameter ')' ;
from_parameter : EXPR_STRING { }
               | EXPR_STRING ',' { } var_list
               | from_mapper ':' var_list
               ;
from_mapper : FROM_MAPPER { }'('EXPR_STRING')' { }
            ;
link_clause : LINK {
} '(' var_list ')' { }
  ;
device_type_clause : DEVICE_TYPE '(' device_type_parameter ')' { } ;

device_type_parameter : HOST { }
                    | NOHOST { }
                    | ANY { }
                    ;

map_clause : MAP { }'(' map_parameter')';

map_parameter : EXPR_STRING { }
              | EXPR_STRING ',' { } var_list
              | map_modifier_type ':' var_list
              ;
map_modifier_type : map_type
                  | map_modifier1 map_type
                  | map_modifier1 ',' map_type
                  | map_modifier1 ',' map_modifier_parameter1
                  | map_modifier1 map_modifier_parameter1                  
                  ;
map_modifier_parameter1 : map_modifier2 map_type
                        | map_modifier2 ',' map_type
                        | map_modifier2 map_modifier_parameter2
                        | map_modifier2 ',' map_modifier_parameter2
                        ;
map_modifier_parameter2 : map_modifier3 map_type
                        | map_modifier3 ',' map_type
                        ; 

map_modifier1 : MAP_MODIFIER_ALWAYS { }
              | MAP_MODIFIER_CLOSE  { }
              | map_modifier_mapper { }
              ;
map_modifier2 : MAP_MODIFIER_ALWAYS { }
              | MAP_MODIFIER_CLOSE  { }
              | map_modifier_mapper { }
              ;
map_modifier3 : MAP_MODIFIER_ALWAYS { }
              | MAP_MODIFIER_CLOSE  { }
              | map_modifier_mapper { }
              ;
map_type : MAP_TYPE_TO { }
         | MAP_TYPE_FROM { }
         | MAP_TYPE_TOFROM { }
         | MAP_TYPE_ALLOC { }
         | MAP_TYPE_RELEASE { }
         | MAP_TYPE_DELETE { }
         ;
map_modifier_mapper : MAP_MODIFIER_MAPPER '('EXPR_STRING')' { }
                    ;

task_reduction_clause : TASK_REDUCTION '(' task_reduction_identifier ':' var_list ')' { }
                      ;
task_reduction_identifier : task_reduction_enum_identifier
                          | EXPR_STRING { }
                          ;

task_reduction_enum_identifier : '+' { }
                               | '-' { }
                               | '*' { }
                               | '&' { }
                               | '|' { }
                               | '^' { }
                               | LOGAND { }
                               | LOGOR { }
                               | MAX { }
                               | MIN { }
                               ;
ordered_clause_optseq : /* empty */
                      | ordered_clause_threads_simd_seq
                      | ordered_clause_depend_seq
                      ;
ordered_clause_threads_simd_seq : ordered_clause_threads_simd
                                | ordered_clause_threads_simd_seq ordered_clause_threads_simd
                                | ordered_clause_threads_simd_seq ',' ordered_clause_threads_simd
                                ;
ordered_clause_depend_seq : ordered_clause_depend
                          | ordered_clause_depend_seq ordered_clause_depend
                          | ordered_clause_depend_seq ',' ordered_clause_depend
                          ;
ordered_clause_depend : depend_ordered_clause
                      ;
ordered_clause_threads_simd : threads_clause
                            | simd_ordered_clause
                            ;
threads_clause : THREADS { } 
               ;
simd_ordered_clause : SIMD { } 
                    ;
teams_distribute_directive : TEAMS DISTRIBUTE {if (!cluster_stack_isEmpty()) {enDistribute = 1;}} teams_distribute_clause_optseq
                           ;
teams_distribute_clause_optseq : /* empty */
                               | teams_distribute_clause_seq
                               ;
teams_distribute_clause_seq : teams_distribute_clause
                            | teams_distribute_clause_seq teams_distribute_clause
                            | teams_distribute_clause_seq ',' teams_distribute_clause
                            ;
teams_distribute_clause : num_teams_clause
                        | thread_limit_clause
                        | default_clause
                        | private_clause
                        | firstprivate_clause
                        | shared_clause
                        | {enReductionDistribute = 1;} reduction_clause_cluster
                        | allocate_clause              
                        | lastprivate_distribute_clause
                        | collapse_clause
                        | dist_schedule_clause_cluster
                        ;
teams_distribute_simd_directive :  TEAMS DISTRIBUTE SIMD {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 1; guardarLineasDist += "#pragma omp simd";}} teams_distribute_simd_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}} 
                                ;
teams_distribute_simd_clause_optseq : /* empty */
                                    | teams_distribute_simd_clause_seq
                                    ;
teams_distribute_simd_clause_seq : teams_distribute_simd_clause
                                 | teams_distribute_simd_clause_seq teams_distribute_simd_clause
                                 | teams_distribute_simd_clause_seq ',' teams_distribute_simd_clause
                                 ;
teams_distribute_simd_clause : num_teams_clause
                             | thread_limit_clause
                             | default_clause
                             | private_clause
                             | firstprivate_clause
                             | shared_clause
                             | reduction_clause
                             | allocate_clause
                             | lastprivate_clause
                             | collapse_clause
                             | dist_schedule_clause_cluster
                             | if_simd_clause
                             | safelen_clause
                             | simdlen_clause
                             | linear_clause
                             | aligned_clause
                             | nontemporal_clause
                             | order_clause
                             ;
teams_distribute_parallel_for_directive :  TEAMS DISTRIBUTE PARALLEL FOR {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 1; guardarLineasDist += "#pragma omp parallel for";}} teams_distribute_parallel_for_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                                        ;
teams_distribute_parallel_for_clause_optseq : /* empty */
                                            | teams_distribute_parallel_for_clause_seq
                                            ;
teams_distribute_parallel_for_clause_seq : teams_distribute_parallel_for_clause
                                         | teams_distribute_parallel_for_clause_seq teams_distribute_parallel_for_clause
                                         | teams_distribute_parallel_for_clause_seq ',' teams_distribute_parallel_for_clause
                                         ;
teams_distribute_parallel_for_clause : num_teams_clause
                                     | thread_limit_clause
                                     | default_clause
                                     | private_clause
                                     | firstprivate_clause
                                     | shared_clause
                                     | reduction_clause
                                     | allocate_clause
                                     | if_parallel_clause
                                     | num_threads_clause                   
                                     | copyin_clause                            
                                     | proc_bind_clause                      
                                     | lastprivate_clause 
                                     | linear_clause
                                     | schedule_clause
                                     | collapse_clause
                                     | ordered_clause
                                     | nowait_clause
                                     | order_clause 
                                     | dist_schedule_clause_cluster                                   
                                     ;
teams_distribute_parallel_do_directive :  TEAMS DISTRIBUTE PARALLEL DO {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 1; guardarLineasDist += "#pragma omp parallel do";}} teams_distribute_parallel_do_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                                       ;
teams_distribute_parallel_do_clause_optseq : /* empty */
                                           | teams_distribute_parallel_do_clause_seq
                                           ;
teams_distribute_parallel_do_clause_seq : teams_distribute_parallel_do_clause
                                        | teams_distribute_parallel_do_clause_seq teams_distribute_parallel_do_clause
                                        | teams_distribute_parallel_do_clause_seq ',' teams_distribute_parallel_do_clause
                                        ;
teams_distribute_parallel_do_clause : num_teams_clause
                                    | thread_limit_clause
                                    | default_clause
                                    | private_clause
                                    | firstprivate_clause
                                    | shared_clause
                                    | reduction_clause
                                    | allocate_clause
                                    | if_parallel_clause
                                    | num_threads_clause                   
                                    | copyin_clause                            
                                    | proc_bind_clause                      
                                    | lastprivate_clause 
                                    | linear_clause
                                    | schedule_clause
                                    | collapse_clause
                                    | ordered_clause
                                    | nowait_clause
                                    | order_clause 
                                    | dist_schedule_clause_cluster
                                     ;
teams_distribute_parallel_for_simd_directive : TEAMS DISTRIBUTE PARALLEL FOR SIMD {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 1; guardarLineasDist += "#pragma omp parallel for simd";}} teams_distribute_parallel_for_simd_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                                             ;
teams_distribute_parallel_for_simd_clause_optseq : /* empty */
                                                 | teams_distribute_parallel_for_simd_clause_seq
                                                 ;
teams_distribute_parallel_for_simd_clause_seq : teams_distribute_parallel_for_simd_clause
                                              | teams_distribute_parallel_for_simd_clause_seq teams_distribute_parallel_for_simd_clause
                                              | teams_distribute_parallel_for_simd_clause_seq ',' teams_distribute_parallel_for_simd_clause
                                              ;
teams_distribute_parallel_for_simd_clause : num_teams_clause
                                          | thread_limit_clause
                                          | default_clause
                                          | private_clause
                                          | firstprivate_clause
                                          | shared_clause
                                          | reduction_clause
                                          | allocate_clause
                                          | if_parallel_simd_clause
                                          | num_threads_clause
                                          | copyin_clause                               
                                          | proc_bind_clause                                  
                                          | lastprivate_clause 
                                          | linear_clause
                                          | schedule_clause
                                          | collapse_clause
                                          | ordered_clause
                                          | nowait_clause
                                          | order_clause 
                                          | dist_schedule_clause_cluster
                                          | safelen_clause
                                          | simdlen_clause
                                          | aligned_clause
                                          | nontemporal_clause
                                          ;
teams_distribute_parallel_do_simd_directive : TEAMS DISTRIBUTE PARALLEL DO SIMD {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 1; guardarLineasDist += "#pragma omp parallel do simd";}} teams_distribute_parallel_do_simd_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                                            ;
teams_distribute_parallel_do_simd_clause_optseq : /* empty */
                                                | teams_distribute_parallel_do_simd_clause_seq
                                                ;
teams_distribute_parallel_do_simd_clause_seq : teams_distribute_parallel_do_simd_clause
                                             | teams_distribute_parallel_do_simd_clause_seq teams_distribute_parallel_do_simd_clause
                                             | teams_distribute_parallel_do_simd_clause_seq ',' teams_distribute_parallel_do_simd_clause
                                             ;
teams_distribute_parallel_do_simd_clause : num_teams_clause
                                         | thread_limit_clause
                                         | default_clause
                                         | private_clause
                                         | firstprivate_clause
                                         | shared_clause
                                         | reduction_clause
                                         | allocate_clause
                                         | if_parallel_simd_clause
                                         | num_threads_clause
                                         | copyin_clause                               
                                         | proc_bind_clause                                  
                                         | lastprivate_clause 
                                         | linear_clause
                                         | schedule_clause
                                         | collapse_clause
                                         | ordered_clause
                                         | nowait_clause
                                         | order_clause 
                                         | dist_schedule_clause_cluster
                                         | safelen_clause
                                         | simdlen_clause
                                         | aligned_clause
                                         | nontemporal_clause
                                         ;
teams_loop_directive : TEAMS LOOP{ } teams_loop_clause_optseq 
                     ;
teams_loop_clause_optseq : /* empty */
                         | teams_loop_clause_seq
                         ;
teams_loop_clause_seq : teams_loop_clause
                      | teams_loop_clause_seq teams_loop_clause
                      | teams_loop_clause_seq ',' teams_loop_clause
                      ;
teams_loop_clause : num_teams_clause
                  | thread_limit_clause
                  | default_clause
                  | private_clause
                  | firstprivate_clause
                  | shared_clause
                  | reduction_default_only_clause
                  | allocate_clause
                  | bind_clause
                  | collapse_clause
                  | order_clause
                  | lastprivate_clause
                  ;
target_parallel_directive : TARGET PARALLEL{ } target_parallel_clause_optseq 
                          ;
target_parallel_clause_optseq : /* empty */
                              | target_parallel_clause_seq
                              ;
target_parallel_clause_seq : target_parallel_clause
                           | target_parallel_clause_seq target_parallel_clause
                           | target_parallel_clause_seq ',' target_parallel_clause
                           ;
target_parallel_clause : if_target_parallel_clause
                       | device_clause
                       | private_clause
                       | firstprivate_clause
                       | in_reduction_clause
                       | map_clause
                       | is_device_ptr_clause
                       | defaultmap_clause
                       | nowait_clause
                       | allocate_clause
                       | depend_with_modifier_clause
                       | uses_allocators_clause
                       | num_threads_clause
                       | default_clause
                       | shared_clause
                       | copyin_clause
                       | reduction_clause
                       | proc_bind_clause
                       ;

target_parallel_for_directive : TARGET PARALLEL FOR{ } target_parallel_for_clause_optseq 
                              ;
target_parallel_for_clause_optseq : /* empty */
                                  | target_parallel_for_clause_seq
                                  ;
target_parallel_for_clause_seq : target_parallel_for_clause
                               | target_parallel_for_clause_seq target_parallel_for_clause
                               | target_parallel_for_clause_seq ',' target_parallel_for_clause
                               ;
target_parallel_for_clause : if_target_parallel_clause
                           | device_clause
                           | private_clause
                           | firstprivate_clause
                           | in_reduction_clause
                           | map_clause
                           | is_device_ptr_clause
                           | defaultmap_clause
                           | nowait_clause
                           | allocate_clause
                           | depend_with_modifier_clause
                           | uses_allocators_clause
                           | num_threads_clause
                           | default_clause                          
                           | shared_clause
                           | copyin_clause
                           | reduction_clause
                           | proc_bind_clause
                           | lastprivate_clause 
                           | linear_clause
                           | schedule_clause
                           | collapse_clause
                           | ordered_clause
                           | order_clause 
                           | dist_schedule_clause
                           ;
target_parallel_do_directive : TARGET PARALLEL DO{ } target_parallel_do_clause_optseq 
                             ;
target_parallel_do_clause_optseq : /* empty */
                                 | target_parallel_do_clause_seq
                                 ;
target_parallel_do_clause_seq : target_parallel_do_clause
                              | target_parallel_do_clause_seq target_parallel_do_clause
                              | target_parallel_do_clause_seq ',' target_parallel_do_clause
                              ;
target_parallel_do_clause : if_target_parallel_clause
                          | device_clause
                          | private_clause
                          | firstprivate_clause
                          | in_reduction_clause
                          | map_clause
                          | is_device_ptr_clause
                          | defaultmap_clause
                          | nowait_clause
                          | allocate_clause
                          | depend_with_modifier_clause
                          | uses_allocators_clause
                          | num_threads_clause
                          | default_clause                          
                          | shared_clause
                          | copyin_clause
                          | reduction_clause
                          | proc_bind_clause
                          | lastprivate_clause 
                          | linear_clause
                          | schedule_clause
                          | collapse_clause
                          | ordered_clause
                          | order_clause 
                          | dist_schedule_clause
                          ;
target_parallel_for_simd_directive : TARGET PARALLEL FOR SIMD{ } target_parallel_for_simd_clause_optseq 
                                   ;
target_parallel_for_simd_clause_optseq : /* empty */
                                       | target_parallel_for_simd_clause_seq
                                       ;
target_parallel_for_simd_clause_seq : target_parallel_for_simd_clause
                                    | target_parallel_for_simd_clause_seq target_parallel_for_simd_clause
                                    | target_parallel_for_simd_clause_seq ',' target_parallel_for_simd_clause
                                    ;
target_parallel_for_simd_clause : if_target_parallel_simd_clause
                                | device_clause
                                | private_clause
                                | firstprivate_clause
                                | in_reduction_clause
                                | map_clause
                                | is_device_ptr_clause
                                | defaultmap_clause
                                | nowait_clause
                                | allocate_clause
                                | depend_with_modifier_clause
                                | uses_allocators_clause
                                | num_threads_clause
                                | default_clause                    
                                | shared_clause
                                | copyin_clause
                                | reduction_clause
                                | proc_bind_clause                       
                                | lastprivate_clause 
                                | linear_clause
                                | schedule_clause
                                | collapse_clause
                                | ordered_clause                        
                                | order_clause
                                | safelen_clause
                                | simdlen_clause
                                | aligned_clause
                                | nontemporal_clause
                                ;
target_parallel_do_simd_directive : TARGET PARALLEL DO SIMD{ } target_parallel_do_simd_clause_optseq 
                                  ;
target_parallel_do_simd_clause_optseq : /* empty */
                                      | target_parallel_do_simd_clause_seq
                                      ;
target_parallel_do_simd_clause_seq : target_parallel_do_simd_clause
                                   | target_parallel_do_simd_clause_seq target_parallel_do_simd_clause
                                   | target_parallel_do_simd_clause_seq ',' target_parallel_do_simd_clause
                                   ;
target_parallel_do_simd_clause : if_target_parallel_simd_clause
                               | device_clause
                               | private_clause
                               | firstprivate_clause
                               | in_reduction_clause
                               | map_clause
                               | is_device_ptr_clause
                               | defaultmap_clause
                               | nowait_clause
                               | allocate_clause
                               | depend_with_modifier_clause
                               | uses_allocators_clause
                               | num_threads_clause
                               | default_clause                    
                               | shared_clause
                               | copyin_clause
                               | reduction_clause
                               | proc_bind_clause                       
                               | lastprivate_clause 
                               | linear_clause
                               | schedule_clause
                               | collapse_clause
                               | ordered_clause                        
                               | order_clause
                               | safelen_clause
                               | simdlen_clause
                               | aligned_clause
                               | nontemporal_clause
                               ;
target_parallel_loop_directive : TARGET PARALLEL LOOP{ } target_parallel_loop_clause_optseq 
                               ;
target_parallel_loop_clause_optseq : /* empty */
                                   | target_parallel_loop_clause_seq
                                   ;
target_parallel_loop_clause_seq : target_parallel_loop_clause
                                | target_parallel_loop_clause_seq target_parallel_loop_clause
                                | target_parallel_loop_clause_seq ',' target_parallel_loop_clause
                                ;
target_parallel_loop_clause : if_target_parallel_clause
                            | device_clause
                            | private_clause
                            | firstprivate_clause
                            | in_reduction_clause
                            | map_clause
                            | is_device_ptr_clause
                            | defaultmap_clause
                            | nowait_clause
                            | allocate_clause
                            | depend_with_modifier_clause
                            | uses_allocators_clause
                            | num_threads_clause
                            | default_clause             
                            | shared_clause
                            | copyin_clause
                            | reduction_clause
                            | proc_bind_clause                   
                            | lastprivate_clause 
                            | collapse_clause
                            | bind_clause
                            | order_clause 
                            ;
target_simd_directive : TARGET SIMD{ } target_simd_clause_optseq 
                      ;
target_simd_clause_optseq : /* empty */
                          | target_simd_clause_seq
                          ;
target_simd_clause_seq : target_simd_clause
                       | target_simd_clause_seq target_simd_clause
                       | target_simd_clause_seq ',' target_simd_clause
                       ;
target_simd_clause : if_target_simd_clause
                   | device_clause
                   | private_clause
                   | firstprivate_clause
                   | in_reduction_clause
                   | map_clause
                   | is_device_ptr_clause
                   | defaultmap_clause
                   | nowait_clause
                   | allocate_clause
                   | depend_with_modifier_clause
                   | uses_allocators_clause
                   | safelen_clause
                   | simdlen_clause
                   | linear_clause
                   | aligned_clause
                   | nontemporal_clause
                   | lastprivate_clause
                   | reduction_clause
                   | collapse_clause
                   | order_clause
                   ;
target_teams_directive : TARGET TEAMS{ } target_teams_clause_optseq 
                       ;
target_teams_clause_optseq : /* empty */
                           | target_teams_clause_seq
                           ;
target_teams_clause_seq : target_teams_clause
                        | target_teams_clause_seq target_teams_clause
                        | target_teams_clause_seq ',' target_teams_clause
                        ;
target_teams_clause : if_target_clause
                    | device_clause
                    | private_clause
                    | firstprivate_clause
                    | in_reduction_clause
                    | map_clause
                    | is_device_ptr_clause
                    | defaultmap_clause
                    | nowait_clause
                    | allocate_clause
                    | depend_with_modifier_clause
                    | uses_allocators_clause
                    | num_teams_clause
                    | thread_limit_clause
                    | default_clause
                    | shared_clause
                    | reduction_default_only_clause
                    ;
target_teams_distribute_directive : TARGET TEAMS DISTRIBUTE{ } target_teams_distribute_clause_optseq 
                                  ;
target_teams_distribute_clause_optseq : /* empty */
                                      | target_teams_distribute_clause_seq
                                      ;
target_teams_distribute_clause_seq : target_teams_distribute_clause
                                   | target_teams_distribute_clause_seq target_teams_distribute_clause
                                   | target_teams_distribute_clause_seq ',' target_teams_distribute_clause
                                   ;
target_teams_distribute_clause : if_target_clause
                               | device_clause
                               | private_clause
                               | firstprivate_clause
                               | in_reduction_clause
                               | map_clause
                               | is_device_ptr_clause
                               | defaultmap_clause
                               | nowait_clause
                               | allocate_clause
                               | depend_with_modifier_clause
                               | uses_allocators_clause
                               | num_teams_clause
                               | thread_limit_clause
                               | default_clause                   
                               | shared_clause
                               | reduction_default_only_clause
                               | lastprivate_distribute_clause
                               | collapse_clause
                               | dist_schedule_clause
                               ;
target_teams_distribute_simd_directive : TARGET TEAMS DISTRIBUTE SIMD{ } target_teams_distribute_simd_clause_optseq 
                                       ;
target_teams_distribute_simd_clause_optseq : /* empty */
                                           | target_teams_distribute_simd_clause_seq
                                           ;
target_teams_distribute_simd_clause_seq : target_teams_distribute_simd_clause
                                        | target_teams_distribute_simd_clause_seq target_teams_distribute_simd_clause
                                        | target_teams_distribute_simd_clause_seq ',' target_teams_distribute_simd_clause
                                        ;
target_teams_distribute_simd_clause : if_target_simd_clause
                                    | device_clause
                                    | private_clause
                                    | firstprivate_clause
                                    | in_reduction_clause
                                    | map_clause
                                    | is_device_ptr_clause
                                    | defaultmap_clause
                                    | nowait_clause
                                    | allocate_clause
                                    | depend_with_modifier_clause
                                    | uses_allocators_clause 
                                    | num_teams_clause
                                    | thread_limit_clause
                                    | default_clause
                                    | shared_clause
                                    | reduction_clause
                                    | lastprivate_clause
                                    | collapse_clause
                                    | dist_schedule_clause
                                    | safelen_clause
                                    | simdlen_clause
                                    | linear_clause
                                    | aligned_clause
                                    | nontemporal_clause
                                    | order_clause
                                    ;
target_teams_loop_directive : TARGET TEAMS LOOP{ } target_teams_loop_clause_optseq 
                            ;
target_teams_loop_clause_optseq : /* empty */
                                | target_teams_loop_clause_seq
                                ;
target_teams_loop_clause_seq : target_teams_loop_clause
                             | target_teams_loop_clause_seq target_teams_loop_clause
                             | target_teams_loop_clause_seq ',' target_teams_loop_clause
                             ;
target_teams_loop_clause : if_target_clause
                         | device_clause
                         | private_clause
                         | firstprivate_clause
                         | in_reduction_clause
                         | map_clause
                         | is_device_ptr_clause
                         | defaultmap_clause
                         | nowait_clause
                         | allocate_clause
                         | depend_with_modifier_clause
                         | uses_allocators_clause 
                         | num_teams_clause
                         | thread_limit_clause
                         | default_clause
                         | shared_clause
                         | reduction_default_only_clause                                 
                         | bind_clause
                         | collapse_clause
                         | order_clause
                         | lastprivate_clause
                         ;
target_teams_distribute_parallel_for_directive : TARGET TEAMS DISTRIBUTE PARALLEL FOR{ } target_teams_distribute_parallel_for_clause_optseq 
                                               ;
target_teams_distribute_parallel_for_clause_optseq : /* empty */
                                                   | target_teams_distribute_parallel_for_clause_seq
                                                   ;
target_teams_distribute_parallel_for_clause_seq : target_teams_distribute_parallel_for_clause
                                                | target_teams_distribute_parallel_for_clause_seq target_teams_distribute_parallel_for_clause
                                                | target_teams_distribute_parallel_for_clause_seq ',' target_teams_distribute_parallel_for_clause
                                                ;
target_teams_distribute_parallel_for_clause : if_target_parallel_clause
                                            | device_clause
                                            | private_clause
                                            | firstprivate_clause
                                            | in_reduction_clause
                                            | map_clause
                                            | is_device_ptr_clause
                                            | defaultmap_clause
                                            | nowait_clause
                                            | allocate_clause
                                            | depend_with_modifier_clause
                                            | uses_allocators_clause 
                                            | num_teams_clause
                                            | thread_limit_clause
                                            | default_clause                                 
                                            | shared_clause
                                            | reduction_clause                            
                                            | num_threads_clause                   
                                            | copyin_clause                            
                                            | proc_bind_clause                      
                                            | lastprivate_clause 
                                            | linear_clause
                                            | schedule_clause
                                            | collapse_clause
                                            | ordered_clause                                
                                            | order_clause 
                                            | dist_schedule_clause
                                            ;
target_teams_distribute_parallel_do_directive : TARGET TEAMS DISTRIBUTE PARALLEL DO{ } target_teams_distribute_parallel_do_clause_optseq 
                                              ;
target_teams_distribute_parallel_do_clause_optseq : /* empty */
                                                  | target_teams_distribute_parallel_do_clause_seq
                                                  ;
target_teams_distribute_parallel_do_clause_seq : target_teams_distribute_parallel_do_clause
                                               | target_teams_distribute_parallel_do_clause_seq target_teams_distribute_parallel_do_clause
                                               | target_teams_distribute_parallel_do_clause_seq ',' target_teams_distribute_parallel_do_clause
                                               ;
target_teams_distribute_parallel_do_clause : if_target_parallel_clause
                                           | device_clause
                                           | private_clause
                                           | firstprivate_clause
                                           | in_reduction_clause
                                           | map_clause
                                           | is_device_ptr_clause
                                           | defaultmap_clause
                                           | nowait_clause
                                           | allocate_clause
                                           | depend_with_modifier_clause
                                           | uses_allocators_clause 
                                           | num_teams_clause
                                           | thread_limit_clause
                                           | default_clause                                 
                                           | shared_clause
                                           | reduction_clause                            
                                           | num_threads_clause                   
                                           | copyin_clause                            
                                           | proc_bind_clause                      
                                           | lastprivate_clause 
                                           | linear_clause
                                           | schedule_clause
                                           | collapse_clause
                                           | ordered_clause                                
                                           | order_clause 
                                           | dist_schedule_clause
                                           ;
target_teams_distribute_parallel_for_simd_directive : TARGET TEAMS DISTRIBUTE PARALLEL FOR SIMD{ } target_teams_distribute_parallel_for_simd_clause_optseq 
                                                    ;
target_teams_distribute_parallel_for_simd_clause_optseq : /* empty */
                                                        | target_teams_distribute_parallel_for_simd_clause_seq
                                                        ;
target_teams_distribute_parallel_for_simd_clause_seq : target_teams_distribute_parallel_for_simd_clause
                                                     | target_teams_distribute_parallel_for_simd_clause_seq target_teams_distribute_parallel_for_simd_clause
                                                     | target_teams_distribute_parallel_for_simd_clause_seq ',' target_teams_distribute_parallel_for_simd_clause
                                                     ;
target_teams_distribute_parallel_for_simd_clause : if_target_parallel_simd_clause
                                                 | device_clause
                                                 | private_clause
                                                 | firstprivate_clause
                                                 | in_reduction_clause
                                                 | map_clause
                                                 | is_device_ptr_clause
                                                 | defaultmap_clause
                                                 | nowait_clause
                                                 | allocate_clause
                                                 | depend_with_modifier_clause
                                                 | uses_allocators_clause 
                                                 | num_teams_clause
                                                 | thread_limit_clause
                                                 | default_clause                                     
                                                 | shared_clause
                                                 | reduction_clause
                                                 | num_threads_clause
                                                 | copyin_clause                               
                                                 | proc_bind_clause                                  
                                                 | lastprivate_clause 
                                                 | linear_clause
                                                 | schedule_clause
                                                 | collapse_clause
                                                 | ordered_clause                          
                                                 | order_clause 
                                                 | dist_schedule_clause
                                                 | safelen_clause
                                                 | simdlen_clause
                                                 | aligned_clause
                                                 | nontemporal_clause
                                                 ;
target_teams_distribute_parallel_do_simd_directive : TARGET TEAMS DISTRIBUTE PARALLEL DO SIMD{ } target_teams_distribute_parallel_do_simd_clause_optseq 
                                                   ;
target_teams_distribute_parallel_do_simd_clause_optseq : /* empty */
                                                       | target_teams_distribute_parallel_do_simd_clause_seq
                                                       ;
target_teams_distribute_parallel_do_simd_clause_seq : target_teams_distribute_parallel_do_simd_clause
                                                    | target_teams_distribute_parallel_do_simd_clause_seq target_teams_distribute_parallel_do_simd_clause
                                                    | target_teams_distribute_parallel_do_simd_clause_seq ',' target_teams_distribute_parallel_do_simd_clause
                                                     ;
target_teams_distribute_parallel_do_simd_clause : if_target_parallel_simd_clause
                                                | device_clause
                                                | private_clause
                                                | firstprivate_clause
                                                | in_reduction_clause
                                                | map_clause
                                                | is_device_ptr_clause
                                                | defaultmap_clause
                                                | nowait_clause
                                                | allocate_clause
                                                | depend_with_modifier_clause
                                                | uses_allocators_clause 
                                                | num_teams_clause
                                                | thread_limit_clause
                                                | default_clause                                     
                                                | shared_clause
                                                | reduction_clause
                                                | num_threads_clause
                                                | copyin_clause                               
                                                | proc_bind_clause                                  
                                                | lastprivate_clause 
                                                | linear_clause
                                                | schedule_clause
                                                | collapse_clause
                                                | ordered_clause                          
                                                | order_clause 
                                                | dist_schedule_clause
                                                | safelen_clause
                                                | simdlen_clause
                                                | aligned_clause
                                                | nontemporal_clause
                                                ;				   												
												
/*YAYING*/
for_directive : FOR { } for_clause_optseq 
              ;
do_directive : DO { } do_clause_optseq
             ;
do_paired_directive : DO { } do_paried_clause_optseq
                    ;
simd_directive : SIMD { } simd_clause_optseq 
               ;
for_simd_directive : FOR SIMD { } for_simd_clause_optseq
                   ;
do_simd_directive : DO SIMD { } do_simd_clause_optseq
                  ;
do_simd_paired_directive : DO SIMD { } do_simd_paried_clause_optseq
                         ;
parallel_for_simd_directive : PARALLEL FOR SIMD parallel_for_simd_clause_optseq
                            ;
parallel_do_simd_directive : PARALLEL DO SIMD { } parallel_for_simd_clause_optseq
                           ;
declare_simd_directive : DECLARE SIMD { } declare_simd_clause_optseq
                       ;
declare_simd_fortran_directive : DECLARE SIMD { } '(' proc_name ')' declare_simd_clause_optseq
                               ;
proc_name : /* empty */
          | EXPR_STRING { }
          ;
distribute_directive : DISTRIBUTE {if (!cluster_stack_isEmpty()) {enDistribute = 1;}} distribute_clause_optseq
                     ;
distribute_simd_directive : DISTRIBUTE SIMD {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 1; guardarLineasDist += "#pragma omp simd";}} distribute_simd_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                          ;
distribute_parallel_for_directive : DISTRIBUTE PARALLEL FOR {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 2; guardarLineasDist += "#pragma omp parallel for";}} distribute_parallel_for_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                                  ;
distribute_parallel_do_directive : DISTRIBUTE PARALLEL DO {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 3; guardarLineasDist += "#pragma omp parallel do";}} distribute_parallel_do_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                                 ;
distribute_parallel_for_simd_directive : DISTRIBUTE PARALLEL FOR SIMD {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 4; guardarLineasDist += "#pragma omp parallel for simd";}} distribute_parallel_for_simd_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                                       ;
distribute_parallel_do_simd_directive : DISTRIBUTE PARALLEL DO SIMD {if (!cluster_stack_isEmpty()) {enDistribute = 1; enDistributeExtendido = 5; guardarLineasDist += "#pragma omp parallel do simd";}} distribute_parallel_do_simd_clause_optseq {if (!cluster_stack_isEmpty()) {enDistributeExtendido = 0;}}
                                      ;
parallel_for_directive : PARALLEL FOR { } parallel_for_clause_optseq
                       ;
parallel_do_directive : PARALLEL DO { } parallel_do_clause_optseq
                      ;
parallel_loop_directive : PARALLEL LOOP { } parallel_loop_clause_optseq
                        ;
parallel_sections_directive : PARALLEL SECTIONS { } parallel_sections_clause_optseq
                            ;
parallel_workshare_directive : PARALLEL WORKSHARE { } parallel_workshare_clause_optseq
                             ;
parallel_master_directive : PARALLEL MASTER { } parallel_master_clause_optseq
                          ;
master_taskloop_directive : MASTER TASKLOOP {
                          }
                          master_taskloop_clause_optseq
                          ;
master_taskloop_simd_directive : MASTER TASKLOOP SIMD {
                               }
                               master_taskloop_simd_clause_optseq
                               ;
parallel_master_taskloop_directive : PARALLEL MASTER TASKLOOP {
                                   }
                                   parallel_master_taskloop_clause_optseq
                                   ; 
parallel_master_taskloop_simd_directive : PARALLEL MASTER TASKLOOP SIMD {
                                        }
                                        parallel_master_taskloop_simd_clause_optseq
                                        ; 
loop_directive : LOOP { } loop_clause_optseq
               ;
scan_directive : SCAN { } scan_clause_optseq
               ;
sections_directive : SECTIONS { } sections_clause_optseq
                   ;
sections_paired_directive : SECTIONS { } sections_paired_clause_optseq
                          ;
section_directive : SECTION { }
                  ;
single_directive : SINGLE { } single_clause_optseq
                 ;
single_paired_directive : SINGLE { } single_paired_clause_optseq
                        ;
workshare_directive : WORKSHARE { }
                    ;
workshare_paired_directive : WORKSHARE { } workshare_paired_clause_optseq
                           ;
cancel_directive : CANCEL { } cancel_clause_optseq
                 ;
//cancel_fortran_directive : CANCEL {
//                              current_directive = new OpenMPDirective(OMPD_cancel);
//                         }
//                         cancel_clause_fortran_optseq
//                         ;
cancellation_point_directive : CANCELLATION POINT { } cancellation_point_clause_optseq
                             ;
//cancellation_point_fortran_directive : CANCELLATION POINT {
//                                         current_directive = new OpenMPDirective(OMPD_cancellation_point);
//                                     }
//                                     cancellation_point_clause_fortran_optseq
//                                     ;
teams_directive : TEAMS { } teams_clause_optseq
                ;

allocate_directive : ALLOCATE { } allocate_list allocate_clause_optseq
                   ;
allocate_list : '('directive_varlist')'
              ;

directive_variable : EXPR_STRING { }
                   ;
directive_varlist : directive_variable
                  | directive_varlist ',' directive_variable
                  ;

threadprivate_directive : THREADPRIVATE {  } '('threadprivate_list')'
                        ;
threadprivate_variable : EXPR_STRING { }
                       ;
threadprivate_list : threadprivate_variable
                   | threadprivate_list ',' threadprivate_variable
                   ;

declare_reduction_directive : DECLARE REDUCTION { } '(' reduction_list ')' declare_reduction_clause_optseq
                            ;

reduction_list : reduction_identifiers ':' typername_list ':' combiner
               ;

reduction_identifiers : '+'{ }
                      | '-'{ }
                      | '*'{ }
                      | '&'{ }
                      | '|'{ }
                      | '^'{ }
                      | LOGAND{ }
                      | LOGOR{ }
                      ; 

typername_variable : EXPR_STRING { }
                   ;
typername_list : typername_variable
               | typername_list ',' typername_variable
               ;
combiner : EXPR_STRING { }
         ;

declare_mapper_directive : DECLARE MAPPER { } '(' mapper_list ')' declare_mapper_clause_optseq
                         ;

mapper_list : mapper_identifier_optseq 
            ;

mapper_identifier_optseq : type_var
                         | mapper_identifier ':' type_var
                         ;
 
mapper_identifier : IDENTIFIER_DEFAULT { }
                  | EXPR_STRING { }
                  ;

type_var : EXPR_STRING { }
         ;

parallel_clause_optseq : /* empty */
                       | parallel_clause_seq
                       ;
					   
cluster_clause_optseq : /* empty */
					  | cluster_clause_seq
					  ;					 

cluster_data_clause_optseq : cluster_data_clause_seq
						   ;


cluster_update_clause_optseq : cluster_update_clause_seq
			     ;

cluster_teams_clause_optseq : /* empty */
			    | cluster_teams_clause_seq
			    ;

cluster_distribute_clause_optseq : /* empty */
				 | cluster_distribute_clause_seq
				 ;

cluster_teams_distribute_clause_optseq : /* empty */
				       | cluster_teams_distribute_clause_seq
				       ;

task_async_clause_optseq : /* empty */
						 | task_async_clause_seq
				         ;
					   						 					 
teams_clause_optseq : /* empty */
                    | teams_clause_seq
                    ;

for_clause_optseq : /*empty*/
                  | for_clause_seq
                  ;
do_clause_optseq : /*empty*/
                 | do_clause_seq
                 ;
do_paried_clause_optseq : /*empty*/
                        | nowait_clause
                        ;
simd_clause_optseq : /*empty*/
                   | simd_clause_seq
                   ;
for_simd_clause_optseq : /*empty*/
                       | for_simd_clause_seq
                       ;
do_simd_clause_optseq : /*empty*/
                      | do_simd_clause_seq
                      ;
do_simd_paried_clause_optseq : /*empty*/
                             | nowait_clause
                             ;
parallel_for_simd_clause_optseq : /*empty*/
                                | parallel_for_simd_clause_seq
                                ;
declare_simd_clause_optseq : /*empty*/
                           | declare_simd_clause_seq
                           ;
distribute_clause_optseq : /*empty*/
                         | distribute_clause_seq
                         ;
distribute_simd_clause_optseq : /*empty*/
                              | distribute_simd_clause_seq
                              ;
distribute_parallel_for_clause_optseq : /*empty*/
                                      | distribute_parallel_for_clause_seq
                                      ;
distribute_parallel_do_clause_optseq : /*empty*/
                                     | distribute_parallel_do_clause_seq
                                     ;
distribute_parallel_for_simd_clause_optseq : /*empty*/
                                           | distribute_parallel_for_simd_clause_seq
                                           ;
distribute_parallel_do_simd_clause_optseq : /*empty*/
                                          | distribute_parallel_do_simd_clause_seq
                                          ;
parallel_for_clause_optseq : /*empty*/
                           | parallel_for_clause_seq
                           ;
parallel_do_clause_optseq : /*empty*/
                          | parallel_do_clause_seq
                          ;
parallel_loop_clause_optseq : /*empty*/
                            | parallel_loop_clause_seq
                            ;
parallel_sections_clause_optseq : /*empty*/
                                | parallel_sections_clause_seq
                                ;
parallel_workshare_clause_optseq : /*empty*/
                                 | parallel_workshare_clause_seq
                                 ;
parallel_master_clause_optseq : /*empty*/
                              | parallel_master_clause_seq
                              ;
master_taskloop_clause_optseq : /*empty*/
                              | master_taskloop_clause_seq
                              ;
master_taskloop_simd_clause_optseq : /*empty*/
                                   | master_taskloop_simd_clause_seq
                                   ;
parallel_master_taskloop_clause_optseq : /*empty*/
                                       | parallel_master_taskloop_clause_seq
                                       ;
parallel_master_taskloop_simd_clause_optseq : /*empty*/
                                            | parallel_master_taskloop_simd_clause_seq
                                            ;
loop_clause_optseq : /*empty*/
                   | loop_clause_seq
                   ;
scan_clause_optseq : scan_clause_seq
                   ;
sections_clause_optseq : /*empty*/
                       | sections_clause_seq
                       ;
sections_paired_clause_optseq : /*empty*/
                              | nowait_clause
                              ;
single_clause_optseq : /*empty*/
                     | single_clause_seq
                     ;
single_paired_clause_optseq : /*empty*/
                            | single_paired_clause_seq
                            ;
workshare_paired_clause_optseq : /*empty*/
                               | nowait_clause
                               ;
cancel_clause_optseq : /*empty*/
                     | cancel_clause_seq
                     ;
//cancel_clause_fortran_optseq : /*empty*/
//                             | cancel_clause_fortran_seq
//                             ;
cancellation_point_clause_optseq : /*empty*/
                                 | cancellation_point_clause_seq
                                 ;
//cancellation_point_clause_fortran_optseq : /*empty*/
//                                         | cancellation_point_clause_fortran_seq
                                         ;
allocate_clause_optseq : /*empty*/
                       | allocate_clause_seq
                       ;
allocate_clause_seq : allocate_directive_clause
                    | allocate_clause_seq allocate_directive_clause
                    | allocate_clause_seq ',' allocate_directive_clause
                    ; 



declare_reduction_clause_optseq :  /*empty*/
                                | declare_reduction_clause_seq
                                ;
declare_mapper_clause_optseq : /*empty*/
                             | declare_mapper_clause_seq
                             ;
declare_mapper_clause_seq : declare_mapper_clause
                          | declare_mapper_clause_seq declare_mapper_clause
                          | declare_mapper_clause_seq ',' declare_mapper_clause
                          ; 
parallel_clause_seq : parallel_clause
                    | parallel_clause_seq parallel_clause
                    | parallel_clause_seq ',' parallel_clause
                    ;
					
cluster_clause_seq : cluster_clause	
				   | cluster_clause_seq cluster_clause
				   | cluster_clause_seq ',' cluster_clause
				   ;		

cluster_data_clause_seq : cluster_data_clause
						| cluster_data_clause_seq cluster_data_clause
						| cluster_data_clause_seq ',' cluster_data_clause
						;

cluster_update_clause_seq : cluster_update_clause
						| cluster_update_clause_seq cluster_update_clause
						| cluster_update_clause_seq ',' cluster_update_clause
						;

cluster_teams_clause_seq : cluster_teams_clause
						| cluster_teams_clause_seq cluster_teams_clause
						| cluster_teams_clause_seq ',' cluster_teams_clause
						;

cluster_distribute_clause_seq : cluster_distribute_clause
						| cluster_distribute_clause_seq cluster_distribute_clause
						| cluster_distribute_clause_seq ',' cluster_distribute_clause
						;

cluster_teams_distribute_clause_seq : cluster_teams_distribute_clause
						| cluster_teams_distribute_clause_seq cluster_teams_distribute_clause
						| cluster_teams_distribute_clause_seq ',' cluster_teams_distribute_clause
						;
						
task_async_clause_seq : task_async_clause
					  | task_async_clause_seq task_async_clause 
					  | task_async_clause_seq ',' task_async_clause 
					  ;
				
teams_clause_seq : teams_clause
                 | teams_clause_seq teams_clause
                 | teams_clause_seq ',' teams_clause
                 ;

for_clause_seq : for_clause
               | for_clause_seq for_clause
               | for_clause_seq "," for_clause
               ;

do_clause_seq : do_clause
              | do_clause_seq do_clause
              | do_clause_seq "," do_clause
              ;

simd_clause_seq : simd_clause
                | simd_clause_seq simd_clause
                | simd_clause_seq "," simd_clause
                ;

for_simd_clause_seq : for_simd_clause
                    | for_simd_clause_seq for_simd_clause
                    | for_simd_clause_seq "," for_simd_clause
                    ;
do_simd_clause_seq : do_simd_clause
                   | do_simd_clause_seq do_simd_clause
                   | do_simd_clause_seq "," do_simd_clause
                   ;
parallel_for_simd_clause_seq : parallel_for_simd_clause
                             | parallel_for_simd_clause_seq parallel_for_simd_clause
                             | parallel_for_simd_clause_seq "," parallel_for_simd_clause
                             ;
declare_simd_clause_seq : declare_simd_clause
                        | declare_simd_clause_seq declare_simd_clause
                        | declare_simd_clause_seq "," declare_simd_clause
                        ;
distribute_clause_seq : distribute_clause
                      | distribute_clause_seq distribute_clause
                      | distribute_clause_seq "," distribute_clause
                      ;
distribute_simd_clause_seq : distribute_simd_clause
                           | distribute_simd_clause_seq distribute_simd_clause
                           | distribute_simd_clause_seq "," distribute_simd_clause
                           ;
distribute_parallel_for_clause_seq : distribute_parallel_for_clause
                                   | distribute_parallel_for_clause_seq distribute_parallel_for_clause
                                   | distribute_parallel_for_clause_seq "," distribute_parallel_for_clause
                                   ;
distribute_parallel_do_clause_seq : distribute_parallel_do_clause
                                  | distribute_parallel_do_clause_seq distribute_parallel_do_clause
                                  | distribute_parallel_do_clause_seq "," distribute_parallel_do_clause
                                  ;
distribute_parallel_for_simd_clause_seq : distribute_parallel_for_simd_clause
                                        | distribute_parallel_for_simd_clause_seq distribute_parallel_for_simd_clause
                                        | distribute_parallel_for_simd_clause_seq "," distribute_parallel_for_simd_clause
                                        ;
distribute_parallel_do_simd_clause_seq : distribute_parallel_do_simd_clause
                                       | distribute_parallel_do_simd_clause_seq distribute_parallel_do_simd_clause
                                       | distribute_parallel_do_simd_clause_seq "," distribute_parallel_do_simd_clause
                                       ;
parallel_for_clause_seq : parallel_for_clause
                        | parallel_for_clause_seq parallel_for_clause
                        | parallel_for_clause_seq "," parallel_for_clause
                        ;
parallel_do_clause_seq : parallel_do_clause
                       | parallel_do_clause_seq parallel_do_clause
                       | parallel_do_clause_seq "," parallel_do_clause
                       ;
parallel_loop_clause_seq : parallel_loop_clause
                         | parallel_loop_clause_seq parallel_loop_clause
                         | parallel_loop_clause_seq "," parallel_loop_clause
                         ;
parallel_sections_clause_seq : parallel_sections_clause
                             | parallel_sections_clause_seq parallel_sections_clause
                             | parallel_sections_clause_seq "," parallel_sections_clause
                             ;
parallel_workshare_clause_seq : parallel_workshare_clause
                              | parallel_workshare_clause_seq parallel_workshare_clause
                              | parallel_workshare_clause_seq "," parallel_workshare_clause
                              ;
parallel_master_clause_seq : parallel_master_clause
                           | parallel_master_clause_seq parallel_master_clause
                           | parallel_master_clause_seq "," parallel_master_clause
                           ;
master_taskloop_clause_seq : master_taskloop_clause
                           | master_taskloop_clause_seq master_taskloop_clause
                           | master_taskloop_clause_seq "," master_taskloop_clause
                           ;
master_taskloop_simd_clause_seq : master_taskloop_simd_clause
                                | master_taskloop_simd_clause_seq master_taskloop_simd_clause
                                | master_taskloop_simd_clause_seq "," master_taskloop_simd_clause
                                ;
parallel_master_taskloop_clause_seq : parallel_master_taskloop_clause
                                    | parallel_master_taskloop_clause_seq parallel_master_taskloop_clause
                                    | parallel_master_taskloop_clause_seq "," parallel_master_taskloop_clause
                                    ;
parallel_master_taskloop_simd_clause_seq : parallel_master_taskloop_simd_clause
                                         | parallel_master_taskloop_simd_clause_seq parallel_master_taskloop_simd_clause
                                         | parallel_master_taskloop_simd_clause_seq "," parallel_master_taskloop_simd_clause
                                         ;
loop_clause_seq : loop_clause
                | loop_clause_seq loop_clause
                | loop_clause_seq "," loop_clause
                ;
scan_clause_seq : scan_clause
                ;
sections_clause_seq : sections_clause
                    | sections_clause_seq sections_clause
                    | sections_clause_seq "," sections_clause
                    ;
//sections_clause_fortran_seq : sections_fortran_clause
//                            | sections_clause_fortran_seq sections_fortran_clause
//                            | sections_clause_fortran_seq "," sections_fortran_clause
//                            ;
single_clause_seq : single_clause
                  | single_clause_seq single_clause
                  | single_clause_seq "," single_clause
                  ;
single_paired_clause_seq : single_paired_clause
                         | single_paired_clause_seq single_paired_clause
                         | single_paired_clause_seq "," single_paired_clause
                         ;
cancel_clause_seq : construct_type_clause
                  | construct_type_clause if_cancel_clause
                  | construct_type_clause "," if_cancel_clause
                  ;
//cancel_clause_fortran_seq : construct_type_clause_fortran
//                          | construct_type_clause_fortran if_cancel_clause
//                          | construct_type_clause_fortran "," if_cancel_clause
//                          ;
cancellation_point_clause_seq : construct_type_clause
                              ;
//cancellation_point_clause_fortran_seq : construct_type_clause_fortran
//                                      ;
allocate_directive_clause : allocator_clause
                          | align_clause
                          ;
declare_reduction_clause_seq : initializer_clause
                             ;
declare_mapper_clause : map_clause
                      ;
parallel_clause : if_parallel_clause
                | num_threads_clause
                | default_clause
                | private_clause
                | firstprivate_clause
                | shared_clause
                | copyin_clause
                | reduction_clause
                | proc_bind_clause
                | allocate_clause
                ;
				
cluster_clause : alloc_clause
			   | broad_clause
			   | {enScatter = 1; chunk_pos = 0; aumentarScatter();} scatter_clause {enScatter = 0; MPIScatter(); if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			   | {cluster_stack *c = cluster_stack_peek(); c->enGather = 1; chunk_pos = 0; aumentarGather(); enGatherInst = 1;} gather_clause {enGatherInst = 0; if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			   | {cluster_stack *c = cluster_stack_peek(); c->enAllGather = 1; chunk_pos = 0; aumentarAllGather(); enAllGatherInst = 1;} allgather_clause {enAllGatherInst = 0; if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			   | halo_clause
			   | {cluster_stack *c = cluster_stack_peek(); c->enReduction = 1;} reduction_clause_cluster
			   | {cluster_stack *c = cluster_stack_peek(); c->enAllReduction = 1;} allreduction_clause_cluster
			   ;
				
cluster_data_clause : alloc_clause
				    			  | broad_clause
			              | {enScatter = 1; chunk_pos = 0; aumentarScatter();} scatter_clause {enScatter = 0; MPIScatter(); if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			              | {cluster_stack *c = cluster_stack_peek(); c->enGather = 1; chunk_pos = 0; aumentarGather(); enGatherInst = 1;} gather_clause {enGatherInst = 0; if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			              | {cluster_stack *c = cluster_stack_peek(); c->enAllGather = 1; chunk_pos = 0; aumentarAllGather(); enAllGatherInst = 1;} allgather_clause {enAllGatherInst = 0; if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			              | halo_clause
			              | {cluster_stack *c = cluster_stack_peek(); c->enReduction = 1;} reduction_clause_cluster
			              | {cluster_stack *c = cluster_stack_peek(); c->enAllReduction = 1;} allreduction_clause_cluster
				    ;

cluster_update_clause : alloc_clause
				    			  | broad_clause
			              | {enScatter = 1; chunk_pos = 0; aumentarScatter();} scatter_clause {enScatter = 0; MPIScatter(); if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			              | {cluster_stack *c = cluster_stack_peek(); c->enGather = 1; chunk_pos = 0; aumentarGather(); enGatherInst = 1;} gather_clause {enGatherInst = 0; if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			              | {cluster_stack *c = cluster_stack_peek(); c->enAllGather = 1; aumentarAllGather(); enAllGatherInst = 1;} allgather_clause {enAllGatherInst = 0; if (!includeStringDone){includeStringDone = 1; IncludeString();}}
			              | halo_clause
			              | {cluster_stack *c = cluster_stack_peek(); c->enReduction = 1;} reduction_clause_cluster
			              | {cluster_stack *c = cluster_stack_peek(); c->enAllReduction = 1;} allreduction_clause_cluster
				    ;

cluster_teams_clause : if_target_clause
                     | device_clause
                     | private_clause
                     | firstprivate_clause
                     | in_reduction_clause
                     | map_clause
                     | is_device_ptr_clause
                     | defaultmap_clause
                     | nowait_clause
                     | allocate_clause
                     | depend_with_modifier_clause
                     | uses_allocators_clause
                     | num_teams_clause
                     | thread_limit_clause
                     | default_clause
                     | shared_clause
                     | reduction_default_only_clause
                     ;
 
cluster_distribute_clause : private_clause
                  	  | firstprivate_clause 
                  	  | lastprivate_distribute_clause
                  	  | collapse_clause
                  	  | dist_schedule_clause_cluster
                 	  | allocate_clause
                    | {enReductionDistribute = 1;} reduction_clause_cluster
                    | {enAllReductionDistribute = 1;} allreduction_clause_cluster
                 	  ;

cluster_teams_distribute_clause : if_target_clause
                                | device_clause
                                | private_clause
                                | firstprivate_clause
                                | in_reduction_clause
                                | map_clause
                                | is_device_ptr_clause
                                | defaultmap_clause
                                | nowait_clause
                                | allocate_clause
                                | depend_with_modifier_clause
                                | uses_allocators_clause
                                | num_teams_clause
                                | thread_limit_clause
                                | default_clause                   
                                | shared_clause
                                | lastprivate_distribute_clause
                                | collapse_clause
                                | dist_schedule_clause_cluster
                                | {enReductionDistribute = 1;} reduction_clause_cluster
                                | {enAllReductionDistribute = 1;} allreduction_clause_cluster
                                ;
					
task_async_clause : DEPEND { } '(' dependance_type ':' var_list_task_async ')' 
				  ;

dependance_type : IN    { ta_current_kind = TD_DEPEND_IN; }
				        | OUT   { ta_current_kind = TD_DEPEND_OUT; }
                | INOUT { ta_current_kind = TD_DEPEND_INOUT; }
				        ;

var_list_task_async   : variable {
                          const char* chunk = $1;

                          char var[256];
                          int i = 0;
                          cluster_stack *c = cluster_stack_peek();

                          while(chunk[i] != '\0' && chunk[i] != '[' && i < 255){
                            var[i] = chunk[i];
                            i++;
                          }
                          var[i] = '\0';

                          SymbolInfo* symbol = table.getSymbolInfo(var);
                          if(symbol == NULL){
                            fprintf(stderr, "Error: variable not found in symble table\n");
                            exit(901);
                          }

                          std::string valueType = symbol->getVariableType();
                          std::string mpiType = translateTypes(symbol->getVariableType());

                          if(c == NULL || c->tasking_region == NULL){
                            fprintf(stderr, "Error: task depend without tasking_region\n");
                            exit(901);
                          }

                          if(tasking_region_add_task_async_block_depend(c->tasking_region, ta_current_kind, chunk, valueType.c_str(), mpiType.c_str()) != 0){
                            fprintf(stderr, "Error: tasking_region_add_task_async_block_depend\n");
                            exit(901);
                          }
                        } 
                      | var_list_task_async ',' variable {
                          const char* chunk = $3;

                          char var[256];
                          int i = 0;
                          cluster_stack *c = cluster_stack_peek();

                          while(chunk[i] != '\0' && chunk[i] != '[' && i < 255){
                            var[i] = chunk[i];
                            i++;
                          }
                          var[i] = '\0';

                          SymbolInfo* symbol = table.getSymbolInfo(var);
                          if(symbol == NULL){
                            fprintf(stderr, "Error: variable not found in symble table\n");
                            exit(901);
                          }

                          std::string valueType = symbol->getVariableType();
                          std::string mpiType = translateTypes(symbol->getVariableType());

                          if(c == NULL || c->tasking_region == NULL){
                            fprintf(stderr, "Error: task depend without tasking_region\n");
                            exit(901);
                          }

                          if(tasking_region_add_task_async_block_depend(c->tasking_region, ta_current_kind, chunk, valueType.c_str(), mpiType.c_str()) != 0){
                            fprintf(stderr, "Error: tasking_region_add_task_async_block_depend\n");
                            exit(901);
                          }
                        }
                      ;

teams_clause : num_teams_clause
             | thread_limit_clause
             | default_clause
             | private_clause
             | firstprivate_clause
             | shared_clause
             | reduction_default_only_clause
             | allocate_clause
             ;

for_clause : private_clause
           | firstprivate_clause
           | lastprivate_clause
           | linear_clause
           | reduction_clause
           | schedule_clause
           | collapse_clause
           | ordered_clause
           | nowait_clause
           | allocate_clause
           | order_clause
           ;

do_clause : private_clause
          | firstprivate_clause
          | lastprivate_clause
          | linear_clause
          | reduction_clause
          | schedule_clause
          | collapse_clause
          | ordered_clause
          | allocate_clause
          | order_clause
          ;

simd_clause : if_simd_clause
            | safelen_clause
            | simdlen_clause
            | linear_clause
            | aligned_clause
            | nontemporal_clause
            | private_clause
            | lastprivate_clause
            | reduction_clause
            | collapse_clause
            | order_clause
            ;

for_simd_clause : if_simd_clause
                | safelen_clause
                | simdlen_clause
                | linear_clause
                | aligned_clause
                | private_clause
                | firstprivate_clause 
                | lastprivate_clause
                | reduction_clause
                | schedule_clause
                | collapse_clause
                | ordered_clause
                | nowait_clause
                | allocate_clause
                | order_clause
                | nontemporal_clause
                ;
do_simd_clause : if_simd_clause
               | safelen_clause
               | simdlen_clause
               | linear_clause
               | aligned_clause
               | private_clause 
               | firstprivate_clause 
               | lastprivate_clause
               | reduction_clause
               | schedule_clause
               | collapse_clause
               | ordered_clause
               | allocate_clause
               | order_clause
               | nontemporal_clause
               ;
parallel_for_simd_clause : if_parallel_simd_clause
                         | num_threads_clause
                         | default_clause
                         | private_clause
                         | firstprivate_clause
                         | shared_clause
                         | copyin_clause
                         | reduction_clause
                         | proc_bind_clause
                         | allocate_clause
                         | lastprivate_clause 
                         | linear_clause
                         | schedule_clause
                         | collapse_clause
                         | ordered_clause
                         | order_clause
                         | safelen_clause
                         | simdlen_clause
                         | aligned_clause
                         | nontemporal_clause
                         ;
 
declare_simd_clause : simdlen_clause
                    | linear_clause
                    | aligned_clause
                    | uniform_clause
                    | inbranch_clause
                    | notinbranch_clause
                    ;
 
distribute_clause : private_clause
                  | firstprivate_clause 
                  | lastprivate_distribute_clause
                  | collapse_clause
                  | dist_schedule_clause_cluster
                  | allocate_clause
                  ;

distribute_simd_clause : private_clause
                       | firstprivate_clause 
                       | lastprivate_clause
                       | collapse_clause
                       | dist_schedule_clause_cluster
                       | allocate_clause
                       | if_simd_clause
                       | safelen_clause
                       | simdlen_clause
                       | {if (enDistributeExtendido) {guardarLineasDist += " linear("; meterEnClause = 1;}} linear_clause
                       | {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += " aligned("; meterEnClause = 1;}} aligned_clause
                       | nontemporal_clause
                       | reduction_clause_cluster
                       | order_clause
                       ;
                       
distribute_parallel_for_clause : if_parallel_clause
                               | num_threads_clause
                               | default_clause
                               | private_clause
                               | firstprivate_clause
                               | shared_clause
                               | copyin_clause
                               | reduction_clause_cluster
                               | proc_bind_clause
                               | allocate_clause
                               | lastprivate_clause 
                               | {if (enDistributeExtendido) {guardarLineasDist += " linear("; meterEnClause = 1;}} linear_clause
                               | schedule_clause
                               | collapse_clause
                               | {if (enDistributeExtendido > 1) {guardarLineasDist += " ordered"; meterEnClause = 1;}} ordered_clause
                               | nowait_clause
                               | order_clause 
                               | dist_schedule_clause_cluster
                               ;
distribute_parallel_do_clause : if_parallel_clause
                              | num_threads_clause
                              | default_clause
                              | private_clause
                              | firstprivate_clause
                              | shared_clause
                              | copyin_clause
                              | reduction_clause_cluster
                              | proc_bind_clause
                              | allocate_clause
                              | lastprivate_clause 
                              | {if (enDistributeExtendido) {guardarLineasDist += " linear("; meterEnClause = 1;}} linear_clause
                              | schedule_clause
                              | collapse_clause
                              | {if (enDistributeExtendido > 1) {guardarLineasDist += " ordered"; meterEnClause = 1;}} ordered_clause
                              | order_clause 
                              | dist_schedule_clause_cluster
                              ;
distribute_parallel_for_simd_clause : if_parallel_simd_clause
                                    | num_threads_clause
                                    | default_clause
                                    | private_clause
                                    | firstprivate_clause
                                    | shared_clause
                                    | copyin_clause
                                    | reduction_clause_cluster
                                    | proc_bind_clause
                                    | allocate_clause
                                    | lastprivate_clause 
                                    | {if (enDistributeExtendido) {guardarLineasDist += " linear("; meterEnClause = 1;}} linear_clause
                                    | schedule_clause
                                    | collapse_clause
                                    | {if (enDistributeExtendido > 1) {guardarLineasDist += " ordered"; meterEnClause = 1;}} ordered_clause
                                    | nowait_clause
                                    | order_clause 
                                    | dist_schedule_clause_cluster
                                    | safelen_clause
                                    | simdlen_clause
                                    | {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += " aligned("; meterEnClause = 1;}} aligned_clause
                                    | nontemporal_clause
                                    ;
distribute_parallel_do_simd_clause : if_parallel_simd_clause
                                   | num_threads_clause
                                   | default_clause
                                   | private_clause
                                   | firstprivate_clause
                                   | shared_clause
                                   | copyin_clause
                                   | reduction_clause_cluster
                                   | proc_bind_clause
                                   | allocate_clause
                                   | lastprivate_clause 
                                   | {if (enDistributeExtendido) {guardarLineasDist += " linear("; meterEnClause = 1;}} linear_clause
                                   | schedule_clause
                                   | collapse_clause
                                   | {if (enDistributeExtendido > 1) {guardarLineasDist += " ordered"; meterEnClause = 1;}} ordered_clause
                                   | order_clause 
                                   | dist_schedule_clause_cluster
                                   | safelen_clause
                                   | simdlen_clause
                                   | {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += " aligned("; meterEnClause = 1;}} aligned_clause
                                   | nontemporal_clause
                                   ;
parallel_for_clause : if_parallel_clause
                    | num_threads_clause
                    | default_clause
                    | private_clause
                    | firstprivate_clause
                    | shared_clause
                    | copyin_clause
                    | reduction_clause
                    | proc_bind_clause
                    | allocate_clause
                    | lastprivate_clause 
                    | linear_clause
                    | schedule_clause
                    | collapse_clause
                    | ordered_clause
                    | nowait_clause
                    | order_clause 
                    ;
parallel_do_clause : if_parallel_clause
                   | num_threads_clause
                   | default_clause
                   | private_clause
                   | firstprivate_clause
                   | shared_clause
                   | copyin_clause
                   | reduction_clause
                   | proc_bind_clause
                   | allocate_clause
                   | lastprivate_clause 
                   | linear_clause
                   | schedule_clause
                   | collapse_clause
                   | ordered_clause
                   | order_clause 
                   ;
parallel_loop_clause : if_parallel_clause
                     | num_threads_clause
                     | default_clause
                     | private_clause
                     | firstprivate_clause
                     | shared_clause
                     | copyin_clause
                     | reduction_clause
                     | proc_bind_clause
                     | allocate_clause
                     | lastprivate_clause 
                     | collapse_clause
                     | bind_clause
                     | order_clause 
                     ;
parallel_sections_clause : if_parallel_clause
                         | num_threads_clause
                         | default_clause
                         | private_clause
                         | firstprivate_clause
                         | shared_clause
                         | copyin_clause
                         | reduction_clause
                         | proc_bind_clause
                         | allocate_clause
                         | lastprivate_clause 
                         ;
parallel_workshare_clause : if_parallel_clause
                          | num_threads_clause
                          | default_clause
                          | private_clause
                          | firstprivate_clause
                          | shared_clause
                          | copyin_clause
                          | reduction_clause
                          | proc_bind_clause
                          | allocate_clause
                          ;
parallel_master_clause : if_parallel_clause
                       | num_threads_clause
                       | default_clause
                       | private_clause
                       | firstprivate_clause
                       | shared_clause
                       | copyin_clause
                       | reduction_clause
                       | proc_bind_clause
                       | allocate_clause
                       ;
master_taskloop_clause : if_taskloop_clause
                       | shared_clause
                       | private_clause
                       | firstprivate_clause
                       | lastprivate_clause
                       | reduction_clause
                       | in_reduction_clause
                       | default_clause
                       | grainsize_clause
                       | num_tasks_clause
                       | collapse_clause
                       | final_clause
                       | priority_clause
                       | untied_clause
                       | mergeable_clause
                       | nogroup_clause
                       | allocate_clause
                       ;
master_taskloop_simd_clause : if_taskloop_simd_clause
                            | shared_clause
                            | private_clause
                            | firstprivate_clause
                            | lastprivate_clause
                            | reduction_clause
                            | in_reduction_clause
                            | default_clause
                            | grainsize_clause
                            | num_tasks_clause
                            | collapse_clause
                            | final_clause
                            | priority_clause
                            | untied_clause
                            | mergeable_clause
                            | nogroup_clause
                            | allocate_clause
                            | safelen_clause
                            | simdlen_clause
                            | linear_clause
                            | aligned_clause
                            | nontemporal_clause
                            | order_clause 
                            ;
parallel_master_taskloop_clause : if_parallel_taskloop_clause
                                | num_threads_clause
                                | default_clause
                                | private_clause
                                | firstprivate_clause
                                | shared_clause
                                | copyin_clause
                                | reduction_clause
                                | proc_bind_clause
                                | allocate_clause
                                | lastprivate_clause 
                                | nowait_clause 
                                | grainsize_clause
                                | num_tasks_clause
                                | collapse_clause
                                | final_clause
                                | priority_clause
                                | untied_clause
                                | mergeable_clause
                                | nogroup_clause
                                ;
parallel_master_taskloop_simd_clause : if_parallel_taskloop_simd_clause
                                     | num_threads_clause
                                     | default_clause
                                     | private_clause
                                     | firstprivate_clause
                                     | shared_clause
                                     | copyin_clause
                                     | reduction_clause
                                     | proc_bind_clause
                                     | allocate_clause
                                     | lastprivate_clause 
                                     | nowait_clause 
                                     | grainsize_clause
                                     | num_tasks_clause
                                     | collapse_clause
                                     | final_clause
                                     | priority_clause
                                     | untied_clause
                                     | mergeable_clause
                                     | nogroup_clause
                                     | safelen_clause
                                     | simdlen_clause
                                     | linear_clause
                                     | aligned_clause
                                     | nontemporal_clause
                                     | order_clause
                                     ;
loop_clause : bind_clause
            | collapse_clause
            | order_clause
            | private_clause
            | lastprivate_clause
            | reduction_default_only_clause
            ;
scan_clause : inclusive_clause
            | exclusive_clause
            ;
sections_clause : private_clause
                | firstprivate_clause
                | lastprivate_clause
                | reduction_clause
                | allocate_clause
                | fortran_nowait_clause
                ;
single_clause : private_clause
              | firstprivate_clause
              | fortran_copyprivate_clause
              | allocate_clause
              | fortran_nowait_clause
              ;
single_paired_clause : copyprivate_clause
                     | nowait_clause
                     ;
construct_type_clause : PARALLEL { }
                      | SECTIONS { }
                      | FOR { }
                      | DO { }
                      | TASKGROUP { }
                      ;
//construct_type_clause_fortran : PARALLEL { current_clause = current_directive->addOpenMPClause(OMPC_parallel); }
//                              | SECTIONS { current_clause = current_directive->addOpenMPClause(OMPC_sections); }
//                              | DO { current_clause = current_directive->addOpenMPClause(OMPC_do); }
//                              | TASKGROUP { current_clause = current_directive->addOpenMPClause(OMPC_taskgroup); }
//                              ;
if_parallel_clause : IF {if (enDistributeExtendido == 2 || enDistributeExtendido == 3){guardarLineasDist += " if("; meterEnClause = 1;}} '(' if_parallel_parameter ')' {if (enDistributeExtendido == 2 || enDistributeExtendido == 3){guardarLineasDist += ")"; meterEnClause = 0;}}
                   ;

if_parallel_parameter : PARALLEL ':' {if (meterEnClause){guardarLineasDist += $1;}}
                        expression {if (meterEnClause){guardarLineasDist += $1;}}
                      | EXPR_STRING {if (meterEnClause){guardarLineasDist += $1;}}
                      ;
if_task_clause : IF '(' if_task_parameter ')' { ; }
               ;

if_task_parameter : TASK ':' { }  expression { ; }
                  | EXPR_STRING { }
                  ;
if_taskloop_clause : IF '(' if_taskloop_parameter ')' { ; }
                   ;

if_taskloop_parameter : TASKLOOP ':' { }  expression { ; }
                      | EXPR_STRING { }
                      ;
if_target_data_clause : IF '(' if_target_data_parameter ')' { ; }
                      ;

if_target_data_parameter : TARGET DATA ':' { }  expression { ; }
                         | EXPR_STRING { } 
                         ;
if_target_parallel_clause : IF '(' if_target_parallel_parameter ')' { ; }
                          ;

if_target_parallel_parameter : TARGET ':' { }  expression { ; }
                             | PARALLEL ':' { }  expression { ; }
                             | EXPR_STRING { } 
                             ;
if_target_simd_clause : IF '(' if_target_simd_parameter ')' { ; }
                      ;

if_target_simd_parameter : TARGET ':' { }  expression { ; }
                         | SIMD ':' { }  expression { ; }
                         | EXPR_STRING { } 
                             ;
if_target_enter_data_clause : IF '(' if_target_enter_data_parameter ')' { ; }
                            ;

if_target_enter_data_parameter : TARGET ENTER DATA ':' { }  expression { ; }
                               | EXPR_STRING { }
                               ;
if_target_exit_data_clause : IF '(' if_target_exit_data_parameter ')' { ; }
                           ;

if_target_exit_data_parameter : TARGET EXIT DATA ':' { }  expression { ; }
                              | EXPR_STRING { }
                              ;
if_target_clause : IF '(' if_target_parameter ')' { ; }
                 ;

if_target_parameter : TARGET ':' { }  expression { ; }
                    | EXPR_STRING { }
                    ;
if_target_update_clause : IF '(' if_target_update_parameter ')' { ; }
                        ;

if_target_update_parameter : TARGET UPDATE ':' { }  expression { ; }
                           | EXPR_STRING { }
                           ;
if_taskloop_simd_clause : IF '(' if_taskloop_simd_parameter ')' { ; }
                        ;

if_taskloop_simd_parameter : TASKLOOP ':' { }  expression { ; }
                           | SIMD ':' { }  expression { ; }
                           | EXPR_STRING {
                             } ;
if_simd_clause : IF {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += " if("; meterEnClause = 1;}} '(' if_simd_parameter ')' {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += ")"; meterEnClause = 0;}}
               ;
if_simd_parameter : SIMD ':' {if (meterEnClause) {guardarLineasDist += "simd:";}}  expression {if (meterEnClause) {guardarLineasDist += $4;}}
                  | EXPR_STRING {if (meterEnClause) {guardarLineasDist += $1;}}
                  ;
if_parallel_simd_clause : IF '(' {if(enDistributeExtendido == 4 || enDistributeExtendido == 5){guardarLineasDist += "if("; meterEnClause = 1;}} if_parallel_simd_parameter ')' {if(enDistributeExtendido == 4 || enDistributeExtendido == 5){guardarLineasDist += ")"; meterEnClause = 0;}}
                        ;
if_parallel_simd_parameter : SIMD ':' expression {if(meterEnClause){guardarLineasDist += "simd:"; guardarLineasDist += $3;}}
                           | PARALLEL ':' expression {if(meterEnClause){guardarLineasDist += "parallel:"; guardarLineasDist += $3;}}
                           | EXPR_STRING {if(meterEnClause){guardarLineasDist += $1;}}
                           ;
if_target_parallel_simd_clause : IF '(' if_target_parallel_simd_parameter ')' { ; }
                               ;
if_target_parallel_simd_parameter : SIMD ':' { }  expression { ; }
                                  | PARALLEL ':' { }  expression { ; }
                                  | TARGET ':' { }  expression { ; }
                                  | EXPR_STRING { }
                                  ;
if_cancel_clause : IF '(' if_cancel_parameter ')' { ; }
                 ;
if_cancel_parameter : CANCEL ':' { }  expression { ; }
                    | EXPR_STRING { }
                    ;
if_parallel_taskloop_clause : IF '(' if_parallel_taskloop_parameter ')' { ; }
                            ;
if_parallel_taskloop_parameter : PARALLEL ':' { }  expression { ; }
                               | TASKLOOP ':' { }  expression { ; }
                               | EXPR_STRING {
                                } ;
if_parallel_taskloop_simd_clause : IF '(' if_parallel_taskloop_simd_parameter ')' { ; }
                                 ;
if_parallel_taskloop_simd_parameter : PARALLEL ':' { }  expression { ; }
                                    | TASKLOOP ':' { }  expression { ; }
                                    | SIMD ':' { }  expression { ; }
                                    | EXPR_STRING { }
                                    ;
/*if_clause : IF '(' if_parameter ')' { ; }
          ;

if_parameter : EXPR_STRING {
                current_clause = current_directive->addOpenMPClause(OMPC_if, OMPC_IF_MODIFIER_unspecified);
                current_clause->addLangExpr($1);
                }
             ;
*/
num_threads_clause: NUM_THREADS {if (enDistributeExtendido > 1){guardarLineasDist += " num_threads("; meterEnClause = 1;}} '(' expression ')' {if (enDistributeExtendido > 1){guardarLineasDist += $4; guardarLineasDist += ")"; meterEnClause = 0;}}
                  ;

num_teams_clause: NUM_TEAMS '(' expression ')' { 
                   enNumTeams = 1;
                   numTeamsExpr = std::string($3);   
                  } 
                ;

align_clause: ALIGN { } '(' expression ')';
                
thread_limit_clause: THREAD_LIMIT { } '(' expression ')'
                   ;
copyin_clause: COPYIN {if (enDistributeExtendido > 1){guardarLineasDist += " copyin("; meterEnClause = 1;}} '(' var_list ')' {if (enDistributeExtendido > 1){guardarLineasDist += ")"; meterEnClause = 0;}}
             ;

default_clause : DEFAULT {if (enDistributeExtendido > 1){guardarLineasDist += " default("; meterEnClause = 1;}} '(' default_parameter ')' {if (enDistributeExtendido > 1){guardarLineasDist += ")"; meterEnClause = 0;}}
               ;

default_parameter : SHARED {if (meterEnClause){guardarLineasDist += $1;}}
                  | NONE {if (meterEnClause){guardarLineasDist += $1;}}
                  | FIRSTPRIVATE {if (meterEnClause){guardarLineasDist += $1;}}
                  | PRIVATE {if (meterEnClause){guardarLineasDist += $1;}}
                  ;

default_variant_clause : DEFAULT '(' default_variant_directive ')' { }
                       ;

default_variant_directive : { }
                          ;

proc_bind_clause : PROC_BIND {if(enDistributeExtendido > 1){guardarLineasDist += " proc_bind("; meterEnClause = 1;}} '(' proc_bind_parameter ')' {if(enDistributeExtendido > 1){guardarLineasDist += ")"; meterEnClause = 0;}}
                  ;

proc_bind_parameter : MASTER {if (meterEnClause){guardarLineasDist += $1;}}
                    | CLOSE {if (meterEnClause){guardarLineasDist += $1;}}
                    | SPREAD {if (meterEnClause){guardarLineasDist += $1;}}
                    ;
bind_clause : BIND '(' bind_parameter ')' { } ;

bind_parameter : TEAMS { }
               | PARALLEL { }
               | THREAD { }
               ;

allocate_clause : ALLOCATE {if (enDistributeExtendido > 1) {guardarLineasDist += " allocate("; meterEnClause = 1;}} '(' allocate_parameter ')' {if (enDistributeExtendido > 1) {guardarLineasDist += ")"; meterEnClause = 0;}}
                ;

allocate_parameter : EXPR_STRING  {if (meterEnClause) {guardarLineasDist += $1;}}
                   | EXPR_STRING ',' {if (meterEnClause) {guardarLineasDist += $1; guardarLineasDist += ",";}} var_list
                   | allocator_parameter ':' {if (meterEnClause) {guardarLineasDist += ":";}} var_list
                   ;
allocator_parameter : DEFAULT_MEM_ALLOC { }
                    | LARGE_CAP_MEM_ALLOC { }
                    | CONST_MEM_ALLOC { }
                    | HIGH_BW_MEM_ALLOC { }
                    | LOW_LAT_MEM_ALLOC { }
                    | CGROUP_MEM_ALLOC { }
                    | PTEAM_MEM_ALLOC { }
                    | THREAD_MEM_ALLOC { }
                    | EXPR_STRING { }
                    ;

private_clause : PRIVATE {if (enDistributeExtendido) {guardarLineasDist += " private("; meterEnClause = 1;}} '(' var_list ')' {if (enDistributeExtendido) {guardarLineasDist += ")"; meterEnClause = 0;}}
               ;

alloc_clause : ALLOC '(' var_list_cluster ')' { MPIAlloc(); };

broad_clause : BROAD { } '(' var_list_cluster ')' { MPIBroad(); };

scatter_clause : SCATTER { } '(' var_chunk_list_cluster ')'  {} ;
			   
halo_clause : HALO { } '(' var_chunk ')'
	{ 
		if(update==1){
			MPIUpdateHalo(); 
			update = 0;
		}else{
			MPIScatterHalo(); 
		}
	} ; 
			   
gather_clause : GATHER { } '(' var_chunk_list_cluster ')' ;
			  
allgather_clause : ALLGATHER { } '(' var_chunk_list_cluster ')' ;

firstprivate_clause : FIRSTPRIVATE {if (enDistributeExtendido > 1) {guardarLineasDist += " firstprivate("; meterEnClause = 1;}} '(' var_list ')' {if (enDistributeExtendido > 1) {guardarLineasDist += ")"; meterEnClause = 0;}}
                    ;

copyprivate_clause : COPYPRIVATE { } '(' var_list ')' { }
                   ;
fortran_copyprivate_clause : COPYPRIVATE { } '(' var_list ')' { }
                           ;
lastprivate_clause : LASTPRIVATE {if (enDistributeExtendido) {guardarLineasDist += " lastprivate("; meterEnClause = 1;}} '(' lastprivate_parameter ')' {if (enDistributeExtendido) {guardarLineasDist += ")"; meterEnClause = 0;}}
                    ;

lastprivate_parameter : EXPR_STRING {if (meterEnClause) {guardarLineasDist += $1;}}
                      | EXPR_STRING {if (meterEnClause) {guardarLineasDist += $1; guardarLineasDist += ",";}} ',' var_list
                      | lastprivate_modifier ':' {if (meterEnClause) {guardarLineasDist += $1;guardarLineasDist += ":";}} var_list
                      ;

lastprivate_distribute_clause : LASTPRIVATE { } '(' var_list ')' { }

lastprivate_modifier : MODIFIER_CONDITIONAL {$$ = $1;}
                     ;

linear_clause : LINEAR '(' linear_parameter ')' {if (enDistributeExtendido) {guardarLineasDist += ")"; meterEnClause = 0;}}
              | LINEAR '(' linear_parameter ':' EXPR_STRING {} ')' {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += ":"; guardarLineasDist += $5; guardarLineasDist += ")"; meterEnClause = 0;}}
              ;

linear_parameter : EXPR_STRING {if (meterEnClause) {guardarLineasDist += $1;}}
                 | EXPR_STRING ',' {if (meterEnClause) {guardarLineasDist += $1; guardarLineasDist += ",";}} var_list
                 | linear_modifier {if (meterEnClause) {guardarLineasDist += "(";}} '(' var_list ')' {if (meterEnClause) {guardarLineasDist += ")";}}
                 ;
linear_modifier : MODOFIER_VAL {if (meterEnClause) {guardarLineasDist += $1;}}
                | MODOFIER_REF {if (meterEnClause) {guardarLineasDist += $1;}}
                | MODOFIER_UVAL {if (meterEnClause) {guardarLineasDist += $1;}}
                ;

aligned_clause : ALIGNED '(' aligned_parameter ')' {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += ")"; meterEnClause = 0;}}
               | ALIGNED '(' aligned_parameter ':' EXPR_STRING ')' {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += ":"; guardarLineasDist += $5; guardarLineasDist += ")"; meterEnClause = 0;}}
               ;


aligned_parameter : EXPR_STRING {if(meterEnClause){guardarLineasDist += $1;}}
                  | EXPR_STRING ',' {if(meterEnClause){guardarLineasDist += $1; guardarLineasDist += ",";}} var_list
                  ;

initializer_clause : INITIALIZER '('initializer_expr')';

initializer_expr : OMP_PRIV '=' expr;

expr: EXPR_STRING { };

safelen_clause: SAFELEN {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += " safelen("; meterEnClause = 1;}} '(' var_list ')' {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += ")"; meterEnClause = 0;}}
              ;

simdlen_clause: SIMDLEN {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += " simdlen("; meterEnClause = 1;}} '(' var_list ')' {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5) {guardarLineasDist += ")"; meterEnClause = 0;}}
              ;

nontemporal_clause: NONTEMPORAL {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5){guardarLineasDist += " nontemporal("; meterEnClause = 1;}} '(' var_list ')' {if (enDistributeExtendido == 1 || enDistributeExtendido == 4 || enDistributeExtendido == 5){guardarLineasDist += ")"; meterEnClause = 0;}}
                      ;

collapse_clause: COLLAPSE {if (enDistributeExtendido) {guardarLineasDist += " collapse("; meterEnClause = 1;}} '(' expression ')' {if (enDistributeExtendido) {guardarLineasDist += $4; guardarLineasDist += ")"; meterEnClause = 0;}}
               ;

ordered_clause: ORDERED '(' {if (enDistributeExtendido > 1) {guardarLineasDist += "(";}} expression ')' {if (enDistributeExtendido > 1) {guardarLineasDist += ")"; guardarLineasDist += $4; meterEnClause = 0;}}
              | ORDERED {if (enDistributeExtendido > 1) {meterEnClause = 0;}}
              ;
fortran_nowait_clause: NOWAIT { }
                     ;
nowait_clause: NOWAIT {if (enDistributeExtendido == 2 || enDistributeExtendido == 4){guardarLineasDist += " nowait";}}
             ;
order_clause: ORDER {if(enDistributeExtendido){guardarLineasDist += " order("; meterEnClause = 1;}} '(' order_parameter ')' {if(enDistributeExtendido){guardarLineasDist += ")"; meterEnClause = 0;}}
            ;

order_parameter : CONCURRENT {if(meterEnClause){guardarLineasDist += $1;}}
                ;

uniform_clause: UNIFORM { } '(' var_list ')'
              ;

inbranch_clause: INBRANCH { }
               ;

notinbranch_clause: NOTINBRANCH { }
                  ;
inclusive_clause: INCLUSIVE { } '(' var_list ')'
                ;
exclusive_clause: EXCLUSIVE { } '(' var_list ')'
                ;
allocator_clause: ALLOCATOR '(' allocator1_parameter ')';
allocator1_parameter : DEFAULT_MEM_ALLOC { }
                     | LARGE_CAP_MEM_ALLOC { }
                     | CONST_MEM_ALLOC { }
                     | HIGH_BW_MEM_ALLOC { }
                     | LOW_LAT_MEM_ALLOC { }
                     | CGROUP_MEM_ALLOC { }
                     | PTEAM_MEM_ALLOC { }
                     | THREAD_MEM_ALLOC { }
                     | EXPR_STRING { }
                     ;

dist_schedule_clause : DIST_SCHEDULE '(' dist_schedule_parameter ')' {}
                     ;
dist_schedule_parameter : STATIC { }
                        | STATIC { } ',' EXPR_STRING { }
                        ;
schedule_clause : SCHEDULE {if(enDistributeExtendido > 1){guardarLineasDist += " schedule("; meterEnClause = 1;}} '(' schedule_parameter ')' {if(enDistributeExtendido > 1){guardarLineasDist += ")"; meterEnClause = 0;}}
                ;

schedule_parameter : schedule_kind
                   | schedule_modifier {if (meterEnClause){guardarLineasDist += ":";}} ':' schedule_kind
                   ;


schedule_kind : schedule_enum_kind
              | schedule_enum_kind ','  EXPR_STRING {if (meterEnClause){guardarLineasDist += ","; guardarLineasDist += $3;}}
              ;

schedule_modifier : schedule_enum_modifier {if (meterEnClause){guardarLineasDist += ",";}} ',' schedule_modifier2
                  | schedule_enum_modifier
                  ;

schedule_modifier2 : MODIFIER_MONOTONIC {if (meterEnClause){guardarLineasDist += $1;}}
                   | MODIFIER_NONMONOTONIC {if (meterEnClause){guardarLineasDist += $1;}}
                   | MODIFIER_SIMD {if (meterEnClause){guardarLineasDist += $1;}}
                   ;
schedule_enum_modifier : MODIFIER_MONOTONIC {if (meterEnClause){guardarLineasDist += $1;}}
                       | MODIFIER_NONMONOTONIC {if (meterEnClause){guardarLineasDist += $1;}}
                       | MODIFIER_SIMD {if (meterEnClause){guardarLineasDist += $1;}}
                       ;

schedule_enum_kind : STATIC {if (meterEnClause){guardarLineasDist += "static";}}
                   | DYNAMIC {if (meterEnClause){guardarLineasDist += "dynamic";}}
                   | GUIDED {if (meterEnClause){guardarLineasDist += "guided";}}
                   | AUTO {if (meterEnClause){guardarLineasDist += "auto";}}
                   | RUNTIME {if (meterEnClause){guardarLineasDist += "runtime";}}
                   ;  
shared_clause : SHARED {if (enDistributeExtendido > 1){guardarLineasDist += " shared("; meterEnClause = 1;}} '(' var_list ')' {if (enDistributeExtendido > 1){guardarLineasDist += ")"; meterEnClause = 0;}}
              ;

reduction_clause : REDUCTION '(' reduction_parameter ':' var_list ')'
                 ;

reduction_clause_cluster : REDUCTION { aumentarReduction(); if(enDistributeExtendido){guardarLineasDist += " reduction("; meterEnClause = 1;} } '(' reduction_parameter_cluster {if(enDistributeExtendido){guardarLineasDist += ":";}} ':' var_list_reduction ')' {if(enDistributeExtendido){guardarLineasDist += ")"; meterEnClause = 0;}}
                         ;


allreduction_clause_cluster : ALLREDUCTION { aumentarAllReduction(); } '(' allreduction_parameter_cluster ':' var_list_allreduction ')'
                            ;

allreduction_parameter_cluster : allreduction_identifier_cluster
                               | reduction_modifier ',' reduction_identifier_cluster

reduction_parameter_cluster : reduction_identifier_cluster
                            | reduction_modifier {if (meterEnClause){guardarLineasDist += ",";}} ',' reduction_identifier_cluster
                            ;

allreduction_identifier_cluster : reduction_enum_identifier { if(enAllReductionCluster || enAllReductionDistribute){addAllReduce(false, $1);}}
                                | EXPR_STRING { }
                                ;
                    
reduction_identifier_cluster : reduction_enum_identifier {if (meterEnClause){guardarLineasDist += $1;} if(enReductionCluster || enReductionDistribute){addReduce(false, $1);}}
                              | EXPR_STRING {if (meterEnClause) {guardarLineasDist += $1;}}
                              ;

reduction_parameter : reduction_identifier {}
                    | reduction_modifier ',' reduction_identifier
                    ;

reduction_identifier : reduction_enum_identifier {}
                     | EXPR_STRING { }
                     ;

reduction_modifier : MODIFIER_INSCAN {if (meterEnClause){guardarLineasDist += $1;}}
                   | MODIFIER_TASK {if (meterEnClause){guardarLineasDist += $1;}}
                   | MODIFIER_DEFAULT {if (meterEnClause){guardarLineasDist += $1;}}
                   ;

reduction_enum_identifier : '+'{ $$ = "+"; }
                          | '-'{ $$ = "-"; }
                          | '*'{ $$ = "*"; }
                          | '&'{ $$ = "&"; }
                          | '|'{ $$ = "|"; }
                          | '^'{ $$ = "^"; }
                          | LOGAND{ $$ = "LOGAND"; }
                          | LOGOR{ $$ = "LOGOR"; }
                          | MAX{ $$ = "MAX"; }
                          | MIN{ $$ = "MIN"; }
                          ;

reduction_default_only_clause : REDUCTION { } '(' reduction_default_only_parameter ':' var_list ')' {
                              }
                              ;

reduction_default_only_parameter : reduction_identifier {}
                                 | reduction_default_only_modifier ',' reduction_identifier
                                 ;

reduction_default_only_modifier : MODIFIER_DEFAULT { }
                                ;

%%

int yyerror(const char *s) {
    // printf(" %s!\n", s);
    fprintf(stdout,"error: %s\n",s);
    return 0;
}
 
int yywrap()
{
    return 1;
} 

// Standalone ompparser
void parseOpenMP(const char* _input, void * _exprParse(const char*)) {
    const char *input = _input;
    start_lexer(input);
    yyparse();
    end_lexer();
}
