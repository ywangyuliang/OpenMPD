#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <iostream>
#include <string>
#include <string.h>
#include <cstdio>
#include <fstream>
#include "SymbolTable.h"
#include <vector>
#include "cluster_stack.h"

using namespace std;

vector<const char *> args;
char *declare = NULL;
int tamDeclare = 0;
std::vector<std::vector<const char *>> argsScatter;
std::vector<std::vector<const char *>> argsGather;
std::vector<std::vector<const char *>> argsAllGather;
std::vector<const char *> argsReduceOpsCluster;
std::vector<std::vector<const char *>> argsReduceVarsCluster;
std::vector<const char *> argsAllReduceOpsCluster;
std::vector<std::vector<const char *>> argsAllReduceVarsCluster;
std::vector<const char *> argsReduceOpsDistribute;
std::vector<std::vector<const char *>> argsReduceVarsDistribute;
std::vector<const char *> argsAllReduceOpsDistribute;
std::vector<std::vector<const char *>> argsAllReduceVarsDistribute;
string scheduleDist = "";

string DeclareTypes = "void DeclareTypesMPI() {\n";

std::vector<std::string> varsReduceConstruir;
std::vector<std::vector<std::string>> reduceConst;

std::vector<std::vector<std::string>> tiposMPI;

//HALO
int pendingHaloDistribute = 0;
std::string pendingHaloCode = "";

extern ofstream output, errFile;

extern int statementZone;
extern int zonaPragma;
extern int finalizeOK;
extern int chunk_pos;
extern int enDistribute;
extern char * conArgc;
extern char * conArgv;
extern int enSecuencial;

extern int chunk;

extern long posInit;
extern long posVarsInit;

extern int enNumTeams;
extern std::string numTeamsExpr;

extern SymbolTable table;

int matrizTipos[11][11] = {{-1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1},
                {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2},
                {-1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1},
                {-1, -1, -1, -1, -1, -1, 1, -1, 0, -1, -1},
                {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2},
                {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1},
                {-1, 0, -1, 0, -1, -1, 1, -1, -1, -1, 1},
                {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                {-1, -1, -2, 1, -1, -1, -1, -1, -1, -1, -1},
                {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                {0, -2, -1, -1, -1, 0, 0, -1, -1, -1, -1}};

//-------------------------------AUXILIARES--------------------
void finSecuencial(){
    zonaPragma = 1;
    output << "\t}" << endl;
}

void aumentarReduction() {
    if (enDistribute)
        argsReduceVarsDistribute.push_back({});
    else
        argsReduceVarsCluster.push_back({});
}

void aumentarAllReduction() {
    if (enDistribute)
        argsAllReduceVarsDistribute.push_back({});
    else
        argsAllReduceVarsCluster.push_back({});
}

void aumentarScatter() {
    argsScatter.push_back({});
}

void aumentarGather() {
    argsGather.push_back({});
}

void aumentarAllGather() {
    argsAllGather.push_back({});
}

void addArgScatter(const char *arg) {
    argsScatter.at(chunk_pos).push_back(arg);
}

void addArgGather(const char *arg) {
    argsGather.at(chunk_pos).push_back(arg);
}

void addArgAllGather(const char *arg) {
    argsAllGather.at(chunk_pos).push_back(arg);
}

void addArg(const char *arg){
    args.push_back(arg);
}

void addReduce(bool vars, const char *arg) {
    if (vars) {
        if (enDistribute)
            argsReduceVarsDistribute.at(argsReduceVarsDistribute.size() - 1).push_back(arg);
        else
            argsReduceVarsCluster.at(argsReduceVarsCluster.size() - 1).push_back(arg);
    }
    else {
        if (enDistribute)
            argsReduceOpsDistribute.push_back(arg);
        else
            argsReduceOpsCluster.push_back(arg);
    }
}

void addAllReduce(bool vars, const char *arg) {
    if (vars) {
        if (enDistribute)
            argsAllReduceVarsDistribute.at(argsAllReduceVarsDistribute.size() - 1).push_back(arg);
        else
            argsAllReduceVarsCluster.at(argsAllReduceVarsCluster.size() - 1).push_back(arg);
    }
    else {
        if (enDistribute)
            argsAllReduceOpsDistribute.push_back(arg);
        else
            argsAllReduceOpsCluster.push_back(arg);
    }
}

// HALO: DECLARACION EN CLUSTER ANTES DE USO CON UPDATE HALO JUNTO CON DISTRIBUTE
std::vector<std::string> declaredHaloVars;

std::string getHaloVarName(std::string arg) {
    size_t pos = arg.find('[');

    if (pos == std::string::npos) {
        return arg;
    }

    return arg.substr(0, pos);
}

void addDeclaredHalo(const char *arg) {
    std::string name = getHaloVarName(std::string(arg));

    for (unsigned long int i = 0; i < declaredHaloVars.size(); i++) {
        if (declaredHaloVars.at(i) == name) {
            return;
        }
    }

    declaredHaloVars.push_back(name);
}

int isHaloDeclared(const char *arg) {
    std::string name = getHaloVarName(std::string(arg));

    for (unsigned long int i = 0; i < declaredHaloVars.size(); i++) {
        if (declaredHaloVars.at(i) == name) {
            return 1;
        }
    }

    return 0;
}

void clearDeclaredHalos() {
    declaredHaloVars.clear();
}

string aMayuscula(string cadena) {
  for (string::size_type i = 0; i < cadena.length(); i++) cadena[i] = toupper(cadena[i]);
  return cadena;
}
string aMinuscula(string cadena) {
  for (string::size_type i = 0; i < cadena.length(); i++) cadena[i] = tolower(cadena[i]);
  return cadena;
}

string translateTypes(string type) {
    string typeRes = "MPI";
    vector<string> parts;

    char *structComp = strstr(type.data(), "STRUCT");
    if (structComp) {
        structComp += 7;
    }
    else {
        structComp = strstr(type.data(), "USER_DEFINED");

        if (structComp) {
            structComp += 7;
        }
    }

    for (unsigned long int i = 0; i < tiposMPI.size(); i++) {
        for (unsigned long int j = 1; j < tiposMPI.at(i).size(); j++) {
            if (structComp) {
                if (strcmp(structComp, tiposMPI.at(i).at(j).data()) == 0) {
                    typeRes = tiposMPI.at(i).at(0);
                    return typeRes;
                }
            }
            else {
                if (strcmp(type.data(), tiposMPI.at(i).at(j).data()) == 0) {
                    typeRes = tiposMPI.at(i).at(0);
                    return typeRes;
                }
            }
        }
    }

    if (structComp) {
        fprintf(stderr, "tipo de MPI: %s, no declarado\n", type.data());
        exit(49);
    }

    for (long unsigned int i = 0; i < type.size(); i++) {
        string x = "";

        for (; i < type.size(); i++) {
            if (type.at(i) == '_' || type.at(i) == ' ') {
                break;
            }
            else {
                x += type.at(i);
            }
        }

        if (x != "") {
            int fila = -1;
            string y = aMayuscula(x);
            if (y == "CHAR") {
                fila = 0;
            }
            else if (y == "INT") {
                fila = 1;
            }
            else if (y == "FLOAT") {
                fila = 2;
            }
            else if (y == "DOUBLE") {
                fila = 3;
            }
            else if (y == "BOOL") {
                fila = 4;
            }
            else if (y == "SHORT") {
                fila = 5;
            }
            else if (y == "LONG") {
                fila = 6;
            }
            else if (y == "BYTE") {
                fila = 7;
            }
            else if (y == "COMPLEX") {
                fila = 8;
            }
            else if (y == "SIGNED") {
                fila = 9;
            }
            else if (y == "UNSIGNED") {
                fila = 10;
            }
            else {
                continue;
            }

            if (parts.size() == 0 && fila >= 0) {
                parts.insert(parts.begin(), x);
                continue;
            }

            for (long unsigned int j = 0; j < parts.size(); j++) {
                int col;

                if (parts.at(j) == "CHAR") {
                    col = 0;
                }
                else if (parts.at(j) == "INT") {
                    col = 1;
                }
                else if (parts.at(j) == "FLOAT") {
                    col = 2;
                }
                else if (parts.at(j) == "DOUBLE") {
                    col = 3;
                }
                else if (parts.at(j) == "BOOL") {
                    col = 4;
                }
                else if (parts.at(j) == "SHORT") {
                    col = 5;
                }
                else if (parts.at(j) == "LONG") {
                    col = 6;
                }
                else if (parts.at(j) == "BYTE") {
                    col = 7;
                }
                else if (parts.at(j) == "COMPLEX") {
                    col = 8;
                }
                else if (parts.at(j) == "SIGNED") {
                    col = 9;
                }
                else if (parts.at(j) == "UNSIGNED") {
                    col = 10;
                }

                int elem = matrizTipos[fila][col];

                switch (elem) {
                    case -2: parts.insert(parts.begin() + j, y);
                            parts.erase(parts.begin() + j + 1);
                            break;
                    case 0: parts.insert(parts.begin() + j, y);
                            break;
                    case 1: parts.insert(parts.begin() + j + 1, y);
                            break;
                }

                if (elem != -1) {
                    break;
                }
            }
        }
    }

    for (long unsigned int i = 0; i < parts.size(); i++) {
        typeRes += ("_" + parts.at(i));
    }

    if (typeRes == "MPI") {
        fprintf(stderr, "tipo de MPI: %s, no permitido\n", type.data());
        exit(50);
    }

    return typeRes;
}

    vector<string> extractValues(string str) {
        vector<string> values;
        bool variable = true;
        bool abierto = false;

        for (long unsigned int i = 0; i < str.size(); i++) {
            string val = "";

            for (; i < str.size(); i++) {
                if(str.at(i) == ']') {
                    if (!abierto) {
                        fprintf(stderr, "'[' expected before ']' in cluster alloc\n");  
                        exit(80);
                    }
                    abierto = false;
                    break;
                }
                else if(str.at(i) == '[') {
                    abierto = true;
                    if (variable) {
                        break;
                    }
                }
                else {
                    val += str.at(i);
                }
            }

            if (abierto && variable) {
                variable = false;
            }
            else if (abierto) {
                fprintf(stderr, "']' expected in cluster alloc\n");
            }

            values.push_back(val);
        }

        return values;
    }

int countBrackets(string str) {
    int counter = 0;
    for (char c : str) {
        if (c == '[') {
            counter++;
        }
    }
    return counter;
}

vector<string> splitMatAIndices(const string& input) {
    size_t start_bracket1 = input.find('[');
    size_t end_bracket1 = input.find(']');
    size_t start_bracket2 = input.find('[', end_bracket1);
    size_t end_bracket2 = input.find(']', start_bracket2);

    string matA = input.substr(0, start_bracket1);
    string fA = input.substr(start_bracket1 + 1, end_bracket1 - start_bracket1 - 1);
    string cA = input.substr(start_bracket2 + 1, end_bracket2 - start_bracket2 - 1);

    return {matA, fA, cA};
}

void splitMatIndex(vector<string>& vec, const string& input) {
    size_t start_bracket1 = input.find('[');
    size_t end_bracket1 = input.find(']');
    size_t start_bracket2 = input.find('[', end_bracket1);
    size_t end_bracket2 = input.find(']', start_bracket2);

    string val1 = input.substr(0, start_bracket1);
    string val2 = input.substr(start_bracket1 + 1, end_bracket1 - start_bracket1 - 1);
    string val3 = input.substr(start_bracket2 + 1, end_bracket2 - start_bracket2 - 1);

	vec.push_back(val1);
	vec.push_back(val2);
	vec.push_back(val3);
}

void preConfPragma(){
    //statementZone = 1; //puede dar problemas
    if(zonaPragma==0 && enSecuencial){
        finSecuencial();
    }
}

vector<string> argFormat(string arg) {
    vector<string> result;

    size_t startBracketPos = arg.find('[');
    size_t colonPos = arg.find(':');
    size_t endBracketPos = arg.find(']');

    if (startBracketPos != string::npos && colonPos != string::npos && endBracketPos != string::npos) {
        string part1 = arg.substr(0, startBracketPos);
        result.push_back(part1);

        string part2 = arg.substr(startBracketPos + 1, colonPos - startBracketPos - 1);
        result.push_back(part2);

        string part3 = arg.substr(colonPos + 1, endBracketPos - colonPos - 1);
        result.push_back(part3);
    } else {
        cerr << "Invalid format: " << arg << endl;
    }

    return result;
}

vector<string> scatterArgs(){
	vector<string> result = splitMatAIndices(args.at(0));
	result.push_back(args.at(1));
	return result;
}

vector<string> scatterHaloArgs(){
	vector<string> result;
	splitMatIndex(result, args.at(0));
	result.push_back(args.at(1));
	return result;
}

//-------------------------------PLANTILLAS--------------------
void MPIInit(){
    long posActual = output.tellp();

    output.seekp(20);

    output.write("#include <assert.h>\n#include <mpi.h>", 36);

    output.seekp(posVarsInit);

    output.write("void DeclareTypesMPI();\n\nint __taskid = -1, __numprocs = -1;\nMPI_Status __status;\n\n", 83);
    
    output.seekp(posActual);
    statementZone = 1;
}

void MPIInitParte2(){
    output.seekp(posInit);
    char texto[152];
    strcpy(texto, "\tMPI_Init(");

    if (conArgc != NULL && conArgv != NULL) {
        texto[10] = '&';
        strcpy(texto+11, conArgc);
        strcpy(texto+15, ",&");
        strcpy(texto+17, conArgv);
    }
    else if (conArgc != NULL) {
        texto[10] = '&';
        strcpy(texto+11, conArgc);
        strcpy(texto+15, ", NULL");
    }
    else if (conArgv != NULL) {
        strcpy(texto + 10, "NULL, &");
        strcpy(texto + 17, conArgv);
    }
    else {
        strcpy(texto + 10, "NULL , NULL");
    }

    if (!enSecuencial) {
        strcpy(texto + 21, ");\n\tMPI_Comm_size(MPI_COMM_WORLD,&__numprocs);\n\tMPI_Comm_rank(MPI_COMM_WORLD,&__taskid);\n\tDeclareTypesMPI();\nif (__taskid == 0) {\n");
    }
    else {
        strcpy(texto + 21, ");\n\tMPI_Comm_size(MPI_COMM_WORLD,&__numprocs);\n\tMPI_Comm_rank(MPI_COMM_WORLD,&__taskid);\n\tDeclareTypesMPI();\n                    \n");
    }

    output.write(texto, 151);

    enSecuencial = 1;
    
    statementZone = 1;
}

void IncludeString() {
    long posActual = output.tellp();

    output.seekp(0);

    output.write("#include <string.h>", 19);

    output.seekp(posActual);
}

void MPIEmpezarSecuencial() {
    if (!enSecuencial) {
        output << "if (__taskid == 0) {" << endl;
        enSecuencial = 1;
    }
}

void MPINumTeams(){
    if(enNumTeams){
        output << "if (__numprocs != " << numTeamsExpr << ") {\n";
        output << "\tprintf(\"Error, num_teams debe ser " << numTeamsExpr << "\\n\");\n";
        output << "\tMPI_Finalize();\n";
        output << "\treturn(-1);\n";
        output << "}\n";

        enNumTeams = 0;
        numTeamsExpr = "";
    }
}

void MPIWriteCluster() {
    if (enSecuencial) {
        output << "}" << endl;
        enSecuencial = 0;
    }
}

void MPIFinalize(){
    if(cluster_stack_isEmpty()){
        MPIWriteCluster();
    }
    finalizeOK = 1;
    string finalize=    "\tMPI_Finalize();";
    output << finalize << endl;
}

    void MPIAlloc() {
        if(args.size() < 1){
            errFile << "Error: Alloc pragma must have 1 argument and it has " << args.size() << endl;
            exit(1);
        }

        string alloc = "";

        for (long unsigned int i = 0; i < args.size(); i++) {
            const char *arg = args.at(i);
            vector<string> values = extractValues(std::string(arg));
            SymbolInfo *infoVar = table.getSymbolInfo(values.at(0));

            alloc += "if (__taskid != 0) {\n";
            alloc += ("\t" + values.at(0) + " = (" + aMinuscula(infoVar->getVariableType()) + " ");
            for (long unsigned int j = 1; j < values.size(); j++) {
                alloc += "*";
            }
            alloc += (") malloc(" + values.at(1) + " * sizeof(" + aMinuscula(infoVar->getVariableType()) + " ");
            for (long unsigned int j = 2; j < values.size(); j++) {
                alloc += "*";
            }
            alloc += "));\n";

            for (long unsigned int j = 2; j < values.size(); j++) {
                string indentacion = "";
                for (long unsigned int k = 1; k < j; k++) {
                    indentacion += "\t";
                }

                alloc += (indentacion + "for (int __alloc" + std::to_string(j-2) + " = 0; __alloc" + std::to_string(j-2) 
                + " < " + values.at(j) + "; __alloc" + std::to_string(j-2) + "++) {\n");

                alloc += "\t" + indentacion + values.at(0);

                for (long unsigned int k = 0; k < j - 1; k++) {
                    alloc += "[__alloc" + std::to_string(k) + "]";
                }

                alloc += (" = (" + aMinuscula(infoVar->getVariableType()) + " ");
                for (long unsigned int k = j; k < values.size(); k++) {
                    alloc += "*";
                }
                alloc += (") malloc(" + values.at(j) + " * sizeof(" + aMinuscula(infoVar->getVariableType()) + " ");
                for (long unsigned int k = j + 1; k < values.size(); k++) {
                    alloc += "*";
                }
                alloc += ("));\n");
            }

            for (long unsigned int j = 2; j < values.size(); j++) {
                alloc += "\t}\n";
            }

            alloc += "}\n";

            output  << alloc << endl;

            alloc = "";
        }

        args.clear();
    }

    void MPIBroad(){
    	string broad = "";
        for(const auto& arg : args){
            std::vector<std::string> values = extractValues(arg);
            if (values.size() == 0) {
                fprintf(stderr, "Sin variable en broad\n");
                exit(126);
            }
            else if (values.size() == 1) {
                SymbolInfo *sim = table.getSymbolInfo(values.at(0));
    		    if(sim->isArray()){
    		    	broad += "MPI_Bcast(" + string(values.at(0)) + ", " + sim->getSizeList() + ", " + translateTypes(sim->getVariableType())  + ", 0, MPI_COMM_WORLD);\n";
    		    }
    		    else{
    		    	broad += "MPI_Bcast(&" + string(values.at(0)) + ", 1, " + translateTypes(sim->getVariableType()) + ", 0, MPI_COMM_WORLD);\n";
    		    }
            }
            else {
                SymbolInfo *sim = table.getSymbolInfo(values.at(0));
                string mult = "";
                for (unsigned long int i = 2; i < values.size(); i++) {
                    mult += "*";
                    mult += values.at(i);
                }

    		    broad += "MPI_Bcast(" + string(values.at(0)) + ", " + values.at(1) + mult + ", " + translateTypes(sim->getVariableType())  + ", 0, MPI_COMM_WORLD);\n";
            }
    	}
    	output << broad << endl;
        args.clear();
    }

// Deleted, use directive broad
/*
void MPIScatterHalo(){

	vector<string> argsScatterHalo = scatterHaloArgs();

	string uno = argsScatterHalo.at(0);//w
	string dos = argsScatterHalo.at(1);//M
	string tres = argsScatterHalo.at(2);//N

	SymbolInfo *sim1 = table.getSymbolInfo(uno);

	string scatterHalo = 
		"\tdouble __" + uno + "[" + dos+ "][" + tres+ "];\n"
		"\tif (__taskid == 0)\n"
		"\t\tfor ( i = 0; i < M; i++ )\n"
		"\t\t\tfor ( j = 0; j < " + tres+ "; j++ )\n" 
		"\t\t\t\t__" + uno + "[i][j] = " + uno + "[i][j];\n"
		"\tMPI_Bcast(&__" + uno + "[0][0], " + dos + "*" + tres+ ", MPI_" + sim1->getVariableType() + ", 0, MPI_COMM_WORLD);\n";
	output << scatterHalo << endl;
}
*/

void MPIUpdateHalo(){

	vector<string> argsScatterHalo = scatterHaloArgs();

	string uno = argsScatterHalo.at(0);//w
	string dos = argsScatterHalo.at(1);//M
	string tres = argsScatterHalo.at(2);//N
	SymbolInfo *sim1 = table.getSymbolInfo(uno);

    string updateHalo;
    /*
		"\t\tint __chunk;\n"
		"\t\tint __start_" + uno + ";\n"
		"\t\tint __end_" + uno + ";\n"
		"\t\t__chunk = (" + dos + " / __numprocs);\n"
		"\t\tif (__taskid < (" + dos + " % __numprocs))\n"
		"\t\t\t__chunk++;\n"
		"\t\t__start_" + uno + " =  __chunk * __taskid;\n"
		"\t\tif (__taskid >= (" + dos + " % __numprocs))"
		"\t\t\t__start_" + uno + " += " + dos + " % __numprocs;\n"
		"\t\t__end_" + uno + " = __start_" + uno + " + __chunk ;\n"
		"\t\tif (__taskid == 0) __start_" + uno + " = __start_" + uno + " - 1;\n"
		"\t\tif (__taskid == (__numprocs-1)) __end_" + uno + " = __end_" + uno + " - 1;\n"
		"\t\tif (__taskid == (__numprocs-1)) assert (__end_" + uno + " == (" + dos + "-1));\n"
    */

    //Caijie

        /*		
        updateHalo = 
        "\t\t\tif(__taskid > 0)\n"
		"\t\t\t\tMPI_Isend(__" + uno + "[__start_" + uno + "], 1*" + tres + ", MPI_" + sim1->getVariableType() + ", __taskid - 1, 100, MPI_COMM_WORLD, &reqs);\n"
		"\t\t\tif(__taskid < __numprocs - 1)\n"
		"\t\t\t\tMPI_Isend(__" + uno + "[__end_" + uno + "-1], 1*" + tres + ", MPI_" + sim1->getVariableType() + ", __taskid + 1, 101, MPI_COMM_WORLD, &reqs);\n"
		"\t\t\tif(__taskid > 0)\n"
		"\t\t\t\tMPI_Recv(__" + uno + "[__start_" + uno + "-1], 1*" + tres + ", MPI_" + sim1->getVariableType() + ", __taskid - 1, 101, MPI_COMM_WORLD, &status);\n"
		"\t\t\tif(__taskid < __numprocs - 1)\n"
		"\t\t\t\tMPI_Recv(__" + uno + "[__end_" + uno + "], 1*" + tres + ", MPI_" + sim1->getVariableType() + ", __taskid + 1, 100, MPI_COMM_WORLD, &status);\n"
        */
        
        string halo_elems= argsScatterHalo.at(3);
        string halo_rows = halo_elems;

        size_t pos = halo_elems.find('*');
        if (pos != string::npos) {
            halo_rows = halo_elems.substr(0, pos);
        }
        updateHalo = 
	    "\t{\n"
    	"\t\t\tif(__taskid > 0)\n"
		"\t\t\t\tMPI_Isend(" + uno + "[__start],"+ halo_elems+", MPI_" + sim1->getVariableType() + ", __taskid - 1, 100, MPI_COMM_WORLD, &reqs);\n"
		"\t\t\tif(__taskid < __numprocs - 1)\n"
		"\t\t\t\tMPI_Isend(" + uno + "[__end-" + halo_rows + "],"+ halo_elems+", MPI_" + sim1->getVariableType() + ", __taskid + 1, 101, MPI_COMM_WORLD, &reqs);\n"
		"\t\t\tif(__taskid > 0)\n"
		"\t\t\t\tMPI_Recv(" + uno + "[__start-" + halo_rows + "],"+ halo_elems+", MPI_" + sim1->getVariableType() + ", __taskid - 1, 101, MPI_COMM_WORLD, &status);\n"
		"\t\t\tif(__taskid < __numprocs - 1)\n"
		"\t\t\t\tMPI_Recv(" + uno + "[__end],"+ halo_elems+", MPI_" + sim1->getVariableType() + ", __taskid + 1, 100, MPI_COMM_WORLD, &status);\n"
	    "\t}\n\n";
	//output << updateHalo << endl;
        pendingHaloCode = updateHalo;
        pendingHaloDistribute = 1;

    args.clear();
}

void MPIDistribute(string ini, string fin) {
    string distribute;

    if (scheduleDist.size() > 0) {
        distribute =
            "{\nint __iter;\nint __start;\nint __end;\n__iter = __numprocs * " + scheduleDist + ";\n" +
            "__start = " + ini + " + __taskid * " + scheduleDist + ";\n" +
            "__end = " + fin + ";\n";
    }
    else {
        distribute =
            "{\nint __iter;\nint __start;\nint __end;\n__iter = (((" + fin + ") - (" + ini + ")) / __numprocs);\n" +
            "if (__taskid < (((" + fin + ") - (" + ini + ")) % __numprocs))\n" +
            "\t__iter++;\n" +
            "__start = ((" + ini + ") + __iter * __taskid);\n" +
            "if (__taskid >= (((" + fin + ") - (" + ini + ")) % __numprocs))\n" +
            "\t__start += (((" + fin + ") - (" + ini + ")) % __numprocs);\n" +
            "__end = __start + __iter;\nif (__taskid == (__numprocs - 1)) assert(__end == (" + fin + "));\n";
    }
    
    output << distribute << endl;
}

void calcularReduceFinal(bool cluster) {
    std::vector<std::vector<const char *>> *argsReduceVars;
    std::vector<const char *> *argsReduceOps;

    if (cluster) {
        argsReduceVars = &argsReduceVarsCluster;
        argsReduceOps = &argsReduceOpsCluster;
    }
    else {
        argsReduceVars = &argsReduceVarsDistribute;
        argsReduceOps = &argsReduceOpsDistribute;
    }

    std::string finalReduce = "\n";

    if ((*argsReduceOps).size() != (*argsReduceVars).size()) {
        exit(120);
    }

    for (long unsigned int i = 0; i < (*argsReduceOps).size(); i++) {
        string opMPI;
        if (strcmp((*argsReduceOps).at(i), "+") == 0) {
            opMPI = "MPI_SUM";
        }
        else if (strcmp((*argsReduceOps).at(i), "*") == 0) {
            opMPI = "MPI_PROD";
        }
        else if (strcmp((*argsReduceOps).at(i), "MAX") == 0) {
            opMPI = "MPI_MAX";
        }
        else if (strcmp((*argsReduceOps).at(i), "MIN") == 0) {
            opMPI = "MPI_MIN";
        }
        else if (strcmp((*argsReduceOps).at(i), "&") == 0) {
            opMPI = "MPI_LAND";
        }
        else if (strcmp((*argsReduceOps).at(i), "|") == 0) {
            opMPI = "MPI_LOR";
        }
        else if (strcmp((*argsReduceOps).at(i), "^") == 0) {
            opMPI = "MPI_LXOR";
        }
        else {
            fprintf(stderr, "Operacion de reduction no valida\n");
            exit(20);
        }

        for (long unsigned int varIt = 0; varIt < (*argsReduceVars).at(i).size(); varIt++) {
            SymbolInfo *infoVar = table.getSymbolInfo((*argsReduceVars).at(i).at(varIt));
            std::string toUpper;
            std::string toLower;
            toUpper += infoVar->getVariableType();
            toLower += infoVar->getVariableType();

            for (long unsigned int j = 0; j < infoVar->getVariableType().size(); j++) {
                toUpper.at(j) = toupper(infoVar->getVariableType().at(j));
                toLower.at(j) = tolower(infoVar->getVariableType().at(j));
            }

            finalReduce += (toLower + " __" + (*argsReduceVars).at(i).at(varIt) + ";\n");

            finalReduce = finalReduce + "MPI_Reduce(&" + (*argsReduceVars).at(i).at(varIt) + ", &__" + (*argsReduceVars).at(i).at(varIt) + ", 1, " + translateTypes(toUpper) +
             ", " + opMPI + ", 0, MPI_COMM_WORLD);\n";

            finalReduce = finalReduce + "if (__taskid == 0) { " + (*argsReduceVars).at(i).at(varIt) + " = __" + (*argsReduceVars).at(i).at(varIt) + "; }\n";
        }
    }

    output << finalReduce << endl;

    (*argsReduceOps).clear();
    (*argsReduceVars).clear();
}

void calcularAllReduceFinal(bool cluster) {
    std::vector<std::vector<const char *>> *argsAllReduceVars;
    std::vector<const char *> *argsAllReduceOps;

    if (cluster) {
        argsAllReduceVars = &argsAllReduceVarsCluster;
        argsAllReduceOps = &argsAllReduceOpsCluster;
    }
    else {
        argsAllReduceVars = &argsAllReduceVarsDistribute;
        argsAllReduceOps = &argsAllReduceOpsDistribute;
    }

    std::string finalReduce = "\n";

    if ((*argsAllReduceOps).size() != (*argsAllReduceVars).size()) {
        exit(120);
    }

    for (long unsigned int i = 0; i < (*argsAllReduceOps).size(); i++) {
        string opMPI;
        if (strcmp((*argsAllReduceOps).at(i), "+") == 0) {
            opMPI = "MPI_SUM";
        }
        else if (strcmp((*argsAllReduceOps).at(i), "*") == 0) {
            opMPI = "MPI_PROD";
        }
        else if (strcmp((*argsAllReduceOps).at(i), "MAX") == 0) {
            opMPI = "MPI_MAX";
        }
        else if (strcmp((*argsAllReduceOps).at(i), "MIN") == 0) {
            opMPI = "MPI_MIN";
        }
        else if (strcmp((*argsAllReduceOps).at(i), "&") == 0) {
            opMPI = "MPI_LAND";
        }
        else if (strcmp((*argsAllReduceOps).at(i), "|") == 0) {
            opMPI = "MPI_LOR";
        }
        else if (strcmp((*argsAllReduceOps).at(i), "^") == 0) {
            opMPI = "MPI_LXOR";
        }
        else {
            fprintf(stderr, "Operacion de reduction no valida\n");
            exit(20);
        }

        for (long unsigned int varIt = 0; varIt < (*argsAllReduceVars).at(i).size(); varIt++) {
            SymbolInfo *infoVar = table.getSymbolInfo((*argsAllReduceVars).at(i).at(varIt));
            std::string toUpper;
            std::string toLower;
            toUpper += infoVar->getVariableType();
            toLower += infoVar->getVariableType();

            for (long unsigned int j = 0; j < infoVar->getVariableType().size(); j++) {
                toUpper.at(j) = toupper(infoVar->getVariableType().at(j));
                toLower.at(j) = tolower(infoVar->getVariableType().at(j));
            }

            finalReduce += (toLower + " __" + (*argsAllReduceVars).at(i).at(varIt) + ";\n");

            finalReduce = finalReduce + "MPI_Allreduce(&" + (*argsAllReduceVars).at(i).at(varIt) + ", &__" + (*argsAllReduceVars).at(i).at(varIt) + ", 1, " + translateTypes(toUpper) +
             ", " + opMPI + ", MPI_COMM_WORLD);\n";

            finalReduce = finalReduce + (*argsAllReduceVars).at(i).at(varIt) + " = __" + (*argsAllReduceVars).at(i).at(varIt) + ";\n";
        }
    }

    output << finalReduce << endl;

    (*argsAllReduceOps).clear();
    (*argsAllReduceVars).clear();
}

void ReducirReduceConstVariables() {
    for (int i = 0; i < 7; i++) {
        reduceConst.push_back({});
    }

    for (unsigned long int i = 0; i < varsReduceConstruir.size(); i++) {
        string x = "";
        string op;
        int estado = 0;
        reduceConst.push_back({});
        int n = 0;

        for (unsigned long int j = 0; j < varsReduceConstruir.at(i).size(); j++) {
            switch(estado) {
                case 0: if (varsReduceConstruir.at(i).at(j) == ':') {estado = 1; continue;}
                        op += varsReduceConstruir.at(i).at(j);
                        break;
                case 1: if (varsReduceConstruir.at(i).at(j) == ',') {
                            if (op == "+") {
                                reduceConst.at(0).push_back(x);
                            }
                            else if (op == "*") {
                                reduceConst.at(1).push_back(x);
                            }
                            else if (op == "max") {
                                reduceConst.at(2).push_back(x);
                            }
                            else if (op == "min") {
                                reduceConst.at(3).push_back(x);
                            }
                            else if (op == "&") {
                                reduceConst.at(4).push_back(x);
                            }
                            else if (op == "|") {
                                reduceConst.at(5).push_back(x);
                            }
                            else if (op == "^") {
                                reduceConst.at(6).push_back(x);
                            }
                            else {
                                fprintf(stderr, "Operacion de reduction no valida\n");
                                exit(20);
                            }

                            x = "";
                            n++;
                            continue;
                        }

                        x += varsReduceConstruir.at(i).at(j);
                        
                        if (j == varsReduceConstruir.at(i).size() - 1) {
                            if (op == "+") {
                                reduceConst.at(0).push_back(x);
                            }
                            else if (op == "*") {
                                reduceConst.at(1).push_back(x);
                            }
                            else if (op == "max") {
                                reduceConst.at(2).push_back(x);
                            }
                            else if (op == "min") {
                                reduceConst.at(3).push_back(x);
                            }
                            else if (op == "&") {
                                reduceConst.at(4).push_back(x);
                            }
                            else if (op == "|") {
                                reduceConst.at(5).push_back(x);
                            }
                            else if (op == "^") {
                                reduceConst.at(6).push_back(x);
                            }
                            else {
                                fprintf(stderr, "Operacion de reduction no valida\n");
                                exit(20);
                            }
                        }
            }
        }
    }

    varsReduceConstruir.clear();
}

string construirReductionDist(int it) {
    string reduction = " reduction(";

    reduction += aMinuscula(argsReduceOpsDistribute.at(it));

    reduction += ":";

    int op;
    int n_escritos = 0;

    for (unsigned long int i = 0; i < argsReduceVarsDistribute.at(it).size(); i++) {
        if (strcmp(argsReduceOpsDistribute.at(it), "+") == 0) {
            op = 0;
        }
        else if (strcmp(argsReduceOpsDistribute.at(it), "*") == 0) {
            op = 1;
        }
        else if (strcmp(argsReduceOpsDistribute.at(it), "MAX") == 0) {
            op = 2;
        }
        else if (strcmp(argsReduceOpsDistribute.at(it), "MIN") == 0) {
            op = 3;
        }
        else if (strcmp(argsReduceOpsDistribute.at(it), "&") == 0) {
            op = 4;
        }
        else if (strcmp(argsReduceOpsDistribute.at(it), "|") == 0) {
            op = 5;
        }
        else if (strcmp(argsReduceOpsDistribute.at(it), "^") == 0) {
            op = 6;
        }
        else {
            fprintf(stderr, "Operacion de reduction no valida\n");
            exit(20);
        }

        int esta = 0;
        for (unsigned long int j = 0; j < reduceConst.at(op).size(); j++) {
            if (strcmp(argsReduceVarsDistribute.at(it).at(i), reduceConst.at(op).at(j).data()) == 0) {
                esta = 1;
                break;
            }
        }

        if (!esta) {
            if (n_escritos > 0) {
                reduction += ", ";
            }
            reduction += argsReduceVarsDistribute.at(it).at(i);
            n_escritos++;
        }
    }

    reduction += ")";

    if (n_escritos == 0) {
        reduction = "";
    }

    return reduction;
}

string construirAllReductionDist(int it) {
    string reduccion = " reduction(";

    // reduccion += argsAllReduceOpsDistribute.at(it);

    reduccion += aMinuscula(argsAllReduceOpsDistribute.at(it));

    reduccion += ":";

    for (long unsigned int i = 0; i < argsAllReduceVarsDistribute.at(it).size(); i++) {
        if (i != 0) {
            reduccion += ", ";
        }
        reduccion += argsAllReduceVarsDistribute.at(it).at(i);
    }

    reduccion += ")";

    return reduccion;
}

void ScatterConChunk(std::vector<const char *> argsS) {
    std::vector<std::string> vals = extractValues(std::string(argsS.at(0)));
    std::string chunk = argsS.at(1);

    SymbolInfo *infoVar = table.getSymbolInfo(vals.at(0));

    string mult = "";

    for (long unsigned int i = 2; i < vals.size(); i++) {
        mult += "*";
        mult += vals.at(i);
    }

    string scatter =
        "{\n\tint __offset = 0;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        aMinuscula(infoVar->getVariableType()) + " *__" + vals.at(0) + "Aux;\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\t__" + vals.at(0) + "Aux = (" + aMinuscula(infoVar->getVariableType()) + " *) malloc(sizeof(" +
        aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t\tmemcpy(__" + vals.at(0) + "Aux, " + vals.at(0) + ", sizeof(" + aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t}\n\n" +

        "\twhile (__offset < " + vals.at(1) + mult + ") {\n" +
        "\t\tif (__taskid == 0) {\n" +
        "\t\t\tfor (int __gather = 0; __gather < __numprocs; __gather++) {\n" +
        "\t\t\t\tif (__offset < " + vals.at(1) + mult + ") {\n" +
        "\t\t\t\t\t__counts[__gather] = " + chunk + ";\n" +
        "\t\t\t\t\t__displs[__gather] = __offset;\n" +
        "\t\t\t\t\t__offset += " + chunk + ";\n" +
        "\t\t\t\t}\n" +
        "\t\t\t\telse {\n" +
        "\t\t\t\t\t__counts[__gather] = 0;\n" +
        "\t\t\t\t\t__displs[__gather] = " + vals.at(1) + mult + ";\n" +
        "\t\t\t\t}\n" +
        "\t\t\t}\n" +
        "\t\t}\n" +
        "\t\telse {\n" +
        "\t\t\tif (__offset + __taskid*" + chunk + " < " + vals.at(1) + mult + ") {\n" +
        "\t\t\t\t__counts[__taskid] = " + chunk + ";\n" +
        "\t\t\t\t__displs[__taskid] = __offset + __taskid*" + chunk + ";\n" +
        "\t\t\t\t__offset += __numprocs*" + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__taskid] = 0;\n" +
        "\t\t\t\t__displs[__taskid] = " + vals.at(1) + mult + ";\n" +
        "\t\t\t\t__offset += __numprocs*" + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n\n" +

        "\t\tMPI_Scatterv(__" + vals.at(0) + "Aux, __counts, __displs, " + translateTypes(infoVar->getVariableType()) + ", &" + vals.at(0);

        for (size_t j = 0; j < infoVar->getArrListSize(); j++) {
            scatter += "[0]";
        }

        scatter += ("+__displs[__taskid], __counts[__taskid], " + translateTypes(infoVar->getVariableType()) + ", 0, MPI_COMM_WORLD);\n" +

        "\t}\n" +
        "}\n");

    output << scatter << endl;
}

void ScatterSinChunk(std::vector<const char *> argsS) {
    std::vector<std::string> vals = extractValues(std::string(argsS.at(0)));

    SymbolInfo *infoVar = table.getSymbolInfo(vals.at(0));

    string mult = "";

    for (long unsigned int i = 2; i < vals.size(); i++) {
        mult += "*";
        mult += vals.at(i);
    }

    string scatter =
        "{\n\tint __chunk;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        aMinuscula(infoVar->getVariableType()) + " *__" + vals.at(0) + "Aux;\n" +
        "\t__chunk = (" + vals.at(1) + " / __numprocs);\n" +
        "\t__displs[__taskid] = __chunk*__taskid" + mult + ";\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\t" + "__" + vals.at(0) + "Aux = (" + aMinuscula(infoVar->getVariableType()) + " *) malloc(sizeof(" +
        aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t\tmemcpy(__" + vals.at(0) + "Aux, " + vals.at(0) + ", sizeof(" + aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t}\n\n" +

        "\tif (__taskid < (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t__counts[__taskid] = (__chunk + 1)" + mult + ";\n" +
        "\t\t__displs[__taskid] += __taskid" + mult + ";\n" +
        "\t}\n" +
        "\telse {\n" +
        "\t\t__counts[__taskid] = __chunk" + mult + ";\n" +
        "\t\t__displs[__taskid] += (" + vals.at(1) + " % __numprocs)" + mult + ";\n" +
        "\t}\n\n" +

        "\tif (__taskid == 0) {\n" +
        "\t\t__displs[0] = 0;\n\n" +
        "\t\tfor (int __gather = 1; __gather < __numprocs; __gather++) {\n" +
        "\t\t\tif (__gather < (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t\t\t__counts[__gather] = (__chunk + 1)" + mult + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + mult + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse if (__gather == (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t\t\t__counts[__gather] = __chunk" + mult + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + mult + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__gather] = __chunk" + mult + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + __chunk" + mult + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n\n" +
        "\t\tassert((__displs[__numprocs - 1] + __counts[__numprocs - 1]) == " + vals.at(1) + mult + ");\n" +
        "\t}\n\n" +

        "\tMPI_Scatterv(__" + vals.at(0) + "Aux, __counts, __displs, " + translateTypes(infoVar->getVariableType()) + ", &" + vals.at(0);

        for (size_t j = 0; j < infoVar->getArrListSize(); j++) {
            scatter += "[0]";
        }

        scatter += ("+__displs[__taskid], __counts[__taskid], " + translateTypes(infoVar->getVariableType()) + ", 0, MPI_COMM_WORLD);\n" +

        "}\n");

    output << scatter << endl;
}

void MPIScatter() {
    for (unsigned long int i = 0; i < argsScatter.size(); i++) {
        if (argsScatter.at(i).size() == 1) {
            ScatterSinChunk(argsScatter.at(i));
        }
        else if (argsScatter.at(i).size() == 2) {
            ScatterConChunk(argsScatter.at(i));
        }
        else {
            fprintf(stderr, "Numero de argumentos de scatter incorrectos, se tienen: %ld\n", argsScatter.at(i).size());
            exit(210);
        }
    }

    argsScatter.clear();
}

void GatherConChunk(std::vector<const char *> argsG) {
    std::vector<std::string> vals = extractValues(std::string(argsG.at(0)));
    std::string chunk = argsG.at(1);

    SymbolInfo *infoVar = table.getSymbolInfo(vals.at(0));

    string mult = "";

    for (long unsigned int i = 2; i < vals.size(); i++) {
        mult += "*";
        mult += vals.at(i);
    }

    string gather =
        "{\n\tint __offset = 0;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        aMinuscula(infoVar->getVariableType()) + " *__" + vals.at(0) + "Aux;\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\t__" + vals.at(0) + "Aux = (" + aMinuscula(infoVar->getVariableType()) + " *) malloc(sizeof(" +
        aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t}\n\n" +

        "\twhile (__offset < " + vals.at(1) + mult + ") {\n" +
        "\t\tif (__taskid == 0) {\n" +
        "\t\t\tfor (int __gather = 0; __gather < __numprocs; __gather++) {\n" +
        "\t\t\t\tif (__offset < " + vals.at(1) + mult + ") {\n" +
        "\t\t\t\t\t__counts[__gather] = " + chunk + ";\n" +
        "\t\t\t\t\t__displs[__gather] = __offset;\n" +
        "\t\t\t\t\t__offset += " + chunk + ";\n" +
        "\t\t\t\t}\n" +
        "\t\t\t\telse {\n" +
        "\t\t\t\t\t__counts[__gather] = 0;\n" +
        "\t\t\t\t\t__displs[__gather] = " + vals.at(1) + mult + ";\n" +
        "\t\t\t\t}\n" +
        "\t\t\t}\n" +
        "\t\t}\n" +
        "\t\telse {\n" +
        "\t\t\tif (__offset + __taskid*" + chunk + " < " + vals.at(1) + mult + ") {\n" +
        "\t\t\t\t__counts[__taskid] = " + chunk + ";\n" +
        "\t\t\t\t__displs[__taskid] = __offset + __taskid*" + chunk + ";\n" +
        "\t\t\t\t__offset += __numprocs*" + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__taskid] = 0;\n" +
        "\t\t\t\t__displs[__taskid] = " + vals.at(1) + mult + ";\n" +
        "\t\t\t\t__offset += __numprocs*" + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n\n" +

        "\t\tMPI_Gatherv(&" + vals.at(0);
        
        for (size_t j = 0; j < infoVar->getArrListSize(); j++) {
            gather += "[0]";
        }

        gather += ("+__displs[__taskid], __counts[__taskid], " + translateTypes(infoVar->getVariableType()) +
        ", __" + vals.at(0) + "Aux, __counts, __displs, " + translateTypes(infoVar->getVariableType()) + ", 0, MPI_COMM_WORLD);\n" +

        "\t}\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\tmemcpy(" + vals.at(0) + ", __" + vals.at(0) + "Aux, sizeof(" + aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t}\n" +
        "}\n");

    output << gather << endl;
}

void GatherSinChunk(std::vector<const char *> argsG) {
    std::vector<std::string> vals = extractValues(std::string(argsG.at(0)));

    SymbolInfo *infoVar = table.getSymbolInfo(vals.at(0));

    string mult = "";

    for (long unsigned int i = 2; i < vals.size(); i++) {
        mult += "*";
        mult += vals.at(i);
    }

    string gather =
        "{\n\tint __chunk;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        aMinuscula(infoVar->getVariableType()) + " *__" + vals.at(0) + "Aux;\n" +
        "\t__chunk = (" + vals.at(1) + " / __numprocs);\n" +
        "\t__displs[__taskid] = __chunk*__taskid" + mult + ";\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\t" + "__" + vals.at(0) + "Aux = (" + aMinuscula(infoVar->getVariableType()) + " *) malloc(sizeof(" +
        aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t}\n\n" +

        "\tif (__taskid < (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t__counts[__taskid] = (__chunk + 1)" + mult + ";\n" +
        "\t\t__displs[__taskid] += __taskid" + mult + ";\n" +
        "\t}\n" +
        "\telse {\n" +
        "\t\t__counts[__taskid] = __chunk" + mult + ";\n" +
        "\t\t__displs[__taskid] += (" + vals.at(1) + " % __numprocs)" + mult + ";\n" +
        "\t}\n\n" +

        "\tif (__taskid == 0) {\n" +
        "\t\t__displs[0] = 0;\n\n" +
        "\t\tfor (int __gather = 1; __gather < __numprocs; __gather++) {\n" +
        "\t\t\tif (__gather < (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t\t\t__counts[__gather] = (__chunk + 1)" + mult + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + mult + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse if (__gather == (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t\t\t__counts[__gather] = __chunk" + mult + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + mult + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__gather] = __chunk" + mult + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + __chunk" + mult + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n\n" +
        "\t\tassert((__displs[__numprocs - 1] + __counts[__numprocs - 1]) == " + vals.at(1) + mult + ");\n" +
        "\t}\n\n" +

        "\tMPI_Gatherv(&" + vals.at(0);
        
        for (size_t j = 0; j < infoVar->getArrListSize(); j++) {
            gather += "[0]";
        }
        
        gather += ("+__displs[__taskid], __counts[__taskid], " + translateTypes(infoVar->getVariableType()) +
        ", __" + vals.at(0) + "Aux, __counts, __displs, " + translateTypes(infoVar->getVariableType()) + ", 0, MPI_COMM_WORLD);\n" +

        "\tif (__taskid == 0) {\n" +
        "\t\tmemcpy(" + vals.at(0) + ", __" + vals.at(0) + "Aux, sizeof(" + aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t}\n"
        "}\n");

    output << gather << endl;
}

void MPIGather() {
    for (unsigned long int i = 0; i < argsGather.size(); i++) {
        if (argsGather.at(i).size() == 1) {
            GatherSinChunk(argsGather.at(i));
        }
        else if (argsGather.at(i).size() == 2) {
            GatherConChunk(argsGather.at(i));
        }
        else {
            fprintf(stderr, "Numero de argumentos de gather incorrectos, se tienen: %ld\n", argsGather.at(i).size());
            exit(210);
        }
    }

    argsGather.clear();
}

void AllGatherConChunk(std::vector<const char *> argsG) {
    std::vector<std::string> vals = extractValues(argsG.at(0));
    std::string chunk = argsG.at(1);

    SymbolInfo *infoVar = table.getSymbolInfo(vals.at(0));

    string mult = "";

    for (long unsigned int i = 2; i < vals.size(); i++) {
        mult += "*";
        mult += vals.at(i);
    }

    string gather =
        "{\n\tint __offset = 0;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        aMinuscula(infoVar->getVariableType()) + " *__" + vals.at(0) + "Aux = (" + aMinuscula(infoVar->getVariableType()) + " *) malloc(sizeof(" +
        aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n\n" +

        "\twhile (__offset < " + vals.at(1) + mult + ") {\n" +
        "\t\tfor (int __gather = 0; __gather < __numprocs; __gather++) {\n" +
        "\t\t\tif (__offset < " + vals.at(1) + mult + ") {\n" +
        "\t\t\t\t__counts[__gather] = " + chunk + ";\n" +
        "\t\t\t\t__displs[__gather] = __offset;\n" +
        "\t\t\t\t__offset += " + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__gather] = 0;\n" +
        "\t\t\t\t__displs[__gather] = " + vals.at(1) + mult + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n" +

        "\t\tMPI_Allgatherv(" + vals.at(0) + "+__displs[__taskid], __counts[__taskid], " + translateTypes(infoVar->getVariableType()) +
        ", __" + vals.at(0) + "Aux, __counts, __displs, " + translateTypes(infoVar->getVariableType()) + ", MPI_COMM_WORLD);\n" +

        "\t}\n" +
        "\tmemcpy(" + vals.at(0) + ", __" + vals.at(0) + "Aux, sizeof(" + aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "}\n";

    output << gather << endl;
}

void AllGatherSinChunk(std::vector<const char *> argsG) {
    std::vector<std::string> vals = extractValues(std::string(argsG.at(0)));

    SymbolInfo *infoVar = table.getSymbolInfo(vals.at(0));

    string mult = "";

    for (long unsigned int i = 2; i < vals.size(); i++) {
        mult += "*";
        mult += vals.at(i);
    }

    string gather =
        "{\n\tint __chunk;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        aMinuscula(infoVar->getVariableType()) + " *__" + vals.at(0) + "Aux = (" + aMinuscula(infoVar->getVariableType()) + " *) malloc(sizeof(" + 
        aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "\t__chunk = (" + vals.at(1) + " / __numprocs);\n\n" +

        "\t__displs[0] = 0;\n" +
        "\tif (0 < (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t__counts[0] = (__chunk + 1)" + mult + ";\n" +
        "\t}\n" +
        "\telse {\n" +
        "\t\t__counts[0] = __chunk" + mult + ";\n" +
        "\t}\n\n" +

        "\tfor (int __gather = 1; __gather < __numprocs; __gather++) {\n" +
        "\t\tif (__gather < (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t\t__counts[__gather] = (__chunk + 1)" + mult + ";\n" +
        "\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + mult + ";\n" +
        "\t\t}\n" +
        "\t\telse if (__gather == (" + vals.at(1) + " % __numprocs)) {\n" +
        "\t\t\t__counts[__gather] = __chunk" + mult + ";\n" +
        "\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + mult + ";\n" +
        "\t\t}\n" +
        "\t\telse {\n" +
        "\t\t\t__counts[__gather] = __chunk" + mult + ";\n" +
        "\t\t\t__displs[__gather] = __displs[__gather - 1] + __chunk" + mult + ";\n" +
        "\t\t}\n" +
        "\t}\n\n" +
        "\tassert((__displs[__numprocs - 1] + __counts[__numprocs - 1]) == " + vals.at(1) + mult + ");\n\n" +

        "\tMPI_Allgatherv(" + vals.at(0) + "+__displs[__taskid], __counts[__taskid], " + translateTypes(infoVar->getVariableType()) +
        ", __" + vals.at(0) + "Aux, __counts, __displs, " + translateTypes(infoVar->getVariableType()) + ", MPI_COMM_WORLD);\n" +

        "\tmemcpy(" + vals.at(0) + ", __" + vals.at(0) + "Aux, sizeof(" + aMinuscula(infoVar->getVariableType()) + ")*" + vals.at(1) + mult + ");\n" +
        "}\n";

    output << gather << endl;
}

void MPIAllGather() {
    for (unsigned long int i = 0; i < argsAllGather.size(); i++) {
        if (argsAllGather.at(i).size() == 1) {
            AllGatherSinChunk(argsAllGather.at(i));
        }
        else if (argsAllGather.at(i).size() == 2) {
            AllGatherConChunk(argsAllGather.at(i));
        }
        else {
            fprintf(stderr, "Numero de argumentos de allgather incorrectos, se tienen: %ld\n", argsAllGather.at(i).size());
            exit(210);
        }
    }

    argsAllGather.clear();
}

void MPIDeclareCluster() {
    if ((declare = (char *) realloc(declare, tamDeclare + 1)) == NULL) {
        fprintf(stderr, "error en asignacion de memoria dinamica\n");
        exit(40);
    }
    declare[tamDeclare] = '\0';
    
    int positionDeclare = 0;
    char *iniAnalisis;
    std::vector<std::vector<std::vector<std::string>>> campos;
    int n_structs = 0;
    std::vector<int> numcampos;

    while ((iniAnalisis = strstr(declare + positionDeclare, "struct")) != NULL) {
        tiposMPI.push_back({});
        campos.push_back({});
        numcampos.push_back(0);
        
        int deftype;
        if (strstr(declare, "typedef") != NULL) {
            deftype = 1;
        }
        else {
            deftype = 0;
        }

        int estado = 0;
        int counterTipo = 0;
        std::string name = "";
        int n_campos = 0;
        int tipoMetido = 0;

        for (int i = iniAnalisis - declare + 6; i < tamDeclare; i++) {
            if (declare[i] == ' ' || declare[i] == '\t' || declare[i] == '\n') {
                continue;
            }

            switch(estado) {
                case 0: if (declare[i] == '{') {
                            estado = 1;
                            if (name.size() > 0) {
                                for (unsigned long int j = 0; j < tiposMPI.size(); j++) {
                                    for (unsigned long int k = 1; k < tiposMPI.at(j).size(); k++) {
                                        if (name.compare(tiposMPI.at(j).at(k)) == 0) {
                                            fprintf(stderr, "declarando varias veces el mismo tipo MPI\n");
                                            exit(52);
                                        }
                                    }
                                }

                                string tipo = name + "_type_MPI";

                                tiposMPI.at(tiposMPI.size() - 1).push_back(tipo);
                                tiposMPI.at(tiposMPI.size() - 1).push_back(name);

                                tipoMetido = 1;
                            }
                            else {
                                if (!deftype) {
                                    fprintf(stderr, "El tipo MPI que se quiere declarar no tiene nombre\n");
                                    exit(51);
                                }
                            }
                            name = "";
                            continue;
                        }

                        name += declare[i];
                        break;

                case 1: if (declare[i] == '}') {
                            if (deftype) {
                                estado = 3;
                            }
                            else {
                                estado = 4;
                            }
                            break;
                        }

                        if (strncmp(declare + i, "char", 4) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "CHAR";
                            counterTipo++;
                            i += 3;
                        }
                        else if (strncmp(declare + i, "int", 3) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "INT";
                            counterTipo++;
                            i += 2;
                        }
                        else if (strncmp(declare + i, "float", 5) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "FLOAT";
                            counterTipo++;
                            i += 4;
                        }
                        else if (strncmp(declare + i, "double", 6) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "DOUBLE";
                            counterTipo++;
                            i += 5;
                        }
                        else if (strncmp(declare + i, "bool", 4) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "BOOL";
                            counterTipo++;
                            i += 3;
                        }
                        else if (strncmp(declare + i, "short", 5) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "SHORT";
                            counterTipo++;
                            i += 4;
                        }
                        else if (strncmp(declare + i, "long", 4) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "LONG";
                            counterTipo++;
                            i += 3;
                        }
                        else if (strncmp(declare + i, "byte", 4) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "BYTE";
                            counterTipo++;
                            i += 3;
                        }
                        else if (strncmp(declare + i, "complex", 7) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "COMPLEX";
                            counterTipo++;
                            i += 6;
                        }
                        else if (strncmp(declare + i, "signed", 6) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "SIGNED";
                            counterTipo++;
                            i += 5;
                        }
                        else if (strncmp(declare + i, "unsigned", 8) == 0) {
                            if (counterTipo) {
                                name += " ";
                            }
                            name += "UNSIGNED";
                            counterTipo++;
                            i += 7;
                        }
                        else {
                            estado = 2;

                            campos.at(n_structs).push_back({});
                            campos.at(n_structs).at(n_campos).push_back(translateTypes(name));
                            campos.at(n_structs).at(n_campos).push_back(name);
                            name = "";
                            name += declare[i];
                            continue;
                        }
                        break;

                case 2: if (declare[i] == ',') {
                            campos.at(n_structs).at(n_campos).push_back(name);
                            numcampos.at(n_structs)++;
                            name = "";
                            continue;
                        }

                        if (declare[i] == ';') {
                            campos.at(n_structs).at(n_campos).push_back(name);
                            numcampos.at(n_structs)++;
                            name = "";
                            estado = 1;
                            n_campos++;
                            continue;
                        }

                        name += declare[i];
                        break;

                case 3: if (declare[i] == ';') {
                            for (unsigned long int j = 0; j < tiposMPI.size(); j++) {
                                for (unsigned long int k = 1; k < tiposMPI.at(j).size(); k++) {
                                    if (name.compare(tiposMPI.at(j).at(k)) == 0) {
                                        fprintf(stderr, "declarando varias veces el mismo tipo MPI\n");
                                        exit(52);
                                    }
                                }
                            }

                            if (!tipoMetido) {
                                string tipo = name + "_type_MPI";
                                tiposMPI.at(tiposMPI.size() - 1).push_back(tipo);
                            }

                            tiposMPI.at(tiposMPI.size() - 1).push_back(name);
                            estado = 4;
                            break;
                        }

                        name += declare[i];
                        break;
            }

            if (estado == 4) {
                positionDeclare = i;
                break;
            }
        }

        n_structs++;
    }

    for (unsigned long int i = 0; i < campos.size(); i++) {
        string declaracion;
        int posTipo = tiposMPI.size() - campos.size() + i;

        declaracion = ("int __blocklengths_" + tiposMPI.at(posTipo).at(1) + "[" + std::to_string(numcampos.at(i)) + "];\n" +
            "MPI_Datatype __old_types_" + tiposMPI.at(posTipo).at(1) + "[" + std::to_string(numcampos.at(i)) + "];\n" +
            "MPI_Aint __disp_" + tiposMPI.at(posTipo).at(1) + "[" + std::to_string(numcampos.at(i)) + "];\n" +
            "MPI_Aint __lb_" + tiposMPI.at(posTipo).at(1) + ";\n" +
            "MPI_Aint __extent_" + tiposMPI.at(posTipo).at(1) + ";\n");
        
        int it = 0;
        
        for (unsigned long int j = 0; j < campos.at(i).size(); j++) {
            for (unsigned long int k = 2; k < campos.at(i).at(j).size(); k++) {
                string newTypeMPI = translateTypes(campos.at(i).at(j).at(0));
                declaracion += ("__blocklengths_" + tiposMPI.at(posTipo).at(1) + "[" + std::to_string(it) + "] = 1;\n");
                declaracion += ("__old_types_" + tiposMPI.at(posTipo).at(1) + "[" + std::to_string(it) + "] = " + newTypeMPI + ";\n");
                
                if (it == 0) {
                    declaracion += ("MPI_Type_get_extent(" + newTypeMPI + ", &__lb_" + tiposMPI.at(posTipo).at(1) + ", &__extent_" + tiposMPI.at(posTipo).at(1) + ");\n");
                    declaracion += ("__disp_" + tiposMPI.at(posTipo).at(1) + "[" + std::to_string(it) + "] = __lb_" + tiposMPI.at(posTipo).at(1) + ";\n");
                }
                else {
                    declaracion += ("__disp_" + tiposMPI.at(posTipo).at(1) + "[" + std::to_string(it) + "] = __disp_" + tiposMPI.at(posTipo).at(1) + 
                    "[" + std::to_string(it - 1) + "] + __extent_" + tiposMPI.at(posTipo).at(1) + ";\n");
                    declaracion += ("MPI_Type_get_extent(" + newTypeMPI + ", &__lb_" + tiposMPI.at(posTipo).at(1) + ", &__extent_" + tiposMPI.at(posTipo).at(1) + ");\n");
                }

                it++;
            }
        }

        declaracion += ("MPI_Type_create_struct(" + std::to_string(numcampos.at(i)) + ", __blocklengths_" + tiposMPI.at(posTipo).at(1) + ", __disp_" 
        + tiposMPI.at(posTipo).at(1) + ", __old_types_" + tiposMPI.at(posTipo).at(1) + ", &" + tiposMPI.at(posTipo).at(0) + ");\n");
        declaracion += ("MPI_Type_commit(&" + tiposMPI.at(posTipo).at(0) + ");\n");

        DeclareTypes += (declaracion + "\n");

        output << "MPI_Datatype " << tiposMPI.at(posTipo).at(0) << ";" << endl;
    }

    tamDeclare = 0;
    free(declare);
    declare = NULL;
}

#endif