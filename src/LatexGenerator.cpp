#include <assert.h>
#include <string.h>

#include "DebugUtils.h"
#include "Differentiator.h"
#include "UtilsRW.h"

#define LATEX_PRINT( format, ... ) fprintf( latex_file, format, ##__VA_ARGS__ );

#define OPERATIONS_LATEX( str, name, value, is_func, n_args, latex_fmt, ... ) latex_fmt,

static const char *latex_format[] = { INIT_OPERATIONS( OPERATIONS_LATEX ) };

#undef OPERATIONS_LATEX

static void LatexInsertChildren( const Node_t *node, FILE *latex_file, int parent_priority );

static void LatexHeader( FILE *latex_file );

Latex_t LatexCtor() {
    Latex_t latex = {};
    latex.tex_path = strdup( "tex" );
    MakeDirectory( latex.tex_path );
    char buffer[MAX_LEN_PATH] = {};
    snprintf( buffer, MAX_LEN_PATH, "%s/main.tex", latex.tex_path );
    latex.tex_file = fopen( buffer, "w" );
    assert( latex.tex_file && "Error opening file" );

    LatexHeader( latex.tex_file );

    return latex;
}

static void LatexHeader( FILE *latex_file ) {
    LATEX_PRINT( "\\documentclass[14pt,a4paper]{article}\n" );
    LATEX_PRINT( "\\usepackage[utf8]{inputenc}\n" );
    LATEX_PRINT( "\\usepackage[T2A]{fontenc}\n" );
    LATEX_PRINT( "\\usepackage[russian]{babel}\n" );
    LATEX_PRINT( "\\usepackage[left=15mm,right=15mm,top=20mm,bottom=20mm]{geometry}\n" );
    LATEX_PRINT( "\\usepackage{amssymb,amsmath}\n" );
    LATEX_PRINT( "\\usepackage{graphicx}\n" );
    LATEX_PRINT( "\\graphicspath{}\n" );
    LATEX_PRINT( "\\usepackage{tikz}\n" );
    LATEX_PRINT( "\\usetikzlibrary{graphs, quotes}\n" );
    LATEX_PRINT( "\\usepackage[unicode=true,colorlinks=true,allcolors=blue]{hyperref}\n" );
    LATEX_PRINT( "\\usepackage{autobreak}\n" );
    LATEX_PRINT( "\\allowdisplaybreaks\n" );
    LATEX_PRINT( "\\usepackage{siunitx}\n" );
    LATEX_PRINT( "\\sisetup{round-precision=3,scientific-notation=true}\n" );
    LATEX_PRINT( "\\title{Дифференциатор 3000}\n" );
    LATEX_PRINT( "\\author{Владимир Пышкин}\n" );
    LATEX_PRINT( "\\begin{document}\n" );
    LATEX_PRINT( "\\maketitle\n" );
    LATEX_PRINT( "\\tableofcontents\n" );
    LATEX_PRINT( "\\newpage\n" );
}

void LatexDtor( Latex_t *latex ) {
    my_assert( latex, "Null pointer on `latex`" );

    FILE *latex_file = latex->tex_file;

    LATEX_PRINT( "\\subsection{\\textbf{Графики функции, касательной в точке, многочлена Тейлора}}" )
    LATEX_PRINT( "\\begin{figure}[ht]\n" );
    LATEX_PRINT( "\\centering\n" );
    LATEX_PRINT( "\\includegraphics[width=0.8\\textwidth]{plot.png}\n" );
    LATEX_PRINT( "\\caption{Графики}\n" );
    LATEX_PRINT( "\\label{fig:my_image}\n" );
    LATEX_PRINT( "\\end{figure}\n\n" );

    fprintf( latex->tex_file, "\\end{document}\n" );
    free( latex->tex_path );

    int fclose_result = fclose( latex->tex_file );
    if ( fclose_result ) {
        PRINT_ERROR( "Fail to close latex file\n" );
        return;
    }

    system( "pdflatex -synctex=1 -interaction=nonstopmode -output-directory=tex "
            "tex/main.tex" );
}

void TreeDumpLatex( const Tree_t *tree, FILE *latex_file ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( latex_file, "Null pointer on `latex_file`" );

    NodeToLatex( tree->root, latex_file, 0 );
}

static int GetOperationPriority( OperationType op ) {
    switch ( op ) {
        case OP_ADD:
            return 0;
        case OP_SUB:
            return 1;
        case OP_MUL:
        case OP_DIV:
            return 2;
        case OP_POW:
            return 3;
        default:
            return 4;
    }
}

void NodeToLatex( const Node_t *node, FILE *latex_file, int parent_priority ) {
    my_assert( node, "Null pointer on `node`" );

    switch ( node->value.type ) {
        case NODE_NUMBER: {
            double num = node->value.data.number;
            bool is_negative = ( CompareDoubleToDouble( num, 0 ) < 0 );

            if ( is_negative && parent_priority > 0 ) {
                LATEX_PRINT( "(\\num{%g})", num );
            } else {
                if ( CompareDoubleToDouble( num, 1e-8 ) == 0 ) {
                    LATEX_PRINT( "0" );
                } else {
                    LATEX_PRINT( "\\num{%g}", num );
                }
            }
            break;
        }
        case NODE_VARIABLE:
            LATEX_PRINT( " %c ", node->value.data.variable );
            break;
        case NODE_OPERATION:
            LatexInsertChildren( node, latex_file, parent_priority );
            break;
        case NODE_UNKNOWN:
        default:
            LATEX_PRINT( "???" );
            break;
    }
}

static void LatexInsertChildren( const Node_t *node, FILE *latex_file, int parent_priority ) {
    OperationType op = (OperationType)node->value.data.operation;
    const char *format = latex_format[op];
    int curr_priority = GetOperationPriority( op );

    bool need_brackets = ( curr_priority < parent_priority );
    if ( need_brackets )
        LATEX_PRINT( "(" );

    int child_id = 0;
    for ( size_t idx = 0; format[idx]; idx++ ) {
        if ( format[idx] == '%' && format[idx + 1] == 'e' ) {
            const Node_t *child = ( child_id == 0 ? node->left : node->right );
            if ( child )
                NodeToLatex( child, latex_file, curr_priority );
            else
                LATEX_PRINT( "<?>" );
            child_id++;
            idx++;
        } else {
            fputc( format[idx], latex_file );
        }
    }

    if ( need_brackets )
        LATEX_PRINT( ")" );
}

static void LatexFunction( Differentiator_t *diff, FILE *latex_file );

void DifferentiatorAddOrigExpression( Differentiator_t *diff, int order ) {
    my_assert( diff, "Null pointer on `diff`" );

    FILE *latex_file = diff->latex.tex_file;

    LATEX_PRINT( "\\section{Исследование функции}\n" );

    LatexFunction( diff, latex_file );

    for ( int i = 1; i <= order; i++ ) {
        LATEX_PRINT( "\\subsection{\\textbf{Производная порядка %d}}\n\n", i );

        DifferentiateExpression( diff, 'x', i );
        ON_DEBUG( DifferentiatiorDump( diff, DUMP_DIFFERENTIATED, "Differentiative (%d)", i ); );

        LATEX_PRINT( "\\textbf{Итог:}\n\n" );

        LATEX_PRINT( "\\begin{align*}\n" );
        LATEX_PRINT( "\\begin{autobreak}\n" );
        LATEX_PRINT( "\\MoveEqLeft\n" );

        LATEX_PRINT( "f^{(%d)}(x) = ", i );
        NodeToLatex( diff->diff_tree->root, latex_file, 0 );

        LATEX_PRINT( "\n" );
        LATEX_PRINT( "\\end{autobreak}\n" );
        LATEX_PRINT( "\\end{align*}\n" );
    }
}

static void LatexFunction( Differentiator_t *diff, FILE *latex_file ) {
    my_assert( diff, "Null pointer on `diff`" );

    LATEX_PRINT( "\\subsection{\\textbf{Функция}}\n" );

    LATEX_PRINT( "\\begin{align*}\n" );
    LATEX_PRINT( "\\begin{autobreak}\n" );
    LATEX_PRINT( "\\MoveEqLeft\n" );

    LATEX_PRINT( "f(x) = " );
    TreeDumpLatex( diff->expr_tree, latex_file );

    LATEX_PRINT( "\\end{autobreak}\n" );
    LATEX_PRINT( "\\end{align*}\n" );
}

void DifferentiatorAddEvaluation( Differentiator_t *diff, char name ) {
    my_assert( diff, "Null pointer on diff" );

    double point = 0;

    VarTableSet( &( diff->var_table ), 'x', diff->x_0 );

    if ( !VarTableGet( &diff->var_table, name, &point ) ) {
        VarTableAskUser( &diff->var_table );
    }

    double val = EvaluateTree( diff->expr_tree, diff );

    FILE *latex_file = diff->latex.tex_file;
    LATEX_PRINT( "\\subsection{Вычисление значения функции в точке}\n" );
    LATEX_PRINT( "Для $%c = \\num{%g}$ получаем $f(%c) = \\num{%g}$\n\n", name, point, name, val );
}

#define PRINT_O                                                                                              \
    LATEX_PRINT( " + o(" );                                                                                  \
    if ( order > 1 ) {                                                                                       \
        LATEX_PRINT( "(x-\\num{%g})^%d", point, order );                                                     \
    } else {                                                                                                 \
        LATEX_PRINT( "x-\\num{%g}", point );                                                                 \
    }                                                                                                        \
    LATEX_PRINT( ")\n" );

void DifferentiatorAddTaylorSeries( Differentiator_t *diff, char var, int order ) {
    my_assert( diff, "Null pointer on `diff`" );

    double point = 0;
    if ( !VarTableGet( &diff->var_table, var, &point ) ) {
        VarTableAskUser( &diff->var_table );
    }

    diff->taylor_tree = DifferentiatorBuildTaylorTree( diff, var, point, order );

    FILE *latex_file = diff->latex.tex_file;

    OptimizeTree( diff->taylor_tree, diff, var );

    LATEX_PRINT( "\\subsection{Ряд Тейлора порядка %d в точке \\num{%g}}\n", order, point );

    LATEX_PRINT( "\\begin{align*}\n" );
    LATEX_PRINT( "\\begin{autobreak}\n" );
    LATEX_PRINT( "\\MoveEqLeft\n" );

    LATEX_PRINT( "T_{%d}(%c) = ", order, var );
    TreeDumpLatex( diff->taylor_tree, diff->latex.tex_file );
    PRINT_O;

    LATEX_PRINT( "\\end{autobreak}\n" );
    LATEX_PRINT( "\\end{align*}\n" );

    ON_DEBUG( DifferentiatiorDump( diff, DUMP_TAYLOR, "After optimization" ); );
}

#undef PRINT_O


static const char *joke_lines[] = {
#define X( str, op_enum, prio, is_func, nargs, latex, joke ) [op_enum] = joke,
    INIT_OPERATIONS( X )
#undef X
};

const char *GetJokeLine( OperationType op ) {
    if ( op < 0 ) {
        return "Операция неизвестна, но всё равно весело!";
    }
    const char *joke = joke_lines[op];
    return joke ? joke : "Операция без шутки — как код без комментариев!";
}
