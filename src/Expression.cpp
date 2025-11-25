#include <cmath>
#include <math.h>
#include <string.h>

#include "Expression.h"
#include "DebugUtils.h"

const size_t MAX_SIZE = 256;

struct Variable_t {
    char name;
    double value;
};

static double EvaluateNode( Node_t* node, Variable_t variables[ MAX_SIZE ], size_t number_of_variables );
static double SearchVariable( char name, Variable_t variables[ MAX_SIZE ], size_t* number_of_variables );

double EvaluateTree( const Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );

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
                case OP_ADD: return L + R;
                case OP_SUB: return L - R;
                case OP_MUL: return L * R;
                case OP_DIV: return L / R;
                case OP_SIN: return sin( L );
                case OP_COS: return cos( L );
                case OP_TAN: return tan( L );
                case OP_POW: return pow( L, R );

                default:
                    PRINT_ERROR( "Ошибка: неизвестная операция '%s'\n", operations_txt[ node->value.data.operation ] );
                    return NAN;
            }
        }
        
        case NODE_UNKNOWN:
        default:
            PRINT_ERROR( "Ошибка: такая нода вообще не должна была здесь появиться!!! \n" );
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

    printf( "Введите значение переменной %c: ", name );

    // TODO: check for incorrect input
    variables[ *number_of_variables ].name = name;
    scanf( "%lf", &( variables[ *number_of_variables ].value ) );

    return variables[ ( *number_of_variables )++ ].value;
}
