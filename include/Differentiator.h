#ifndef DIFFERENTIATOR_H
#define DIFFERENTIATOR_H

#include "Tree.h"

struct Latex_t {
    FILE *tex_file;
    char *tex_path;
};

#ifdef _DEBUG
struct Log_t {
    FILE *log_file;
    char *log_path;
    char *img_log_path;
    size_t image_number;
};

enum DumpMode { DUMP_ORIGINAL = 0, DUMP_DIFFERENTIATED = 1, DUMP_TAYLOR = 2 };
#endif

struct Variable_t {
    char name;
    double value;
};

struct VarTable_t {
    Variable_t *data;
    size_t number_of_variables;
    size_t capacity;
};

struct Differentiator_t {
    Tree_t *expr_tree;
    Tree_t *diff_tree;
    Tree_t *taylor_tree;

    struct VarTable_t var_table;

    struct Expression_t {
        char *buffer;
        char *current_position;
    } expr_info;

    struct Latex_t latex;

#ifdef _DEBUG
    struct Log_t logging;
#endif
};

TreeData_t MakeNumber( double number );
TreeData_t MakeOperation( OperationType operation );
TreeData_t MakeVariable( char variable );
Node_t *MakeNode( OperationType op, Node_t *L, Node_t *R );

Differentiator_t *DifferentiatorCtor( const char *expr_filename );
void DifferentiatorDtor( Differentiator_t **diff );

// EBNF
Tree_t *ExpressionParser( char *buffer );

// Variable Table
bool VarTableGet( VarTable_t *table, char name, double *value );
void VarTableSet( VarTable_t *table, char name, double value );
void VarTableAskUser( VarTable_t *table );

// Tree Optimization
bool OptimizeTree( Tree_t *tree, Differentiator_t *diff, char independent_var );
bool OptimizeConstants( Tree_t *tree, Differentiator_t *diff, char independent_var );
bool SimplifyTree( Tree_t *tree );

// Evaluate expression
double EvaluateTree( Tree_t *tree, Differentiator_t *diff );

// Differentiate expression
Tree_t *DifferentiateExpression( Differentiator_t *diff, char independent_var, int order );

// Taylor decomposition
Tree_t *DifferentiatorBuildTaylorTree( Differentiator_t *diff, char var, double point, int order );

// Graphic DUMP
void DifferentiatiorDump( Differentiator_t *diff, enum DumpMode mode, const char *format, ... );

// Latex
Latex_t LatexCtor();
void LatexDtor( Latex_t *latex );

const char *GetJokeLine( OperationType op );

void NodeToLatex( const Node_t *node, FILE *latex_file, int parent_priority = 0 );
void TreeDumpLatex( const Tree_t *tree, FILE *latex_file );
void DifferentiatorAddOrigExpression( Differentiator_t *diff, int order );
void DifferentiatorAddEvaluation( Differentiator_t *diff, char name );
void DifferentiatorAddTaylorSeries( Differentiator_t *diff, char var, int order );

// GNU PLOT
void DifferentiatorPlotFunctionAndTaylor( Differentiator_t *diff, char var, double x_min, double x_max,
                                          int n_points, const char *output_image );

int CompareDoubleToDouble( double a, double b, double eps = 1e-10 );

#endif