#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

/*
 * Symbol table used by the parser and transform modules. It records scoped C
 * symbols, types, functions, arrays, pointers, struct metadata and task-body
 * expressions so later code generation can reason about declarations.
 */

#include<bits/stdc++.h>

typedef struct task_body_expr task_body_expr_t;

using namespace std;

extern ofstream logFile, sym_tables;

/*
 * One entry in the symbol table: a named C symbol together with its parser
 * type, kind (function, array, pointer, struct or type name), array
 * dimensions, parameter list and any attached task-body expressions.
 */
class symbol_info {
private:
    string symbol_name;
    string symbol_type;
    symbol_info *next_symbol = nullptr;
    int hash_bucket_index = 0, hash_chain_position = 0;

    string return_type = "";
    string value_type = "";
    vector <symbol_info*>* parameter_list = nullptr;
    bool is_function_symbol = false;
    bool is_defined_symbol = false;
    bool is_array_symbol = false;
    bool is_pointer_symbol = false;
    bool is_struct_symbol = false;
	bool type_symbol_flag = false;
    vector<string> array_dimensions;

    task_body_expr_t *task_body_expr = nullptr;
    task_body_expr_t *task_init_expr = nullptr;

public:
    /* Creates a symbol with a name and parser type */
    symbol_info(string name, string type)
        : symbol_name(name), symbol_type(type) {}

    /* Creates an empty symbol */
    symbol_info() {
        next_symbol = NULL;
    }

    /* Copies symbol metadata without sharing the hash chain */
    symbol_info(const symbol_info & other) {
        symbol_name = other.symbol_name;
        symbol_type = other.symbol_type;

        next_symbol = nullptr;

        hash_bucket_index = other.hash_bucket_index;
        hash_chain_position = other.hash_chain_position;
		type_symbol_flag = other.type_symbol_flag;
        return_type = other.return_type;
        value_type = other.value_type;

        task_body_expr = other.task_body_expr;
        task_init_expr = other.task_init_expr;

        array_dimensions = std::vector<string>();
        array_dimensions = other.array_dimensions;

        parameter_list = nullptr;
        if(other.parameter_list != nullptr){
            parameter_list = new vector<symbol_info*>;
            for(symbol_info* parameter_symbol : *other.parameter_list){
                parameter_list->push_back(parameter_symbol);
            }
        }

        is_function_symbol = other.is_function_symbol;
        is_defined_symbol = other.is_defined_symbol;
        is_array_symbol = other.is_array_symbol;
        is_pointer_symbol = other.is_pointer_symbol;
        is_struct_symbol = other.is_struct_symbol;
    }

    /* Stores the parsed task body expression attached to this symbol */
    void set_task_body_expr(task_body_expr_t *expr){
        task_body_expr = expr;
    }

    /* Returns the parsed task body expression attached to this symbol */
    task_body_expr_t *get_task_body_expr(){
        return task_body_expr;
    }

    /* Stores the parsed task initializer expression attached to this symbol */
    void set_task_init_expr(task_body_expr_t *expr){
        task_init_expr = expr;
    }

    /* Returns the parsed task initializer expression attached to this symbol */
    task_body_expr_t *get_task_init_expr(){
        return task_init_expr;
    }

    /* Sets the function return type */
    void set_return_type(string type){
        return_type = type;
    }

    /* Returns the function return type */
    string get_return_type() {
        return return_type;
    }

    /* Sets the symbol name */
    void set_symbol_name(string name) {
        symbol_name = name;
    }

    /* Sets the C value type represented by this symbol */
    void set_variable_type (string type){
        value_type = type;
    }

    /* Returns the C value type represented by this symbol */
    string get_variable_type() {
        return value_type;
    }

    /* Marks whether this symbol is a type name */
	void set_is_type_symbol(bool set){
        type_symbol_flag = set;
    }

    /* Returns whether this symbol is a type name */
    bool is_type_symbol(){
        return type_symbol_flag;
    }

    /* Replaces the function parameter list */
    void set_parameter_list(vector <symbol_info*>* list){
        if(list == parameter_list){
            return;
        }

        if(parameter_list != nullptr){
            delete parameter_list;
            parameter_list = nullptr;
        }

        if(list != nullptr){
            parameter_list = new vector<symbol_info*>(*list);
        } else {
            parameter_list = nullptr;
        }
    }

    /* Returns the function parameter list */
    vector <symbol_info*>* get_parameter_list(){
        return parameter_list;
    }

    /* Formats the parameter list for diagnostics */
    string get_parameter_list_string(){
        string parameter_list_text = "";
        if (parameter_list != nullptr && !parameter_list->empty()){
            for(std::vector<symbol_info*>::size_type i = 0; i < parameter_list->size(); i++){
                parameter_list_text +=  "< " + parameter_list->at(i)->get_symbol_name() + " , " + parameter_list->at(i)->get_symbol_type() + " , " + parameter_list->at(i)->get_variable_type() + " >";
                if(i < parameter_list->size() - 1){
                    parameter_list_text += ", ";
                }
            }
        }
        return parameter_list_text;
    }

    /* Formats array dimensions for diagnostics */
    string get_size_list(){
        string size_list_text = "";
        for(std::vector<string>::size_type i = 0; i < array_dimensions.size(); i++){
            size_list_text += (array_dimensions.at(i));
            if(i < array_dimensions.size() - 1){
                size_list_text += "* ";
            }
        }
        return size_list_text;
    }

    /* Returns the raw array dimension list */
    vector<string> get_array_dimensions() {
        return array_dimensions;
    }

    /* Marks whether this symbol represents a function */
    void set_is_function(bool set){
        is_function_symbol = set;
    }

    /* Marks whether this symbol represents a struct */
    void set_is_struct(bool set){
        is_struct_symbol = set;
    }

    /* Returns whether this symbol represents a function */
    bool is_function(){
        return is_function_symbol;
    }

    /* Marks whether this symbol represents an array and clears old dimensions */
    void set_is_array(bool set){
        array_dimensions = std::vector<string>();
        is_array_symbol = set;
    }

    /* Returns whether this symbol represents an array */
    bool is_array(){
        return is_array_symbol;
    }

    /* Returns whether this symbol represents a struct */
    bool is_struct(){
        return is_struct_symbol;
    }

    /* Marks whether this symbol represents a pointer */
    void set_is_pointer(bool set){
        is_pointer_symbol = set;
    }

    /* Returns whether this symbol represents a pointer */
    bool is_pointer(){
        return is_pointer_symbol;
    }

    /* Adds one array dimension expression */
    void add_array_dimension(string size){
        array_dimensions.push_back(size);
    }

    /* Returns the number of stored array dimensions */
    size_t get_array_dimension_count() {
        return array_dimensions.size();
    }

    /* Marks whether this symbol has a definition */
    void set_is_defined(bool set){
        is_defined_symbol = set;
    }

    /* Returns whether this symbol has a definition */
    bool is_defined(){
        return is_defined_symbol;
    }

    /* Returns the symbol name */
    string get_symbol_name() {
        return symbol_name;
    }

    /* Sets the parser-level symbol type */
    void set_symbol_type(string type) {
        symbol_type = type;
    }

    /* Returns the parser-level symbol type */
    string get_symbol_type() {
        return symbol_type;
    }

    /* Returns the next symbol in the hash chain */
    symbol_info *get_next_symbol() {
        return next_symbol;
    }

    /* Sets the next symbol in the hash chain */
    void set_next_symbol(symbol_info *new_symbol) {
        next_symbol = new_symbol;
    }

    /* Stores the hash bucket index for diagnostics */
    void set_hash_bucket_index(int hash_bucket_index) {
        this->hash_bucket_index = hash_bucket_index;
    }


    /* Stores the hash chain position for diagnostics */
    void set_hash_chain_position(int hash_chain_position) {
        this->hash_chain_position = hash_chain_position;
    }

    /* Returns the formatted hash position */
    string get_hash_position() {
        return to_string(hash_bucket_index) + ", " + to_string(hash_chain_position);
    }

    /* Releases memory owned by the symbol */
    ~symbol_info() {
        if(parameter_list != nullptr){
            delete parameter_list;
            parameter_list = nullptr;
        }
        array_dimensions.clear();
    }

};

/*
 * A single lexical scope: a hash table of symbol_info entries linked to its
 * parent scope. Owns its bucket array and provides insert, lookup and delete.
 */
class scope_table {
private:
    string scope_id;
    int child_scope_count;

    long long bucket_count;
    symbol_info **hash_buckets;
    scope_table *parent_scope;

public:
    /* Creates a scope table with a parent scope */
    scope_table(int requested_bucket_count, scope_table *parent_scope) {
        child_scope_count = 0;

        this->parent_scope = parent_scope;

        if (parent_scope == NULL) {
            scope_id = "1";

        } else {
            scope_id = parent_scope->scope_id + "." + to_string(parent_scope->child_scope_count);
        }

        bucket_count = requested_bucket_count;
        hash_buckets = new symbol_info *[bucket_count];
        for (int i = 0; i < bucket_count; i++)
            hash_buckets[i] = NULL;
    }

    /* Sets the parent scope table */
    void set_parent_scope(scope_table *parent_scope) {
        scope_table::parent_scope = parent_scope;
    }

    /* Increments the child scope counter */
    void add_child_scope() {
        child_scope_count++;
    }

    /* Returns how many child scopes were created from this scope */
    int get_child_scope_count() const {
        return child_scope_count;
    }

    /* Returns the parent scope table */
    scope_table *get_parent_scope() const {
        return parent_scope;
    }

    /* Hashes a symbol name into this scope table */
    uint32_t hash_symbol_name(string key) {
        const char *key_chars = key.c_str();
        uint32_t hash = 0;
        int current_char;

        while ((current_char = *key_chars++)){
            hash = current_char + (hash << 6) + (hash << 16) - hash;
        }
        return hash % bucket_count;
    }

    /* Returns the scope identifier */
    const string get_scope_id() {
        return scope_id;
    }

    /* Inserts a symbol into this scope table */
    void insert(symbol_info *symbol) {
        string key = symbol->get_symbol_name();

        int chain_position = 0;

        int bucket_index = hash_symbol_name(key);

        if (hash_buckets[bucket_index] == NULL) {
            hash_buckets[bucket_index] = symbol;
            symbol->set_hash_bucket_index(bucket_index);
            symbol->set_hash_chain_position(chain_position);
        } else {
            chain_position++;
            symbol_info *current_symbol = hash_buckets[bucket_index];
            while (current_symbol->get_next_symbol() != NULL) {
                current_symbol = current_symbol->get_next_symbol();
                chain_position++;
            }
            current_symbol->set_next_symbol(symbol);
            symbol->set_hash_bucket_index(bucket_index);
            symbol->set_hash_chain_position(chain_position);
        }
    }

    /* Removes a symbol from this scope table */
    bool delete_symbol_from_current_scope(string key) {
        int bucket_index = hash_symbol_name(key);

        symbol_info *current_symbol = hash_buckets[bucket_index];

        if (current_symbol == NULL) {
            return false;
        }

        if (current_symbol->get_symbol_name() == key && current_symbol->get_next_symbol() == NULL) {
            delete current_symbol;
            hash_buckets[bucket_index] = NULL;
            return true;
        }

        symbol_info *previous_symbol = hash_buckets[bucket_index];
        int chain_position = 0;
        while (current_symbol->get_symbol_name() != key && current_symbol->get_next_symbol() != NULL) {
            previous_symbol = current_symbol;
            current_symbol = current_symbol->get_next_symbol();
            chain_position++;
        }

        if (current_symbol->get_symbol_name() == key && current_symbol->get_next_symbol() != NULL) {
            if (chain_position == 0) {
                hash_buckets[bucket_index] = current_symbol->get_next_symbol();
            }
            previous_symbol->set_next_symbol(current_symbol->get_next_symbol());
            current_symbol->set_next_symbol(NULL);

            delete current_symbol;
            return true;
        } else {
            previous_symbol->set_next_symbol(NULL);
            current_symbol->set_next_symbol(NULL);
            delete current_symbol;
            return true;
        }
        return false;
    }

    /* Writes the current scope contents to the symbol-table log */
    void print_current_scope() {
        bool scope_header_printed = false;
        bool bucket_has_symbols = false;

        for (int i = 0; i < bucket_count; i++) {
            symbol_info *current_symbol = hash_buckets[i];
            if(scope_header_printed == false){
                sym_tables << "scope_table # " << scope_id << endl;
                scope_header_printed = true;
            }
            if(current_symbol != NULL){
                sym_tables << " " << i << " --> ";
                bucket_has_symbols = true;
            }

            while (current_symbol != NULL) {
                sym_tables << "| ";
                if(current_symbol->is_struct()){
                    sym_tables << "< " << current_symbol->get_symbol_name() << " , " << "Struct Symbol" << " , " << current_symbol->get_variable_type() << " , " << current_symbol->get_parameter_list_string() << " >";
                }
                else if(current_symbol->is_array()){
                    sym_tables << "< " << current_symbol->get_symbol_name() << " , " << "Array Symbol" << " , " << current_symbol->get_variable_type() << " < Array Size: " << current_symbol->get_size_list() << " > " << " >";
                }
                else if(current_symbol->is_function()){
                    sym_tables << "< " << current_symbol->get_symbol_name() << " , " << "Function Symbol" << " , " << current_symbol->get_variable_type() << " , " << "Parameter List: " << current_symbol->get_parameter_list_string() << " >";
                }
                else if(current_symbol->is_pointer()){
                    sym_tables << "< " << current_symbol->get_symbol_name() << " , " << "Pointer Symbol" << " , " << current_symbol->get_variable_type() << " >";
                }
                else{
                    sym_tables << "< " << current_symbol->get_symbol_name() << " , " << current_symbol->get_variable_type() << " >";
                }
                sym_tables << " |";
                current_symbol = current_symbol->get_next_symbol();
            }
            if(bucket_has_symbols){
                sym_tables << endl;
                bucket_has_symbols = false;
            }
        }
        sym_tables << endl;
    }

    /* Finds a symbol in this scope table */
    symbol_info *lookup(string key) {
        int bucket_index = hash_symbol_name(key);
        symbol_info *current_symbol = hash_buckets[bucket_index];
        if (current_symbol == NULL) {
            return NULL;
        }

        int chain_position = 0;
        while (current_symbol != NULL) {
            if (current_symbol->get_symbol_name() == key) {
                current_symbol->set_hash_chain_position(chain_position);
                return current_symbol;
            }
            chain_position++;
            current_symbol = current_symbol->get_next_symbol();
        }
        return NULL;
    }


    /*
     * Releases the bucket array for this scope table
     *
     * The symbol_info objects are intentionally not deleted here. A scope does
     * not exclusively own its symbols: the scanner hands the table-owned
     * symbol_info pointer back as the token value (see C99-scanner.lex), and
     * parameter lists and struct-member lists alias the same pointers across
     * scopes. Deleting them on scope exit freed objects that were still
     * referenced, causing a use-after-free in later lookups. The translator is
     * a one-shot process, so the symbols are left for process exit
     */
    ~scope_table() {
        delete[] hash_buckets;
        hash_buckets = nullptr;
        logFile << "scope_table " << scope_id << " fully destructed." << std::endl;
    }

};


/*
 * Stack of scope_tables representing the active nesting of lexical scopes.
 * Handles entering and leaving scopes and lookup/insert across the scope chain.
 */
class symbol_table {
private:
    scope_table *current_scope_table;
    int table_size;

public:
    /* Creates a symbol table with one root scope */
    symbol_table(int table_size) {
        this->table_size = table_size;
        current_scope_table = new scope_table(table_size, NULL);
    }

    /* Enters a new child scope */
    void enter_scope() {
        current_scope_table->add_child_scope();
        current_scope_table = new scope_table(table_size, current_scope_table);
        logFile << "New scope_table with id " << current_scope_table->get_scope_id() << " created" << endl;
    }


    /* Leaves the current scope and restores its parent */
    void exit_scope() {
        print_current_scope_table();
        if(!is_empty()){
            if (current_scope_table->get_parent_scope() == NULL) {
                delete current_scope_table;
                current_scope_table = NULL;
            }
            if (current_scope_table != NULL) {
                scope_table *parent_scope = current_scope_table->get_parent_scope();
                delete current_scope_table;
                current_scope_table = parent_scope;
            }
        }
    }

    /* Inserts a symbol into the current scope */
    bool insert(symbol_info *new_symbol) {
        if(current_scope_table == NULL){
            current_scope_table = new scope_table(table_size, NULL);
        }

        if (current_scope_table->lookup(new_symbol->get_symbol_name()) != NULL){
            return false;
        }

        current_scope_table->insert(new_symbol);

        return true;
    }

    /* Removes a symbol from the current scope */
    bool remove(string symbol) {

        symbol_info *current_symbol = current_scope_table->lookup(symbol);
        if (current_symbol != NULL) {
            cout << "Found in scope_table# " << current_scope_table->get_scope_id() << " at position " << current_symbol->get_hash_position() << endl;
            cout << "Deleted Entry " << current_symbol->get_hash_position() << " from current scope_table" << endl;

            current_scope_table->delete_symbol_from_current_scope(symbol);
            return true;
        }

        return false;
    }

    /* Returns whether a symbol exists in any active scope */
    bool contains_in_any_scope(string symbol) {
        scope_table *scope = current_scope_table;
        while (scope) {
            symbol_info *current_symbol = scope->lookup(symbol);
            if (current_symbol != NULL) {
                return true;
            } else scope = scope->get_parent_scope();
        }
        return false;
    }

    /* Returns whether a symbol exists in the current scope */
    bool lookup(string symbol) {
        symbol_info *current_symbol = current_scope_table->lookup(symbol);
        if (current_symbol != NULL) {
            return true;
        } else return false;


    }

    /* Returns symbol metadata from the nearest active scope */
    symbol_info* get_symbol_info(string symbol) {
        scope_table *scope = current_scope_table;
        while (scope) {
            symbol_info *current_symbol = scope->lookup(symbol);
            if (current_symbol != NULL) {
                return current_symbol;
            } else scope = scope->get_parent_scope();
        }
        return NULL;
    }

    /* Writes the current scope table to the symbol-table log */
    void print_current_scope_table() {
        if(!is_empty())
            current_scope_table->print_current_scope();
    }

    /* Returns the identifier of the current scope */
    string current_scope_id() {
        if(!is_empty())
            return current_scope_table->get_scope_id();
        return "No scope table found";
    }

    /* Writes every active scope table to the symbol-table log */
    void print_all_scope_tables() {
        scope_table *scope_cursor = current_scope_table;

        while (scope_cursor) {
            scope_cursor->print_current_scope();
            scope_cursor = scope_cursor->get_parent_scope();

        }
    }

    /* Returns whether there is no current scope table */
    bool is_empty(){
        if(!current_scope_table){
            return true;
        }
        else return false;
    }

    /* Releases all active scope tables */
    ~symbol_table() {
        while (current_scope_table != nullptr) {
            scope_table* scope_to_delete = current_scope_table;
            current_scope_table = current_scope_table->get_parent_scope();
            delete scope_to_delete;
        }

        logFile << "symbol_table fully destructed." << std::endl;
    }
};


#endif /* SYMBOL_TABLE_H */
