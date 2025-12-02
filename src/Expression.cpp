#include <cmath>
#include <math.h>
#include <string.h>

#include "Differentiator.h"
#include "DebugUtils.h"
#include "Tree.h"

const size_t MAX_SIZE = 256;

static double EvaluateNode( Node_t* node, Variable_t variables[ MAX_SIZE ], size_t number_of_variables );
static double SearchVariable( char name, Variable_t variables[ MAX_SIZE ], size_t* number_of_variables );

double EvaluateTree( Tree_t* tree, Differentiator_t* diff ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( diff, "Null pointer on `diff`" );

    double result = 0;
    Variable_t inited_variables[ MAX_SIZE ] = {};
    size_t size = 0;

    result = EvaluateNode( tree->root, inited_variables, size );

    return result;
}

static double EvaluateNode( Node_t* node, Variable_t variables[ MAX_SIZE ], size_t number_of_variables ) {
    if ( !node ) {
        return 0;
    }

    switch ( node->value.type ) {
        case NODE_NUMBER:
            return node->value.data.number;
        case NODE_VARIABLE:
            return SearchVariable( node->value.data.variable, variables, &number_of_variables );
        case NODE_OPERATION: {
            double L = EvaluateNode( node->left, variables, number_of_variables );
            double R = EvaluateNode( node->right, variables, number_of_variables );

            switch ( node->value.data.operation ) {
                case OP_ADD:     return L + R;
                case OP_SUB:     return L - R;
                case OP_MUL:     return L * R;
                case OP_DIV:     return L / R;
                case OP_POW:     return pow( L, R );
                case OP_LOG:     return log(L) / log(R);
                case OP_LN:      return log(L);
                
                case OP_SIN:     return sin(L);
                case OP_COS:     return cos(L);
                case OP_TAN:     return tan(L);
                case OP_CTAN:    return 1.0 / tan(L);

                case OP_SH:      return sinh(L);
                case OP_CH:      return cosh(L);

                case OP_ARCSIN:  return asin(L);
                case OP_ARCCOS:  return acos(L);
                case OP_ARCTAN:  return atan(L);
                case OP_ARCCTAN: return atan( 1.0 / L );

                case OP_ARSINH:  return asinh(L);
                case OP_ARCH:    return acosh(L);
                case OP_ARTANH:  return atanh(L);

                default:
                    PRINT_ERROR( "Error: unknown operation '%s'\n", operations_txt[ node->value.data.operation ] );
                    return NAN;
            }
        }
        
        case NODE_UNKNOWN:
        default:
            PRINT_ERROR( "Error: invalid node in the calculation! \n" );
            return NAN;
    }
}

static double SearchVariable( char name, Variable_t variables[ MAX_SIZE ], size_t* number_of_variables ) {
    my_assert( variables, "Null pointer on `inited_variables" );

    for ( size_t idx = 0; idx < *number_of_variables; idx++ ) {
        if ( name == variables[ idx ].name ) {
            return variables[ idx ].value;
        }
    }

    printf( "Input the valube of variable `%c`: ", name );

    if ( scanf( "%lf", &( variables[ *number_of_variables ].value ) ) != 1 ) {
        PRINT_ERROR( "Invalid input for variable %c. Try again, pls: ", name );
    }

    return variables[ ( *number_of_variables )++ ].value;
}
