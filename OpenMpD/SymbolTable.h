#ifndef SYMBOLTABLE_SYMBOLTABLE_H
#define SYMBOLTABLE_SYMBOLTABLE_H

#include<bits/stdc++.h>

typedef struct task_body_expr task_body_expr_t;

using namespace std;

extern ofstream logFile, sym_tables;

class SymbolInfo {
private:
    string symbolName;
    string symbolType;
    SymbolInfo *nextSymbol = nullptr;    //for chaining
    int hashIdx = 0, hashPos = 0;

    string returnType = ""; 
    string varType = "";
    vector <SymbolInfo*>* paramList = nullptr;
    bool isFunc = false;
    bool isDef = false;
    bool isArr = false;
    bool isPoint = false;
    bool isStrct = false;
	bool isType = false;
    vector<string> arrSize;
    
    task_body_expr_t *taskBodyExpr = nullptr;
    task_body_expr_t *taskInitExpr = nullptr;
    
public:
    SymbolInfo(string name, string type) 
        : symbolName(name), symbolType(type) {}

    SymbolInfo() {
        nextSymbol = NULL;
    }

    // copy constructor
    SymbolInfo(const SymbolInfo & s1) {
        symbolName = s1.symbolName;
        symbolType = s1.symbolType;

        // nextSymbol = s1.nextSymbol; 
        nextSymbol = nullptr;

        hashIdx = s1.hashIdx;
        hashPos = s1.hashPos;
		isType = s1.isType;
        returnType = s1.returnType;  
        varType = s1.varType;

        taskBodyExpr = s1.taskBodyExpr;
        taskInitExpr = s1.taskInitExpr;

        arrSize = std::vector<string>();
        arrSize = s1.arrSize;

        // paramList = new vector<SymbolInfo*>;
        // paramList = s1.paramList;

        paramList = nullptr;
        if(s1.paramList != nullptr){
            paramList = new vector<SymbolInfo*>;
            for(SymbolInfo* p : *s1.paramList){
                paramList->push_back(p);
            }
        }
        
        isFunc = s1.isFunc;
        isDef = s1.isDef;
        isArr = s1.isArr;
        isPoint = s1.isPoint;
        isStrct = s1.isStrct;
    }

    void setTaskBodyExpr(task_body_expr_t *expr){
        taskBodyExpr = expr;
    }

    task_body_expr_t *getTaskBodyExpr(){
        return taskBodyExpr;
    }

    void setTaskInitExpr(task_body_expr_t *expr){
        taskInitExpr = expr;
    }

    task_body_expr_t *getTaskInitExpr(){
        return taskInitExpr;
    }

    void setReturnType (string type){
        returnType = type;
    }

    string getReturnType() {
        return returnType;
    }

    void setSymbolName (string name) {
        symbolName = name;
    }

    void setVariableType (string type){
        varType = type;
    }

    string getVariableType() {
        return varType;
    }

	void setSymIsType(bool set){
        isType = set;
    }

    bool getSymIsType(){
        return isType;
    }

    void setParamList(vector <SymbolInfo*>* list){
        if(list == paramList){
            return;
        }

        if(paramList != nullptr){
            delete paramList;
            paramList = nullptr;
        } 

        if(list != nullptr){
            paramList = new vector<SymbolInfo*>(*list);
        } else {
            paramList = nullptr;
        }
    }

    vector <SymbolInfo*>* getParamList(){
        return paramList;
    }

    string getParamListString(){
        string paramListStr = "";
        if (paramList != nullptr && !paramList->empty()){
            for(std::vector<SymbolInfo*>::size_type i = 0; i < paramList->size(); i++){
                paramListStr +=  "< " + paramList->at(i)->getSymbolName() + " , " + paramList->at(i)->getSymbolType() + " , " + paramList->at(i)->getVariableType() + " >";
                if(i < paramList->size() - 1){
                    paramListStr += ", ";
                }
            }
        }
        return paramListStr;
    }

    string getSizeList(){
        string sizeListStr = "";
        for(std::vector<string>::size_type i = 0; i < arrSize.size(); i++){
            sizeListStr += (arrSize.at(i));
            if(i < arrSize.size() - 1){
                sizeListStr += ", ";
            }
        }
        return sizeListStr;
    }

    vector<string> getArrSize() {
        return arrSize;
    }

    void setIsFunction(bool set){
        isFunc = set;
    }

    void setIsStruct(bool set){
        isStrct = set;
    }

    bool isFunction(){
        return isFunc;
    }

    void setIsArray(bool set){
        arrSize = std::vector<string>();
        isArr = set;
    }

    bool isArray(){
        return isArr;
    }

    bool isStruct(){
        return isStrct;
    }
    
    void setIsPointer(bool set){
        isPoint = set;
    }

    bool isPointer(){
        return isPoint;
    }

    void addArrSize(string size){
        arrSize.push_back(size);
    }

    size_t getArrListSize() {
        return arrSize.size();
    }

    void setIsDefined(bool set){
        isDef = set;
    }

    bool isDefined(){
        return isDef;
    }

    string getSymbolName() {
        return symbolName;
    }

    void setSymbolType(string type) {
        symbolType = type;
    }

    string getSymbolType() {
        return symbolType;
    }

    SymbolInfo *getNextSymbol() {
        return nextSymbol;
    }

    void setNextSymbol(SymbolInfo *newSymbol) {
        nextSymbol = newSymbol;
    }

    void setHashIdx(int hashIdx) {
        this->hashIdx = hashIdx;
    }


    void setHashPos(int hashPos) {
        this->hashPos = hashPos;
    }

    string getPosition() {
        return to_string(hashIdx) + ", " + to_string(hashPos);
    }

    ~SymbolInfo() {
        // fprintf(stderr, "[DEBUG] ~SymbolInfo() this=%p name=%s\n", (void*)this, symbolName.c_str());
        if(paramList != nullptr){
            delete paramList;
            paramList = nullptr;
        }
        arrSize.clear();
    }

};

class ScopeTable {
    // the hash table
private:
    string scopeID;
    int childCount;

    long long total_buckets;
    SymbolInfo **chainHashTable;
    ScopeTable *parentScope;

public:
    ScopeTable(int n, ScopeTable *parentScope) {
        childCount = 0;

        this->parentScope = parentScope;

        if (parentScope == NULL) {
//            // cout << "creating  root scope with scope id " << scopeID << endl;
            scopeID = "1";

        } else {
            scopeID = parentScope->scopeID + "." + to_string(parentScope->childCount);
        }

        total_buckets = n;
        chainHashTable = new SymbolInfo *[total_buckets];
        for (int i = 0; i < total_buckets; i++)
            chainHashTable[i] = NULL;
    }

    void setParentScope(ScopeTable *parentScope) {
        ScopeTable::parentScope = parentScope;
    }

    void addChild() {
        childCount++;
    }

    int getChildCount() const {
        return childCount;
    }

    ScopeTable *getParentScope() const {
        return parentScope;
    }

    uint32_t sdbmhash(string key) {
        const char *str = key.c_str();
        uint32_t hash = 0;
        int c;

        while ((c = *str++)){
            hash = c + (hash << 6) + (hash << 16) - hash;
        }
        return hash % total_buckets;
    }

    const string getScopeId() {
        return scopeID;
    }

    void insert(SymbolInfo *symbol) {
        string key = symbol->getSymbolName();

        int posInChain = 0;

        int idx = sdbmhash(key);
        // // cout << "hash index: " << idx << endl;

        // fprintf(stderr, "[DEBUG] insert() symbol=%p name=%s scope=%s bucket=%d\n", (void *)symbol, key.c_str(), scopeID.c_str(), idx);

        if (chainHashTable[idx] == NULL) {
            // simply insert
            chainHashTable[idx] = symbol;
            symbol->setHashIdx(idx);
            symbol->setHashPos(posInChain);
        } else {
            // traverse till the end of the linked list for this hash position
            posInChain++;
            SymbolInfo *currSymbol = chainHashTable[idx];
            while (currSymbol->getNextSymbol() != NULL) {
                currSymbol = currSymbol->getNextSymbol();
                posInChain++;
            }
            currSymbol->setNextSymbol(symbol);
            symbol->setHashIdx(idx);
            symbol->setHashPos(posInChain);
        }

        // // cout << "Inserted in ScopeTable# " << scopeID << " at position " << idx << ", " << posInChain << endl;
        // // logFile << "Inserted in ScopeTable# " << scopeID << " at position " << idx << ", " << posInChain << endl;

    }

    bool deleteSymbolFromCurrScope(string key) {
        int idx = sdbmhash(key);

        SymbolInfo *currSymbol = chainHashTable[idx];

        if (currSymbol == NULL) {
            // cout << key << " not found, cannot delete" << endl;
            // logFile << key << " not found, cannot delete" << endl;
            return false;
        }

        // currSymbol has no further chain
        if (currSymbol->getSymbolName() == key && currSymbol->getNextSymbol() == NULL) {
            delete currSymbol;
            chainHashTable[idx] = NULL;
            return true;
        }

        // has chain
        SymbolInfo *parent = chainHashTable[idx];
        int c = 0;
        while (currSymbol->getSymbolName() != key && currSymbol->getNextSymbol() != NULL) {
            parent = currSymbol;
            currSymbol = currSymbol->getNextSymbol();
            c++;
        }

        // when found match and the symbol is in the middle of the current table
        if (currSymbol->getSymbolName() == key && currSymbol->getNextSymbol() != NULL) {
            // if the deleted item is at the first place of the chain, need to make the next symbol the head of the chain
            if (c == 0) {
                chainHashTable[idx] = currSymbol->getNextSymbol();
            }
            parent->setNextSymbol(currSymbol->getNextSymbol());
            currSymbol->setNextSymbol(NULL);

            delete currSymbol;
            return true;
        } else {
            // the symbol is at the end of the curr table
            parent->setNextSymbol(NULL);
            currSymbol->setNextSymbol(NULL);
            delete currSymbol;
            return true;
        }
        return false;
    }

    void printCurr() {
        bool enteredScope = false;
        bool printEnter = false;

        for (int i = 0; i < total_buckets; i++) {
            SymbolInfo *currSymbol = chainHashTable[i];
            if(enteredScope == false){
                sym_tables << "ScopeTable # " << scopeID << endl;
                enteredScope = true;
            }
            if(currSymbol != NULL){
                // cout << i << " --> ";
                sym_tables << " " << i << " --> ";
                printEnter = true;
            }

            while (currSymbol != NULL) {
                // cout << "< " << currSymbol->getSymbolName() << " : " << currSymbol->getSymbolType() << " > ";
                sym_tables << "| ";
                if(currSymbol->isStruct()){
                    sym_tables << "< " << currSymbol->getSymbolName() << " , " << "Struct Symbol" << " , " << currSymbol->getVariableType() << " , " << currSymbol->getParamListString() << " >";
                }
                else if(currSymbol->isArray()){
                    sym_tables << "< " << currSymbol->getSymbolName() << " , " << "Array Symbol" << " , " << currSymbol->getVariableType() << " < Array Size: " << currSymbol->getSizeList() << " > " << " >";
                }
                else if(currSymbol->isFunction()){
                    sym_tables << "< " << currSymbol->getSymbolName() << " , " << "Function Symbol" << " , " << currSymbol->getVariableType() << " , " << "Parameter List: " << currSymbol->getParamListString() << " >";
                }
                else if(currSymbol->isPointer()){
                    sym_tables << "< " << currSymbol->getSymbolName() << " , " << "Pointer Symbol" << " , " << currSymbol->getVariableType() << " >";
                }
                else{
                    sym_tables << "< " << currSymbol->getSymbolName() << " , " << currSymbol->getVariableType() << " >";
                }
                sym_tables << " |";
                currSymbol = currSymbol->getNextSymbol();
            }
            if(printEnter){
                // cout << endl;
                sym_tables << endl;
                printEnter = false;
            }
        }
        // cout << endl;
        sym_tables << endl;
    }

    SymbolInfo *lookup(string key) {
        int idx = sdbmhash(key);
        SymbolInfo *currSymbol = chainHashTable[idx];
        if (currSymbol == NULL) {
            return NULL;
        }

        int c = 0;
        while (currSymbol != NULL) {
            if (currSymbol->getSymbolName() == key) {
                currSymbol->setHashPos(c);
                return currSymbol;
            }
            c++;
            currSymbol = currSymbol->getNextSymbol();
        }
        return NULL;
    }


    // ~ScopeTable() {
    // cout << "Destroying the current ScopeTable" << endl;

    //     for (int i = 0; i < total_buckets; i++) {

    //         SymbolInfo *tempSymbol = chainHashTable[i];
    //         while(tempSymbol){
    //             // cleaning the chains
    //             SymbolInfo * currNext = tempSymbol->getNextSymbol();
    //             delete tempSymbol;
    //             tempSymbol = currNext;
    //         }
    //         // delete chainHashTable[i];
    //     }
    //     delete[] chainHashTable;
    
    // }

    ~ScopeTable() {
    // logFile << "Destructing ScopeTable with ID: " << scopeID << std::endl;
    // fprintf(stderr, "[DEBUG] ~ScopeTable() ID=%s\n", scopeID.c_str());
    for (int i = 0; i < total_buckets; i++) {
        // logFile << "Starting chain " << i << std::endl;
        SymbolInfo *current = chainHashTable[i];
        while (current != nullptr) {
            SymbolInfo *next = current->getNextSymbol();
            // logFile << "Deleting Symbol: " << current->getSymbolName() << " at chain " << i << std::endl;
            // logFile << "Current Pointer: " << current << ", Next Pointer: " << next << std::endl;
            // delete current;
            current = next;
            if (current == nullptr) {
                // logFile << "End of chain " << i << std::endl;
            }
        }
        chainHashTable[i] = nullptr;  // Explicitly nulling the pointer after clearing the chain
    }
    delete[] chainHashTable; // Deletes the array of pointers
    chainHashTable = nullptr; // Prevents dangling pointer
    logFile << "ScopeTable " << scopeID << " fully destructed." << std::endl;
}

};


class SymbolTable {
private:
    ScopeTable *currScopeTable;
    int tableSize;

public:
    SymbolTable(int tableSize) {
        this->tableSize = tableSize;
        // constructing the root scopeTable here
        currScopeTable = new ScopeTable(tableSize, NULL);
    }

    void enterScope() {
        currScopeTable->addChild();
//        Create a new ScopeTable and make it current one. Also
//        make the previous current table as its parentScopeTable.
        currScopeTable = new ScopeTable(tableSize, currScopeTable);
        // cout << "New ScopeTable with id " << currScopeTable->getScopeId() << " created" << endl;
        logFile << "New ScopeTable with id " << currScopeTable->getScopeId() << " created" << endl;
    }

    
    void exitScope() {
//        Remove the current ScopeTable
        // fprintf(stderr, "[DEBUG] exitScope() ID=%s\n", currScopeTable->getScopeId().c_str());
        printCurrScopeTable();
        if(!isSymbolTableEmpty()){
            if (currScopeTable->getParentScope() == NULL) {
                // reached the root scope
                // cout << "ScopeTable# " << currScopeTable->getScopeId() << " removed" << endl;
                // logFile << "ScopeTable# " << currScopeTable->getScopeId() << " removed" << endl;
                // cout << "Destroying the First Scope" << endl;
                // // logFile << "Destroying the First Scope" << endl;

                // currScopeTable = NULL;
                delete currScopeTable;
                currScopeTable = NULL;
            }
            if (currScopeTable != NULL) {
                ScopeTable *parentScope = currScopeTable->getParentScope();
                // cout << "ScopeTable# " << currScopeTable->getScopeId() << " removed" << endl;
                // logFile << "ScopeTable# " << currScopeTable->getScopeId() << " removed" << endl;
                delete currScopeTable;
                currScopeTable = parentScope;
            }
        }
    }

    bool insert(SymbolInfo *newSymbol) {
//        Insert a symbol in current ScopeTable. Return true for
//        successful insertion and false otherwise.
        if(currScopeTable == NULL){
            currScopeTable = new ScopeTable(tableSize, NULL);
        }
        
        if (currScopeTable->lookup(newSymbol->getSymbolName()) != NULL){

            // cout << "< " << newSymbol->getSymbolName() << " : " << newSymbol->getSymbolType() << " > " << " already exists in current ScopeTable" <<endl;
            // logFile << "< " << newSymbol->getSymbolName() << " : " << newSymbol->getSymbolType() << " > " << " already exists in current ScopeTable" <<endl;
            // delete newSymbol;
            return false;
        }
        
        currScopeTable->insert(newSymbol);

        return true;
    }

    bool remove(string symbol) {
        
        SymbolInfo *currSymbol = currScopeTable->lookup(symbol);
        if (currSymbol != NULL) {
            cout << "Found in ScopeTable# " << currScopeTable->getScopeId() << " at position " << currSymbol->getPosition() << endl;
            cout << "Deleted Entry " << currSymbol->getPosition() << " from current ScopeTable" << endl;

            // // logFile << "Found in ScopeTable# " << currScopeTable->getScopeId() << " at position " << currSymbol->getPosition() << endl;
            // // logFile << "Deleted Entry " << currSymbol->getPosition() << " from current ScopeTable" << endl;
            currScopeTable->deleteSymbolFromCurrScope(symbol);
            return true;
        } 
        
        // cout << symbol << " Not found" << endl;
        return false;
    }

    bool lookupEntire(string symbol) {
        ScopeTable *scope = currScopeTable;
        while (scope) {
            SymbolInfo *currSymbol = scope->lookup(symbol);
            if (currSymbol != NULL) {
                // cout << "Found in ScopeTable# " << scope->getScopeId() << " at position " << currSymbol->getPosition() << endl;
                // // logFile << "Found in ScopeTable# " << scope->getScopeId() << " at position " << currSymbol->getPosition() << endl;
                return true;
            } else scope = scope->getParentScope();
        }
        // cout << symbol << " Not found" << endl;
        // // logFile << symbol << " Not found" << endl;
        return false;
    }

    bool lookup(string symbol) {
        // lookup in the current scope
        SymbolInfo *currSymbol = currScopeTable->lookup(symbol);
        if (currSymbol != NULL) {
            // cout << "Found in ScopeTable# " << scope->getScopeId() << " at position " << currSymbol->getPosition() << endl;
            // // logFile << "Found in ScopeTable# " << scope->getScopeId() << " at position " << currSymbol->getPosition() << endl;
            return true;
        } else return false;

      
    }

    SymbolInfo* getSymbolInfo(string symbol) {
        ScopeTable *scope = currScopeTable;
        while (scope) {
            SymbolInfo *currSymbol = scope->lookup(symbol);
            if (currSymbol != NULL) {
                // cout << "Found in ScopeTable# " << scope->getScopeId() << " at position " << currSymbol->getPosition() << endl;
                // // logFile << "Found in ScopeTable# " << scope->getScopeId() << " at position " << currSymbol->getPosition() << endl;
                return currSymbol;
            } else scope = scope->getParentScope();
        }
        // cout << symbol << " Not found" << endl;
        // // logFile << symbol << " Not found" << endl;
        return NULL;
    }

    void printCurrScopeTable() {
        if(!isSymbolTableEmpty())
            currScopeTable->printCurr();
    }

    string printScopeId() {
        if(!isSymbolTableEmpty())
            return currScopeTable->getScopeId();
        return "No scope table found";
    }

    void printAllScopeTables() {
        ScopeTable *tempScope = currScopeTable;

        while (tempScope) {
            tempScope->printCurr();
            tempScope = tempScope->getParentScope();

        }
    }

    bool isSymbolTableEmpty(){
        if(!currScopeTable){
            // cout << "NO CURRENT SCOPE" << endl;
            // // logFile << "NO CURRENT SCOPE" << endl;
            return true;
        }
        else return false;
    }

    ~SymbolTable() {
    // logFile << "Destructing SymbolTable..." << std::endl;
    
    while (currScopeTable != nullptr) {
        ScopeTable* tempScopeTable = currScopeTable;
        currScopeTable = currScopeTable->getParentScope();  // Move up the chain of scope tables
        // logFile << "Destructing ScopeTable with ID: " << tempScopeTable->getScopeId() << std::endl;
        delete tempScopeTable;  // Delete the current scope table
        tempScopeTable = nullptr;  // Ensure pointer is null after deletion
        if (currScopeTable != nullptr) {
            // logFile << "Moving to parent ScopeTable with ID: " << currScopeTable->getScopeId() << std::endl;
        } else {
            // logFile << "No more ScopeTables to destruct." << std::endl;
        }
    }

    logFile << "SymbolTable fully destructed." << std::endl;
    
    }
};


#endif //SYMBOLTABLE_SYMBOLTABLE_H