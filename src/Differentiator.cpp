#include <assert.h>
#include <cstdarg>
#include <cstdio>
#include <string.h>

#include "Differentiator.h"
#include "DebugUtils.h"
#include "UtilsRW.h"


ON_DEBUG( static Log_t DumpCtor() );
ON_DEBUG( static void DumpDtor( Log_t* logging ) );

Differentiator_t* DifferentiatorCtor( const char* expr_filename ) {
    Differentiator_t* diff = ( Differentiator_t* ) calloc ( 1, sizeof( *diff ) );
    assert( diff && "Memory allocation error for `diff`" );

    diff->expr_info.buffer = ReadToBuffer( expr_filename );
    diff->expr_tree = TreeReadFromBuffer( diff->expr_info.buffer );

    ON_DEBUG( diff->logging = DumpCtor(); )

    return diff;
}

void DifferentiatorDtor( Differentiator_t** diff ) {
    my_assert( diff, "Null pointer on `diff`" );
    my_assert( *diff, "An attempt to launch destructor for a null-terminated struct" );

    free( ( *diff )->expr_info.buffer );

    TreeDtor( &( ( *diff )->expr_tree ), NULL );
    TreeDtor( &( ( *diff )->diff_tree ), NULL );

    DumpDtor( &( ( *diff )->logging ) );

    free( *diff );
    *diff = NULL;
}

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
}

#undef PRINT_HTML
#endif


static Node_t* DifferentiateNode( Node_t* node, char independent_variable );

static TreeData_t MakeNumber( double number );
static TreeData_t MakeOperation( OperationType operation );
static TreeData_t MakeVariable( char variable );

Tree_t* DifferentiateExpression( Differentiator_t* diff, const char independent_variable, int order ) {
    my_assert( diff, "Null pointer on `diff`" );

    diff->diff_tree = TreeCtor();

    diff->diff_tree->root = DifferentiateNode( diff->expr_tree->root, independent_variable );

    if ( diff->diff_tree->root ) {
        return diff->diff_tree;
    }
    else {
        PRINT_ERROR( "Error differentiating the expression!!! \n" );
        return NULL;
    }
}


#define NUM_( n ) NodeCreate(MakeNumber(n), NULL)
#define VAR_( v ) NodeCreate(MakeVariable(v), NULL)

#define ADD_( L, R ) MakeNode( OP_ADD, L, R )
#define SUB_( L, R ) MakeNode( OP_SUB, L, R )
#define MUL_( L, R ) MakeNode( OP_MUL, L, R )
#define DIV_( L, R ) MakeNode( OP_DIV, L, R )
#define POW_( L, R ) MakeNode( OP_POW, L, R )

#define cL NodeCopy( node->left )
#define cR NodeCopy( node->right )

#define dL DifferentiateNode( node->left,  independent_variable )
#define dR DifferentiateNode( node->right, independent_variable )

static Node_t* MakeNode( OperationType op, Node_t* L, Node_t* R ) {
    Node_t* n = NodeCreate( MakeOperation( op ), NULL );
    n->left = L;
    if (L) L->parent = n;
    n->right = R;
    if (R) R->parent = n;
    return n;
}

static Node_t* DifferentiateNode( Node_t* node, char independent_variable ) {
    if (!node)
        return NULL;

    switch (node->value.type) {
        case NODE_NUMBER:
            return NUM_(0);

        case NODE_VARIABLE:
            return (node->value.data.variable == independent_variable) ? NUM_(1) : NUM_(0);

        case NODE_OPERATION: {
            switch (node->value.data.operation) {
                case OP_ADD: return ADD_( dL, dR );
                case OP_SUB: return SUB_( dL, dR );
                case OP_MUL: return ADD_(
                                        MUL_( dL, cR ),
                                        MUL_( cL, dR )
                                    );
                case OP_DIV: return DIV_(
                                        SUB_( MUL_( dL, cR ), MUL_( cL, dR ) ),
                                        MUL_(cR, cR)
                                    );
                case OP_POW: {
                    Node_t* base = node->left;
                    Node_t* exp  = node->right;
                    bool base_is_const = ( base->value.type == NODE_NUMBER );
                    bool exp_is_const  = ( exp->value.type  == NODE_NUMBER );

                    if ( exp_is_const ) {
                        double c = exp->value.data.number;
                        return MUL_(
                                   MUL_( NUM_( c ),
                                         POW_( cL, NUM_( c - 1 ) ) ),
                                   dL
                               );
                    }
                    if ( base_is_const ) {
                        double a = base->value.data.number;
                        return MUL_(
                                   MUL_( POW_( NUM_(a), cR ),
                                         MakeNode( OP_LOG, NUM_(a), NULL ) ),
                                   dR
                               );
                    }
                    return MUL_(
                               POW_( cL, cR ),
                               ADD_( MUL_( dR, MakeNode( OP_LOG, cL, NULL ) ),
                                     MUL_( cR, DIV_( dL, cL ) ) ) );
                }

                case OP_LOG:
                    if (!node->right) return DIV_(dL, cL);
                    return DIV_( SUB_( MUL_( dR, MakeNode(OP_LOG, cL, NULL)),
                                     MUL_( dL, MakeNode(OP_LOG, cR, NULL))),
                                MUL_(cR, MakeNode(OP_LOG, cL, NULL)));

                case OP_SIN: return MUL_(MakeNode(OP_COS, cL, NULL), dL);
                case OP_COS: return MUL_(NUM_(-1), MUL_(MakeNode(OP_SIN, cL, NULL), dL));
                case OP_TAN: return DIV_(dL, POW_(MakeNode(OP_COS, cL, NULL), NUM_(2)));

                case OP_CTAN: return MUL_(NUM_(-1), DIV_(dL, POW_(MakeNode(OP_SIN, cL, NULL), NUM_(2))));
                case OP_SH:   return MUL_(MakeNode(OP_CH, cL, NULL), dL);
                case OP_CH:   return MUL_(MakeNode(OP_SH, cL, NULL), dL);

                case OP_ARCSIN: return DIV_(dL, POW_(SUB_(NUM_(1), POW_(cL, NUM_(2))), NUM_(-1))); // 1 / sqrt(1-x^2)
                case OP_ARCCOS: return MUL_(NUM_(-1), DIV_(dL, POW_(SUB_(NUM_(1), POW_(cL, NUM_(2))), NUM_(-1))));
                case OP_ARCTAN: return DIV_(dL, ADD_(NUM_(1), POW_(cL, NUM_(2))));
                case OP_ARCCTAN: return MUL_(NUM_(-1), DIV_(dL, ADD_(NUM_(1), POW_(cL, NUM_(2)))));

                default:
                    PRINT_ERROR( "Unknown operation in differentiation!\n" );
                    return NULL;
            }
        }

        case NODE_UNKNOWN:
        default:
            PRINT_ERROR( "Error while differentiating the expression!\n" );
            return NULL;
    }
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


#define LATEX_PRINT( format, ... )

static void NodeToLatex( const Node_t* node, FILE* file );

void TreeDumpLatex( Tree_t* tree, const char* filename ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( filename, "Null pointer on `filename`" );

    FILE* file = fopen( filename, "w" );
    if ( !file ) {
        PRINT_ERROR( "Error opening file `%s` \n", filename );
        return;
    }

    LATEX_PRINT(
        "\\documentclass{article}\n"
        "\\usepackage{amsmath}\n"
        "\\begin{document}\n"
        "\\[\n"
    );

    NodeToLatex( tree->root, file );

    LATEX_PRINT(
        "\\]\n"
        "\\end{document}\n"
    );

    if ( fclose( file ) ) {
        PRINT_ERROR( "Error closing file `%s` \n", filename );
    }
}

static void NodeToLatex( const Node_t* node, FILE* file ) {
    if ( !node )
        return;

    switch ( node->value.type ) {
        case NODE_NUMBER:
            LATEX_PRINT( "%g", node->value.data.number );
            return;

        case NODE_VARIABLE:
            LATEX_PRINT( "%c", node->value.data.variable );
            return;

        case NODE_OPERATION:
            switch (node->value.data.operation)
            {
                case OP_ADD:
                    LATEX_PRINT( "(" );
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( " + " );
                    NodeToLatex(node->right, file);
                    LATEX_PRINT( ")" );
                    return;

                case OP_SUB:
                    LATEX_PRINT( "(");
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( " - ");
                    NodeToLatex(node->right, file);
                    LATEX_PRINT( ")");
                    return;

                case OP_MUL:
                    LATEX_PRINT( "(");
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( "\\cdot ");
                    NodeToLatex(node->right, file);
                    LATEX_PRINT( ")");
                    return;

                case OP_DIV:
                    LATEX_PRINT( "\\frac{");
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( "}{");
                    NodeToLatex(node->right, file);
                    LATEX_PRINT( "}");
                    return;

                case OP_POW:
                    LATEX_PRINT( "{");
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( "}^{");
                    NodeToLatex(node->right, file);
                    LATEX_PRINT( "}");
                    return;

                case OP_SIN:
                    LATEX_PRINT( "\\sin(");
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( ")");
                    return;

                case OP_COS:
                    LATEX_PRINT( "\\cos(");
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( ")");
                    return;

                case OP_TAN:
                    LATEX_PRINT( "\\tan(");
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( ")");
                    return;

                case OP_LOG:
                    LATEX_PRINT( "\\log(");
                    NodeToLatex(node->left, file);
                    LATEX_PRINT( ")");
                    return;

                default:
                    LATEX_PRINT( "??");
                    return;
            }

        case NODE_UNKNOWN:
        default:
            LATEX_PRINT( "???" );
    }
}
