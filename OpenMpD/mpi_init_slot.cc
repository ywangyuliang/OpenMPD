#include "mpi_init_slot.h"

#include <fstream>
#include "cluster_stack.h"

extern std::ofstream output;

extern int enMain;
extern int enSecuencial;
extern int MPIInitMainDone;
extern int activarDeclaracion;

extern long posInit;

extern void MPIInitParte2();

void ensure_mpi_init_slot(){
    if(enMain && posInit == -100){
        if(MPIInitMainDone == 0){
            posInit = output.tellp();
			output.write("                                                                                                                                                      \n", 151);
        } else {
            long posActual = output.tellp();
			posInit = output.tellp();
			MPIInitParte2();
			output.seekp(posActual + 151);
        }
    }

    if (cluster_stack_isEmpty() && (enMain == 2 /*|| enFuncion == 2*/) && !enSecuencial) {
		output << "if (__taskid == 0) {" << std::endl;
		enSecuencial = 1;
	}

    activarDeclaracion = 1;
}
