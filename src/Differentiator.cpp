#include <assert.h>
#include <cstdarg>
#include <cstdio>
#include <string.h>

#include "Differentiator.h"
#include "DebugUtils.h"
#include "Tree.h"
#include "UtilsRW.h"


static Latex_t LatexCtor();
static void LatexDtor( Latex_t* latex );

ON_DEBUG( static Log_t DumpCtor() );
ON_DEBUG( static void DumpDtor( Log_t* logging ) );

Differentiator_t* DifferentiatorCtor( const char* expr_filename ) {
    Differentiator_t* diff = ( Differentiator_t* ) calloc ( 1, sizeof( *diff ) );
    assert( diff && "Memory allocation error for `diff`" );

    diff->expr_info.buffer = ReadToBuffer( expr_filename );
    diff->expr_tree = ExpressionParser( diff->expr_info.buffer );

    diff->latex = LatexCtor();
    ON_DEBUG( diff->logging = DumpCtor(); )

    return diff;
}

void DifferentiatorDtor( Differentiator_t** diff ) {
    my_assert( diff, "Null pointer on `diff`" );
    my_assert( *diff, "An attempt to launch destructor for a null-terminated struct" );

    free( ( *diff )->expr_info.buffer );

    TreeDtor( &( ( *diff )->expr_tree ), NULL );
    TreeDtor( &( ( *diff )->diff_tree ), NULL );

    LatexDtor( &( ( *diff )->latex ) );
    ON_DEBUG( DumpDtor( &( ( *diff )->logging ) ); )

    free( *diff );
    *diff = NULL;
}


#define LATEX_PRINT( format, ... )

static Latex_t LatexCtor() {
    Latex_t latex = {};

    latex.tex_path = strdup( "tex" );

    char buffer[ MAX_LEN_PATH ] = {};
    snprintf( buffer, MAX_LEN_PATH, "%s/main.tex", latex.tex_path );
    latex.tex_file = fopen( buffer, "w" );
    assert( latex.tex_file && "Error opening file" );

    LATEX_PRINT(
        "\\documentclass[14pt,a4paper{article}\n"
        "\\input{tex/style}\n"
        "\\title{Дифференциатор 3000}\n"
        "\\begin{document}\n"
        "\\maketitle\n"
        "\\tableofcontents\n"
        "\\newpage\n"
    );

    fflush( latex.tex_file );

    return latex;
}

static void LatexDtor( Latex_t* latex ) {
    my_assert( latex, "Null pointer on `latex`" );

    LATEX_PRINT( "\\end{document}\n" );

    free( latex->tex_path );
    int fclose_result = fclose( latex->tex_file );
    if ( fclose_result ) {
        PRINT_ERROR( "Fail to close latex file \n" );
    }
}


static void NodeToLatex( const Node_t* node, FILE* file );

void DifferentiatorDumpLatex( Differentiator_t* diff, int order ) {
    my_assert(diff, "Null pointer on `diff`");

    FILE* f = diff->latex.tex_file;
    my_assert(f, "Latex file is not open");

    LATEX_PRINT( "\\section*{Исходное выражение}\n" );
    LATEX_PRINT( "$" );
    NodeToLatex( diff->expr_tree->root, f );
    LATEX_PRINT( "$\n\n");

    LATEX_PRINT( "\\section*{Производная порядка %d}\n", order );

    LATEX_PRINT( "$\\frac{d^{%d}}{d%c^{%d}}(", order, 'x', order  );
    NodeToLatex( diff->expr_tree->root, f );
    LATEX_PRINT( ") = ");

    NodeToLatex( diff->diff_tree->root, f );
    LATEX_PRINT( "$\n\n" );
}


static void NodeToLatex(const Node_t* node, FILE* file) {
    if ( !node ) return;

    switch ( node->value.type ) {
        case NODE_NUMBER:
            LATEX_PRINT( "%g", node->value.data.number);
            break;
        case NODE_VARIABLE:
            LATEX_PRINT( "%c", node->value.data.variable);
            break;
        case NODE_OPERATION: {
            switch (node->value.data.operation) {
                case OP_ADD:
                    LATEX_PRINT( "(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( " + " );
                    NodeToLatex(node->right, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_SUB:
                    LATEX_PRINT( "(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( " - " );
                    NodeToLatex( node->right, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_MUL:
                    LATEX_PRINT( "(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( " \\cdot " );
                    NodeToLatex( node->right, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_DIV:
                    LATEX_PRINT( "\\frac{" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( "}{" );
                    NodeToLatex(node->right, file );
                    LATEX_PRINT( "}" );
                    break;
                case OP_POW:
                    LATEX_PRINT( "{" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( "}^{" );
                    NodeToLatex( node->right, file );
                    LATEX_PRINT( "}" );
                    break;
                case OP_SIN:
                    LATEX_PRINT( "\\sin(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_COS:
                    LATEX_PRINT( "\\cos(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_TAN:
                    LATEX_PRINT( "\\tan(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_LOG:
                    LATEX_PRINT( "\\log(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_SH:
                    LATEX_PRINT( "\\sinh(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_CH:
                    LATEX_PRINT( "\\cosh(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_ARCSIN:
                    LATEX_PRINT( "\\arcsin(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_ARCCOS:
                    LATEX_PRINT( "\\arccos(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_ARCTAN:
                    LATEX_PRINT( "\\arctan(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                case OP_ARCCTAN:
                    LATEX_PRINT( "\\text{arccot}(" );
                    NodeToLatex( node->left, file );
                    LATEX_PRINT( ")" );
                    break;
                default:
                    LATEX_PRINT( "??" );
                    break;
            }
            break;
        }
        case NODE_UNKNOWN:
        default: 
            LATEX_PRINT( "???" ); break;
    }
}

#ifdef _DEBUG
static void DiffNodeDumpLatex( Differentiator_t* diff, Node_t* original, Node_t* derivative, char independent_var, int order ) {
    FILE* file = diff->latex.tex_file;
    LATEX_PRINT( "\\[\n" );
    LATEX_PRINT( "\\frac{d^{%d}}{d %c^{%d}} ", order, independent_var, order);
    NodeToLatex(original, file );
    LATEX_PRINT( " = " );
    NodeToLatex(derivative, file );
    LATEX_PRINT( "\n\\]\n" );
    fflush( file );
}
#endif

#undef LATEX_PRINT

#ifdef _DEBUG
static Log_t DumpCtor() {
    Log_t logging = {};
    logging.log_path = strdup( "dump" );

    char buffer[ MAX_LEN_PATH ] = {};

    snprintf( buffer, MAX_LEN_PATH, "%s/images", logging.log_path );
    logging.img_log_path = strdup( buffer );

    int mkdir_result = MakeDirectory( logging.log_path );
    assert( !mkdir_result && "Error creating directory for dump" );
    mkdir_result = MakeDirectory( logging.img_log_path );
    assert( !mkdir_result && "Error creating directory for dump images" );

    snprintf( buffer, MAX_LEN_PATH, "%s/index.html", logging.log_path );
    logging.log_file = fopen( buffer, "w" );
    assert( logging.log_file && "Error opening file" );

    logging.image_number = 0;

    return logging;
}

static void DumpDtor( Log_t* logging ) {
    my_assert( logging, "Null pointer on `logging" );

    int fclose_result = fclose( logging->log_file );
    if ( fclose_result ) {
        PRINT_ERROR( "Fail to close file with logs \n" );
    }

    free( logging->log_path );
    free( logging->img_log_path );
}


#define PRINT_HTML( fmt, ... ) fprintf( diff->logging.log_file, fmt, ##__VA_ARGS__ );


void DifferentiatiorDump( Differentiator_t* diff, enum DumpMode mode, const char* format, ... ) {
    my_assert( diff, "Null pointer for `diff`" );

    PRINT_HTML( "<h3>DUMP</H3> \n" );

    if ( format ) {
        PRINT_HTML( "<pre>" )
        va_list args = {};
        va_start( args, format );
        vfprintf( diff->logging.log_file, format, args );
        va_end( args );
        PRINT_HTML( "</pre> \n" );
    }

    switch ( mode ) {
        case DUMP_ORIGINAL:
            PRINT_HTML("<pre>Original expression:</pre> \n" );
            NodeGraphicDump(
                diff->expr_tree->root, "%s/image%lu.dot",
                diff->logging.img_log_path, diff->logging.image_number
            );
            PRINT_HTML(
                "<img src=\"images/image%lu.dot.svg\" style=\"width:auto; height:40%%;\">\n",
                diff->logging.image_number++
            );
            break;

        case DUMP_DIFFERENTIATED:
            PRINT_HTML("<pre>Differentiated expression:</pre> \n" );
            NodeGraphicDump(
                diff->diff_tree->root, "%s/image%lu.dot",
                diff->logging.img_log_path, diff->logging.image_number
            );
            PRINT_HTML(
                "<img src=\"images/image%lu.dot.svg\" style=\"width:auto; height:40%%;\">\n",
                diff->logging.image_number++
            );
            break;

        default:
            PRINT_ERROR( "I can't be here, but I'm here \n" );
            break;
    }

    fflush( diff->logging.log_file );
}

#undef PRINT_HTML
#endif


static Node_t* DifferentiateNode( Node_t* node, char independent_var, Differentiator_t* diff, int order );

static TreeData_t MakeNumber( double number );
static TreeData_t MakeOperation( OperationType operation );
static TreeData_t MakeVariable( char variable );

Tree_t* DifferentiateExpression(Differentiator_t* diff, char independent_var, int order) {
    my_assert( diff, "Null pointer on diff" );

    diff->diff_tree = TreeCtor();
    Node_t* node = diff->expr_tree->root;

    Node_t* deriv = NULL;
    for ( int idx = 1; idx <= order; idx++ ) {
        deriv = DifferentiateNode( node, independent_var, diff, idx );
        node = deriv;
    }

    diff->diff_tree->root = deriv;
    return diff->diff_tree;
}


#define NUM_( n ) NodeCreate( MakeNumber(n), NULL )
#define VAR_( v ) NodeCreate( MakeVariable(v), NULL )

#define ADD_( L, R ) MakeNode( OP_ADD, L, R )
#define SUB_( L, R ) MakeNode( OP_SUB, L, R )
#define MUL_( L, R ) MakeNode( OP_MUL, L, R )
#define DIV_( L, R ) MakeNode( OP_DIV, L, R )
#define POW_( L, R ) MakeNode( OP_POW, L, R )

#define cL NodeCopy( node->left )
#define cR NodeCopy( node->right )

#define dL DifferentiateNode( node->left,  independent_var, diff, order )
#define dR DifferentiateNode( node->right, independent_var, diff, order )

static Node_t* MakeNode( OperationType op, Node_t* L, Node_t* R ) {
    Node_t* n = NodeCreate( MakeOperation( op ), NULL );
    n->left = L;
    if (L) L->parent = n;
    n->right = R;
    if (R) R->parent = n;
    return n;
}

static Node_t* DifferentiateNode( Node_t* node, char independent_var, Differentiator_t* diff, int order ) {
    if (!node)
        return NULL;

    Node_t* result = NULL;

    switch ( node->value.type ) {
        case NODE_NUMBER:
            result = NUM_(0);
            break;

        case NODE_VARIABLE:
            result = ( node->value.data.variable == independent_var ) ? NUM_(1) : NUM_(0);
            break;

        case NODE_OPERATION: {
            switch ( node->value.data.operation ) {
                case OP_ADD: result = ADD_( dL, dR ); break;
                case OP_SUB: result = SUB_( dL, dR ); break;
                case OP_MUL: result = ADD_(
                                          MUL_( dL, cR ),
                                          MUL_( cL, dR )
                                      ); break;
                case OP_DIV: result = DIV_(
                                          SUB_( MUL_( dL, cR ), MUL_( cL, dR ) ),
                                          MUL_( cR, cR )
                                      ); break;
                case OP_POW: {
                    Node_t* base = node->left;
                    Node_t* exp  = node->right;
                    bool base_is_const = ( base->value.type == NODE_NUMBER );
                    bool exp_is_const  = ( exp->value.type  == NODE_NUMBER );

                    if ( exp_is_const ) {
                        double c = exp->value.data.number;
                        result = MUL_(
                                     MUL_( NUM_( c ), POW_( cL, NUM_( c - 1 ) ) ),
                                     dL
                                 ); break;
                    }
                    if ( base_is_const ) {
                        double a = base->value.data.number;
                        result = MUL_(
                                   MUL_( POW_( NUM_(a), cR ), MakeNode( OP_LOG, NUM_(a), NULL ) ),
                                   dR
                               ); break;
                    }
                    result = MUL_(
                                 POW_( cL, cR ),
                                 ADD_(
                                     MUL_( dR, MakeNode( OP_LOG, cL, NULL ) ),
                                     MUL_( cR, DIV_( dL, cL ) )
                                 )
                             ); break;
                }

                case OP_LOG:
                    if ( !node->right ) {
                        result = DIV_( dL, cL ); 
                        break;
                    }
                    result = DIV_(
                                 SUB_(
                                     MUL_( dR, MakeNode( OP_LOG, cL, NULL ) ),
                                     MUL_( dL, MakeNode( OP_LOG, cR, NULL ) )
                                 ),
                                 MUL_( cR, MakeNode( OP_LOG, cL, NULL ) )
                             ); break;

                case OP_SIN: result = MUL_( MakeNode( OP_COS, cL, NULL ), dL ); break;
                case OP_COS: result = MUL_( NUM_(-1), MUL_( MakeNode( OP_SIN, cL, NULL ), dL ) ); break;
                case OP_TAN: result = DIV_( dL, POW_( MakeNode( OP_COS, cL, NULL ), NUM_(2) ) ); break;

                case OP_CTAN: result = MUL_( NUM_(-1), DIV_( dL, POW_( MakeNode( OP_SIN, cL, NULL ), NUM_(2) ) ) ); break;
                case OP_SH:   result = MUL_( MakeNode( OP_CH, cL, NULL ), dL ); break;
                case OP_CH:   result = MUL_( MakeNode( OP_SH, cL, NULL ), dL ); break;

                case OP_ARCSIN: result = DIV_( dL, POW_( SUB_( NUM_(1), POW_( cL, NUM_(2) ) ), NUM_(-1) ) ); break;
                case OP_ARCCOS: result = MUL_( NUM_(-1), DIV_( dL, POW_( SUB_( NUM_(1), POW_( cL, NUM_(2) ) ), NUM_(-1) ) ) ); break;
                case OP_ARCTAN: result = DIV_( dL, ADD_( NUM_(1), POW_( cL, NUM_(2) ) ) ); break;
                case OP_ARCCTAN: result = MUL_( NUM_(-1), DIV_( dL, ADD_( NUM_(1), POW_( cL, NUM_(2) ) ) ) ); break;

                default:
                    PRINT_ERROR( "Unknown operation in differentiation!\n" );
                    result = NULL;
                    break;
            }

             ON_DEBUG( if ( diff ) DiffNodeDumpLatex( diff, node, result, independent_var, order ); )
             break;
        }

        case NODE_UNKNOWN:
        default:
            PRINT_ERROR( "Error while differentiating the expression!\n" );
            result = NULL;
            break;
    }

    return result;
}

static TreeData_t MakeNumber( double number ) {
    TreeData_t value = {};
    value.type = NODE_NUMBER;
    value.data.number = number;

    return value;
}

static TreeData_t MakeOperation( OperationType operation ) {
    TreeData_t value = {};

    value.type = NODE_OPERATION;
    value.data.operation = operation;

    return value;
}

static TreeData_t MakeVariable( char variable ) {
    TreeData_t value = {};

    value.type = NODE_VARIABLE;
    value.data.variable = variable;

    return value;
}


