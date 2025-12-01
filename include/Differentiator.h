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

struct Differentiator_t {
    Tree_t* expr_tree;
    Tree_t* diff_tree;

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

Tree_t* ExpressionParser( char* buffer );

double EvaluateTree( Tree_t* tree );

Tree_t* DifferentiateExpression( Differentiator_t* diff, char independent_var, int order );

void DifferentiatiorDump( Differentiator_t* diff, enum DumpMode mode, const char* format, ... );
void DifferentiatorDumpLatex( Differentiator_t* diff, int order );
void TreeDumpLatex( Tree_t* tree, const char* filename );

#endif