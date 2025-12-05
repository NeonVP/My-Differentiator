#include "DebugUtils.h"
#include "Differentiator.h"
#include "Tree.h"
#include <cmath>
#include <cstdio>

static double EvaluateNode( Node_t *node, VarTable_t *var_table );

double EvaluateTree( Tree_t *tree, Differentiator_t *diff ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( diff, "Null pointer on `diff`" );

    return EvaluateNode( tree->root, &diff->var_table );
}

static double EvaluateNode( Node_t *node, VarTable_t *var_table ) {
    if ( !node )
        return 0.0;

    switch ( node->value.type ) {
        case NODE_NUMBER:
            return node->value.data.number;

        case NODE_VARIABLE: {
            double value = 0.0;
            if ( !VarTableGet( var_table, node->value.data.variable,
                               &value ) ) {
                
                printf( "Enter value for variable %c: ",
                        node->value.data.variable );
                if ( scanf( "%lf", &value ) != 1 ) {
                    printf( "Invalid input. Using 0.0 for %c\n",
                            node->value.data.variable );
                    value = 0.0;
                    int c;
                    while ( ( c = getchar() ) != '\n' && c != EOF ) {
                    }
                }
                VarTableSet( var_table, node->value.data.variable, value );
            }
            
            return value;
        }

        case NODE_OPERATION: {
            double L = EvaluateNode( node->left, var_table );
            double R = EvaluateNode( node->right, var_table );

            switch ( node->value.data.operation ) {
                case OP_ADD:
                    return L + R;
                case OP_SUB:
                    return L - R;
                case OP_MUL:
                    return L * R;
                case OP_DIV:
                    return L / R;
                case OP_POW:
                    return pow( L, R );
                case OP_LOG:
                    return log( L ) / log( R );
                case OP_LN:
                    return log( L );

                case OP_SIN:
                    return sin( L );
                case OP_COS:
                    return cos( L );
                case OP_TAN:
                    return tan( L );
                case OP_CTAN:
                    return 1.0 / tan( L );

                case OP_SH:
                    return sinh( L );
                case OP_CH:
                    return cosh( L );

                case OP_ARCSIN:
                    return asin( L );
                case OP_ARCCOS:
                    return acos( L );
                case OP_ARCTAN:
                    return atan( L );
                case OP_ARCCTAN:
                    return atan( 1.0 / L );

                case OP_ARSINH:
                    return asinh( L );
                case OP_ARCH:
                    return acosh( L );
                case OP_ARTANH:
                    return atanh( L );

                default:
                    PRINT_ERROR( "Error: unknown operation\n" );
                    return NAN;
            }
        }

        case NODE_UNKNOWN:
        default:
            PRINT_ERROR( "Error: invalid node in the calculation\n" );
            return NAN;
    }
}
