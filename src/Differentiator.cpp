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

static void VarTableCtor( VarTable_t* table, size_t initial_capacity );
static void VarTableDtor( VarTable_t* table );

static void AddVarsToTableFromNode( Node_t* node, VarTable_t* table );
static void VarTableAskUser( VarTable_t* table );

ON_DEBUG( static Log_t DumpCtor() );
ON_DEBUG( static void DumpDtor( Log_t* logging ) );

Differentiator_t* DifferentiatorCtor( const char* expr_filename ) {
    Differentiator_t* diff = ( Differentiator_t* ) calloc ( 1, sizeof( *diff ) );
    assert( diff && "Memory allocation error for `diff`" );

    diff->expr_info.buffer = ReadToBuffer( expr_filename );
    diff->expr_tree = ExpressionParser( diff->expr_info.buffer );

    VarTableCtor( &diff->var_table, 5 );
    if ( diff->expr_tree && diff->expr_tree->root ) {
        AddVarsToTableFromNode( diff->expr_tree->root, &diff->var_table );
    }
    VarTableAskUser( &diff->var_table );

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

    VarTableDtor( &( *diff )->var_table );

    LatexDtor( &( ( *diff )->latex ) );
    ON_DEBUG( DumpDtor( &( ( *diff )->logging ) ); )

    free( *diff );
    *diff = NULL;
}


#define LATEX_PRINT( format, ... ) fprintf( latex.tex_file, format, ##__VA_ARGS__ );

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

    fprintf( latex->tex_file, "\\end{document}\n" );

    free( latex->tex_path );
    int fclose_result = fclose( latex->tex_file );
    if ( fclose_result ) {
        PRINT_ERROR( "Fail to close latex file \n" );
    }
}

#undef LATEX_PRINT


static void VarTableCtor( VarTable_t* table, size_t initial_capacity ) {
    my_assert( table, "Null pointer on `table`" );

    table->data = (Variable_t*) calloc( initial_capacity, sizeof( Variable_t ) );
    table->number_of_variables = 0;
    table->capacity = initial_capacity;
}

static void VarTableDtor( VarTable_t* table ) {
    my_assert( table, "Null pointer on `tree`" );

    free( table->data );
    table->data = NULL;
    table->number_of_variables = 0;
    table->capacity = 0;
}

static void AddVarsToTableFromNode( Node_t* node, VarTable_t* table ) {
    if ( !node ) return;

    if ( node->value.type == NODE_VARIABLE ) {
        VarTableSet( table, node->value.data.variable, 0.0 );
    }

    AddVarsToTableFromNode( node->left, table );
    AddVarsToTableFromNode( node->right, table );
}
void VarTableSet( VarTable_t* table, char name, double value ) {
    my_assert( table, "Null pointer on `table`" );

    for ( size_t idx = 0; idx < table->number_of_variables; idx++ ) {
        if ( table->data[ idx ].name == name ) {
            table->data[ idx ].value = value;
            return;
        }
    }

    if ( table->number_of_variables >= table->capacity ) {
        size_t new_capacity = table->capacity * 2;
        if ( new_capacity == 0 ) new_capacity = 4;
        table->data = (Variable_t*) realloc( table->data, new_capacity * sizeof( Variable_t ) );
        table->capacity = new_capacity;
    }

    table->data[ table->number_of_variables ].name = name;
    table->data[ table->number_of_variables ].value = value;
    table->number_of_variables++;
}

bool VarTableGet( VarTable_t* table, char name, double* value ) {
    if ( !table || !value ) return false;

    for ( size_t idx = 0; idx < table->number_of_variables; idx++ ) {
        if ( table->data[ idx ].name == name ) {
            *value = table->data[ idx ].value;
            return true;
        }
    }

    return false;
}

static void VarTableAskUser( VarTable_t* table ) {
    my_assert( table, "Null pointer on `table`" );

    for ( size_t idx = 0; idx < table->number_of_variables; idx++ ) {
        printf( "Enter value for variable %c: ", table->data[idx].name );
        if ( scanf( "%lf", &table->data[ idx ].value ) != 1 ) {
            printf( "Invalid input. Using 0.0 for %c\n", table->data[ idx  ].name );
            table->data[ idx ].value = 0.0;

            int c;
            while ( ( c = getchar() ) != '\n' && c != EOF ) {}
        }
    }
}


#define LATEX_PRINT( format, ... ) fprintf( latex_file, format, ##__VA_ARGS__ );

static void NodeToLatex( const Node_t* node, FILE* file );
static void LatexInsertChildren( const Node_t* node, FILE* latex_file, const char* format_string );

#define OPERATIONS_LATEX( str, name, value, is_func, n_args, latex_fmt ) \
    latex_fmt,

static const char* latex_format[] = {
    INIT_OPERATIONS( OPERATIONS_LATEX )
};

#undef OPERATIONS_LATEX


void TreeDumpLatex( const Tree_t* tree, FILE* latex_file ) {
    if ( !tree ) return;
    if ( !latex_file ) return;

    NodeToLatex( tree->root, latex_file );
}

static void NodeToLatex( const Node_t* node, FILE* latex_file ) {
    if ( !node ) return;

    switch ( node->value.type ) {
        case NODE_NUMBER:
            LATEX_PRINT( " %g ", node->value.data.number);
            break;
        case NODE_VARIABLE:
            LATEX_PRINT( " %c ", node->value.data.variable);
            break;
        case NODE_OPERATION: {
            const int op = node->value.data.operation;
            const char* format = latex_format[op];
            LatexInsertChildren( node, latex_file, format );
            break;
        }
        case NODE_UNKNOWN:
        default: 
            LATEX_PRINT( "???" ); 
            break;
    }
}

static void LatexInsertChildren( const Node_t* node, FILE* latex_file, const char* format_string ) {
    int child_id = 0;

    for ( size_t idx = 0; format_string[ idx ]; idx++ ) {
        if ( format_string[idx] == '%' && format_string[ idx + 1 ] == 'e' ) {
            const Node_t* child = ( child_id == 0 ? node->left : node->right );

            if ( child )
                NodeToLatex( child, latex_file );
            else
                LATEX_PRINT( "<?>" );

            child_id++;
            idx++;
        } else {
            fputc( format_string[idx], latex_file );
        }
    }
}


#ifdef _DEBUG
static void DiffNodeDumpLatex( Differentiator_t* diff, Node_t* original, Node_t* derivative, char independent_var, int order ) {
    FILE* latex_file = diff->latex.tex_file;
    LATEX_PRINT( "\\[\n" );
    LATEX_PRINT( "\\frac{d^{%d}}{d %c^{%d}} ", order, independent_var, order);
    NodeToLatex(original, latex_file );
    LATEX_PRINT( " = " );
    NodeToLatex(derivative, latex_file );
    LATEX_PRINT( "\n\\]\n" );
    fflush( latex_file );
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
            if ( !diff->expr_tree ) {
                PRINT_HTML( "Empty tree" );
                break;
            }

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

// ======================== OPTIMIZATION ========================
#include <cstdlib>
#include <cmath>
#include <cstring>
#include "Differentiator.h"
#include "DebugUtils.h"
#include "Tree.h"

static bool ContainsVariable( Node_t* node, char independent_var );
static bool EvaluateConstant( Node_t* node, VarTable_t* var_table, double* result );
static void OptimizeConstantsNode( Node_t* node, VarTable_t* var_table, char independent_var );
static bool IsNumber( Node_t* node, double value );
static bool NodesEqual( Node_t* a, Node_t* b );
static void ReplaceNode( Node_t** node_ptr, Node_t* new_node );
static bool SimplifyTreeNode( Node_t** node );
static bool CompareDoubleToDouble( double a, double b, double eps = 1e-10 );

bool OptimizeTree( Tree_t* tree, Differentiator_t* diff, char independent_var ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( diff, "Null pointer on `diff`" );

    OptimizeConstants( tree, &diff->var_table, independent_var );
    SimplifyTree( tree );

    return true;
}

bool OptimizeConstants( Tree_t* tree, VarTable_t* var_table, char independent_var ) {
    my_assert( tree, "Null pointer on `tree`" );
    if ( !tree->root ) return true;

    OptimizeConstantsNode( tree->root, var_table, independent_var );
    return true;
}

bool SimplifyTree( Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );
    if ( !tree->root ) return true;

    bool changed = true;
    int iterations = 0;
    const int max_iterations = 100;

    while ( changed ) {
        changed = SimplifyTreeNode( &tree->root );
        iterations++;
        if ( iterations > max_iterations ) {
            PRINT_ERROR( "Too many iterations in `SimplifyTree`\n" );
            break;
        }
    }

    return true;
}

static bool ContainsVariable( Node_t* node, char independent_var ) {
    if ( !node ) return false;

    if ( node->value.type == NODE_VARIABLE ) return node->value.data.variable == independent_var;
    if ( node->value.type == NODE_NUMBER ) return false;

    return ContainsVariable( node->left, independent_var ) || ContainsVariable( node->right, independent_var );
}

static bool EvaluateConstant( Node_t* node, VarTable_t* var_table, double* result ) {
    if ( !node || !result ) return false;

    switch ( node->value.type ) {
        case NODE_NUMBER:
            *result = node->value.data.number;
            return true;

        case NODE_VARIABLE: {
            // Если переменная есть в таблице и это не независимая переменная
            double value = 0.0;
            if ( VarTableGet( var_table, node->value.data.variable, &value ) && value != NAN ) {
                *result = value;
                return true;
            }
            return false;
        }

        case NODE_OPERATION: {
            double left = 0, right = 0;
            bool left_ok = true, right_ok = true;

            if ( node->left ) left_ok = EvaluateConstant( node->left, var_table, &left );
            if ( node->right ) right_ok = EvaluateConstant( node->right, var_table, &right );

            if ( !left_ok || !right_ok ) return false;

            OperationType op = (OperationType) node->value.data.operation;
            switch ( op ) {
                case OP_ADD: *result = left + right; return true;
                case OP_SUB: *result = left - right; return true;
                case OP_MUL: *result = left * right; return true;
                case OP_DIV: 
                    if ( fabs(right) < 1e-15 ) return false;
                    *result = left / right; return true;
                case OP_POW: *result = pow(left, right); return true;
                case OP_LOG:
                    if ( left <= 0 || right <= 0 || CompareDoubleToDouble(left, 1.0) ) return false;
                    *result = log(right)/log(left); return true;
                case OP_SIN: *result = sin(left); return true;
                case OP_COS: *result = cos(left); return true;
                case OP_TAN: *result = tan(left); return true;
                case OP_CTAN:
                    if ( CompareDoubleToDouble(tan(left),0.0) ) return false;
                    *result = 1.0/tan(left); return true;
                case OP_SH: *result = sinh(left); return true;
                case OP_CH: *result = cosh(left); return true;
                case OP_ARCSIN: 
                    if ( left < -1.0 || left > 1.0 ) return false;
                    *result = asin(left); return true;
                case OP_ARCCOS: 
                    if ( left < -1.0 || left > 1.0 ) return false;
                    *result = acos(left); return true;
                case OP_ARCTAN: *result = atan(left); return true;
                case OP_ARCCTAN: *result = atan(1.0/left); return true;
                case OP_ARSINH: *result = asinh(left); return true;
                case OP_ARCH: 
                    if ( left < 1.0 ) return false;
                    *result = acosh(left); return true;
                case OP_ARTANH: 
                    if ( left <= -1.0 || left >= 1.0 ) return false;
                    *result = atanh(left); return true;

                case OP_NOPE:
                default: 
                    return false;
            }
        }

        case NODE_UNKNOWN:
        default:
            return false;
    }
}

static void OptimizeConstantsNode( Node_t* node, VarTable_t* var_table, char independent_var ) {
    my_assert( node, "Null pointer on `node`" );

    if ( node->left ) OptimizeConstantsNode( node->left, var_table, independent_var );
    if ( node->right ) OptimizeConstantsNode( node->right, var_table, independent_var );

    if ( node->value.type == NODE_OPERATION ) {
        bool left_const  = node->left  ? !ContainsVariable(node->left, independent_var) : true;
        bool right_const = node->right ? !ContainsVariable(node->right, independent_var) : true;

        if ( left_const && right_const ) {
            double result = 0.0;
            if ( EvaluateConstant(node, var_table, &result) ) {
                node->value.type = NODE_NUMBER;
                node->value.data.number = result;

                if ( node->left ) { NodeDelete(node->left, NULL, NULL); node->left = NULL; }
                if ( node->right ) { NodeDelete(node->right, NULL, NULL); node->right = NULL; }
            }
        }
    }
}

static bool IsNumber( Node_t* node, double value ) {
    return node && node->value.type == NODE_NUMBER && CompareDoubleToDouble(node->value.data.number, value);
}

static bool NodesEqual( Node_t* a, Node_t* b ) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    if (a->value.type != b->value.type) return false;

    switch ( a->value.type ) {
        case NODE_NUMBER: return CompareDoubleToDouble(a->value.data.number, b->value.data.number);
        case NODE_VARIABLE: return a->value.data.variable == b->value.data.variable;
        case NODE_OPERATION:
            return a->value.data.operation == b->value.data.operation &&
                   NodesEqual(a->left, b->left) &&
                   NodesEqual(a->right, b->right);
        case NODE_UNKNOWN:
        default: return false;
    }
}

static void ReplaceNode( Node_t** node_ptr, Node_t* new_node ) {
    if (!node_ptr) return;

    Node_t* old_node = *node_ptr;
    if (!old_node) {
        if ( new_node ) new_node->parent = NULL;
        *node_ptr = new_node;
        return;
    }

    if ( old_node == new_node ) return;

    Node_t* parent = old_node->parent;
    if ( new_node ) new_node->parent = parent;

    if ( old_node->left ) { NodeDelete(old_node->left,NULL,NULL); old_node->left=NULL; }
    if ( old_node->right ){ NodeDelete(old_node->right,NULL,NULL); old_node->right=NULL; }

    free(old_node);
    *node_ptr = new_node;
    if ( new_node ) new_node->parent = parent;
}

static bool SimplifyAdd(Node_t** node_ptr);
static bool SimplifySub(Node_t** node_ptr);
static bool SimplifyMul(Node_t** node_ptr);
static bool SimplifyDiv(Node_t** node_ptr);
static bool SimplifyPow(Node_t** node_ptr);

static bool SimplifyTreeNode( Node_t** node_ptr ) {
    Node_t* node = *node_ptr;
    if (!node) return false;

    bool changed = false;
    if ( node->left )  changed |= SimplifyTreeNode(&node->left);
    if ( node->right ) changed |= SimplifyTreeNode(&node->right);

    if ( node->value.type != NODE_OPERATION ) return changed;

    switch ( ( OperationType ) node->value.data.operation ) {
        case OP_ADD: changed |= SimplifyAdd( node_ptr ); break;
        case OP_SUB: changed |= SimplifySub( node_ptr ); break;
        case OP_MUL: changed |= SimplifyMul( node_ptr ); break;
        case OP_DIV: changed |= SimplifyDiv( node_ptr ); break;
        case OP_POW: changed |= SimplifyPow( node_ptr ); break;
        default: break;
    }

    return changed;
}

static bool SimplifyAdd(Node_t** node_ptr) {
    Node_t* node = *node_ptr;
    if ( IsNumber(node->left, 0.0) && node->right ) { ReplaceNode(node_ptr, NodeCopy(node->right)); return true; }
    if ( IsNumber(node->right, 0.0) && node->left ) { ReplaceNode(node_ptr, NodeCopy(node->left)); return true; }
    return false;
}

static bool SimplifySub(Node_t** node_ptr) {
    Node_t* node = *node_ptr;
    if ( IsNumber(node->right, 0.0) && node->left ) { ReplaceNode(node_ptr, NodeCopy(node->left)); return true; }
    if ( NodesEqual(node->left, node->right) ) { ReplaceNode(node_ptr, NodeCreate(MakeNumber(0.0), node->parent)); return true; }
    return false;
}

static bool SimplifyMul(Node_t** node_ptr) {
    Node_t* node = *node_ptr;
    if ( IsNumber(node->left, 0.0) || IsNumber(node->right, 0.0) ) { ReplaceNode(node_ptr, NodeCreate(MakeNumber(0.0), node->parent)); return true; }
    if ( IsNumber(node->left, 1.0) && node->right ) { ReplaceNode(node_ptr, NodeCopy(node->right)); return true; }
    if ( IsNumber(node->right, 1.0) && node->left ) { ReplaceNode(node_ptr, NodeCopy(node->left)); return true; }
    return false;
}

static bool SimplifyDiv(Node_t** node_ptr) {
    Node_t* node = *node_ptr;
    if ( IsNumber(node->right, 1.0) && node->left ) { ReplaceNode(node_ptr, NodeCopy(node->left)); return true; }
    return false;
}

static bool SimplifyPow(Node_t** node_ptr) {
    Node_t* node = *node_ptr;
    if ( IsNumber( node->right, 0.0) ) { 
        ReplaceNode(
            node_ptr, 
            NodeCreate(MakeNumber( 1.0 ), node->parent ) 
        ); 
        return true; 
    }
    if ( IsNumber( node->right, 1.0 ) && node->left ) { 
        ReplaceNode(
            node_ptr, 
            NodeCopy( node->left )
        ); 
        return true; 
    }
    if ( IsNumber( node->left,  1.0 ) ) { 
        ReplaceNode(
            node_ptr, 
            NodeCreate( MakeNumber( 1.0 ), node->parent )
        ); 
        return true; 
    }
    return false;
}

static bool CompareDoubleToDouble(double a, double b, double eps) {
    return fabs(a-b) < eps;
}


