#ifndef DIFFERENTIATOR_H
#define DIFFERENTIATOR_H

#include "Tree.h"


struct Latex_t {
    FILE* tex_file;
    char* tex_path;
};

#ifdef _DEBUG
struct Log_t {
    FILE* log_file;
    char* log_path;
    char* img_log_path;
    size_t image_number;
};

enum DumpMode {
    DUMP_ORIGINAL       = 0,
    DUMP_DIFFERENTIATED = 1
};
#endif

struct Variable_t {
    char   name;
    double value;
};

struct Differentiator_t {
    Tree_t* expr_tree;
    Tree_t* diff_tree;

    struct VarTable_t {
        Variable_t* data;
        size_t number_of_variables;
    } var_table;

    struct Expression_t {
        char* buffer;
        char* current_position;
    } expr_info;

    struct Latex_t latex;

#ifdef _DEBUG
    struct Log_t logging;
#endif
};

Differentiator_t* DifferentiatorCtor( const char* expr_filename );
void DifferentiatorDtor( Differentiator_t** diff );

Tree_t* ExpressionParser( Differentiator_t* diff );

double EvaluateTree( Tree_t* tree );

Tree_t* DifferentiateExpression( Differentiator_t* diff, char independent_var, int order );

void DifferentiatiorDump( Differentiator_t* diff, enum DumpMode mode, const char* format, ... );
void DifferentiatorDumpLatex( Differentiator_t* diff, int order );
void TreeDumpLatex( const Tree_t* tree, FILE* latex_file );

void DifferentiatorDumpLatex( Differentiator_t* diff, int order );
void DifferentiatorAddTaylorSeries( Differentiator_t* diff, char var, double point, int order );
void DifferentiatorAddEvaluation( Differentiator_t* diff, double point );


#endif