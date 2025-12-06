#include "DebugUtils.h"
#include "Differentiator.h"
#include "Tree.h"
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>

static bool ContainsVariable( Node_t *node, char independent_var );
static bool EvaluateConstant( Node_t *node, VarTable_t *var_table, double *result );
static void OptimizeConstantsNode( Node_t *node, VarTable_t *var_table, char independent_var );
static bool IsNumber( Node_t *node, double value );
static bool NodesEqual( Node_t *a, Node_t *b );
static void ReplaceNode( Node_t **node_ptr, Node_t *new_node );
static bool SimplifyTreeNode( Node_t **node );

bool OptimizeTree( Tree_t *tree, Differentiator_t *diff, char independent_var ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( diff, "Null pointer on `diff`" );

    OptimizeConstants( tree, diff, independent_var );
    SimplifyTree( tree );

    return true;
}

bool OptimizeConstants( Tree_t *tree, Differentiator_t *diff, char independent_var ) {
    my_assert( tree, "Null pointer on `tree`" );
    if ( !tree->root )
        return true;

    OptimizeConstantsNode( tree->root, &diff->var_table, independent_var );
    return true;
}

bool SimplifyTree( Tree_t *tree ) {
    my_assert( tree, "Null pointer on `tree`" );
    if ( !tree->root )
        return true;

    bool changed = true;
    int iterations = 0;
    const int max_iterations = 100;

    while ( changed ) {
        changed = SimplifyTreeNode( &( tree->root ) );
        iterations++;
        if ( iterations > max_iterations ) {
            PRINT_ERROR( "Too many iterations in `SimplifyTree`\n" );
            break;
        }
    }

    return true;
}

static bool ContainsVariable( Node_t *node, char independent_var ) {
    if ( !node )
        return false;

    if ( node->value.type == NODE_VARIABLE )
        return node->value.data.variable == independent_var;
    if ( node->value.type == NODE_NUMBER )
        return false;

    return ContainsVariable( node->left, independent_var ) ||
           ContainsVariable( node->right, independent_var );
}

static bool EvaluateConstant( Node_t *node, VarTable_t *var_table, double *result ) {
    if ( !node || !result )
        return false;

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

            if ( node->left )
                left_ok = EvaluateConstant( node->left, var_table, &left );
            if ( node->right )
                right_ok = EvaluateConstant( node->right, var_table, &right );

            if ( !left_ok || !right_ok )
                return false;

            OperationType op = (OperationType)node->value.data.operation;
            switch ( op ) {
                case OP_ADD:
                    *result = left + right;
                    return true;
                case OP_SUB:
                    *result = left - right;
                    return true;
                case OP_MUL:
                    *result = left * right;
                    return true;
                case OP_DIV:
                    if ( fabs( right ) < 1e-15 )
                        return false;
                    *result = left / right;
                    return true;
                case OP_POW:
                    *result = pow( left, right );
                    return true;
                case OP_LOG:
                    if ( left <= 0 || right <= 0 || CompareDoubleToDouble( left, 1.0 ) )
                        return false;
                    *result = log( right ) / log( left );
                    return true;
                case OP_SIN:
                    *result = sin( left );
                    return true;
                case OP_COS:
                    *result = cos( left );
                    return true;
                case OP_TAN:
                    *result = tan( left );
                    return true;
                case OP_CTAN:
                    if ( CompareDoubleToDouble( tan( left ), 0.0 ) )
                        return false;
                    *result = 1.0 / tan( left );
                    return true;
                case OP_SH:
                    *result = sinh( left );
                    return true;
                case OP_CH:
                    *result = cosh( left );
                    return true;
                case OP_ARCSIN:
                    if ( left < -1.0 || left > 1.0 )
                        return false;
                    *result = asin( left );
                    return true;
                case OP_ARCCOS:
                    if ( left < -1.0 || left > 1.0 )
                        return false;
                    *result = acos( left );
                    return true;
                case OP_ARCTAN:
                    *result = atan( left );
                    return true;
                case OP_ARCCTAN:
                    *result = atan( 1.0 / left );
                    return true;
                case OP_ARSINH:
                    *result = asinh( left );
                    return true;
                case OP_ARCH:
                    if ( left < 1.0 )
                        return false;
                    *result = acosh( left );
                    return true;
                case OP_ARTANH:
                    if ( left <= -1.0 || left >= 1.0 )
                        return false;
                    *result = atanh( left );
                    return true;
                default:
                    return false;
            }
        }

        case NODE_UNKNOWN:
        default:
            return false;
    }
}

static void OptimizeConstantsNode( Node_t *node, VarTable_t *var_table, char independent_var ) {
    my_assert( node, "Null pointer on `node`" );

    if ( node->left )
        OptimizeConstantsNode( node->left, var_table, independent_var );
    if ( node->right )
        OptimizeConstantsNode( node->right, var_table, independent_var );

    if ( node->value.type == NODE_OPERATION ) {
        bool left_const = node->left ? !ContainsVariable( node->left, independent_var ) : true;
        bool right_const = node->right ? !ContainsVariable( node->right, independent_var ) : true;

        if ( left_const && right_const ) {
            double result = 0.0;
            if ( EvaluateConstant( node, var_table, &result ) ) {
                node->value.type = NODE_NUMBER;
                node->value.data.number = result;

                if ( node->left ) {
                    NodeDelete( node->left, NULL, NULL );
                    node->left = NULL;
                }
                if ( node->right ) {
                    NodeDelete( node->right, NULL, NULL );
                    node->right = NULL;
                }
            }
        }
    }
}

static bool IsNumber( Node_t *node, double value ) {
    return node && node->value.type == NODE_NUMBER && CompareDoubleToDouble( node->value.data.number, value );
}

static bool NodesEqual( Node_t *a, Node_t *b ) {
    if ( !a && !b )
        return true;
    if ( !a || !b )
        return false;
    if ( a->value.type != b->value.type )
        return false;

    switch ( a->value.type ) {
        case NODE_NUMBER:
            return CompareDoubleToDouble( a->value.data.number, b->value.data.number );
        case NODE_VARIABLE:
            return a->value.data.variable == b->value.data.variable;
        case NODE_OPERATION:
            return a->value.data.operation == b->value.data.operation && NodesEqual( a->left, b->left ) &&
                   NodesEqual( a->right, b->right );
        case NODE_UNKNOWN:
        default:
            return false;
    }
}

// static void ReplaceNode( Node_t **node_ptr, Node_t *new_node ) {
//     if ( !node_ptr )
//         return;

//     Node_t *old_node = *node_ptr;
//     if ( !old_node ) {
//         if ( new_node )
//             new_node->parent = NULL;
//         *node_ptr = new_node;
//         return;
//     }

//     if ( old_node == new_node )
//         return;

//     Node_t *parent = old_node->parent;
//     if ( new_node )
//         new_node->parent = parent;

//     if ( old_node->left ) {
//         NodeDelete( old_node->left, NULL, NULL );
//         old_node->left = NULL;
//     }
//     if ( old_node->right ) {
//         NodeDelete( old_node->right, NULL, NULL );
//         old_node->right = NULL;
//     }

//     free( old_node );
//     *node_ptr = new_node;
//     if ( new_node )
//         new_node->parent = parent;
// }

static void ReplaceNode( Node_t **node_ptr, Node_t *new_node ) {
    if ( !node_ptr )
        return;
    Node_t *old_node = *node_ptr;

    if ( !old_node ) {
        *node_ptr = new_node;
        if ( new_node )
            new_node->parent = NULL;
        return;
    }

    if ( old_node == new_node )
        return;

    *node_ptr = new_node;
    
    if ( new_node )
        new_node->parent = old_node->parent;

    if ( old_node == old_node->parent->left )
        old_node->parent->left = new_node;
    else
        old_node->parent->right = new_node;

    NodeDelete( old_node, NULL, NULL );
}

static bool SimplifyAdd( Node_t **node_ptr );
static bool SimplifySub( Node_t **node_ptr );
static bool SimplifyMul( Node_t **node_ptr );
static bool SimplifyDiv( Node_t **node_ptr );
static bool SimplifyPow( Node_t **node_ptr );

static bool SimplifyTreeNode( Node_t **node_ptr ) {
    Node_t *node = *node_ptr;
    if ( !node )
        return false;

    bool changed = false;
    if ( node->left )
        changed |= SimplifyTreeNode( &node->left );
    if ( node->right )
        changed |= SimplifyTreeNode( &node->right );

    if ( node->value.type != NODE_OPERATION )
        return changed;

    switch ( (OperationType)node->value.data.operation ) {
        case OP_ADD:
            changed |= SimplifyAdd( node_ptr );
            break;
        case OP_SUB:
            changed |= SimplifySub( node_ptr );
            break;
        case OP_MUL:
            changed |= SimplifyMul( node_ptr );
            break;
        case OP_DIV:
            changed |= SimplifyDiv( node_ptr );
            break;
        case OP_POW:
            changed |= SimplifyPow( node_ptr );
            break;
        default:
            break;
    }

    return changed;
}

static bool SimplifyAdd( Node_t **node_ptr ) {
    Node_t *node = *node_ptr;
    if ( IsNumber( node->left, 0.0 ) && node->right ) {
        ReplaceNode( node_ptr, NodeCopy( node->right ) );
        return true;
    }
    if ( IsNumber( node->right, 0.0 ) && node->left ) {
        ReplaceNode( node_ptr, NodeCopy( node->left ) );
        return true;
    }
    return false;
}

static bool SimplifySub( Node_t **node_ptr ) {
    Node_t *node = *node_ptr;
    if ( IsNumber( node->right, 0.0 ) && node->left ) {
        ReplaceNode( node_ptr, NodeCopy( node->left ) );
        return true;
    }
    if ( NodesEqual( node->left, node->right ) ) {
        ReplaceNode( node_ptr, NodeCreate( MakeNumber( 0.0 ), node->parent ) );
        return true;
    }
    return false;
}

static bool SimplifyMul( Node_t **node_ptr ) {
    Node_t *node = *node_ptr;
    if ( IsNumber( node->left, 0.0 ) || IsNumber( node->right, 0.0 ) ) {
        ReplaceNode( node_ptr, NodeCreate( MakeNumber( 0.0 ), node->parent ) );
        return true;
    }
    if ( IsNumber( node->left, 1.0 ) && node->right ) {
        ReplaceNode( node_ptr, NodeCopy( node->right ) );
        return true;
    }
    if ( IsNumber( node->right, 1.0 ) && node->left ) {
        ReplaceNode( node_ptr, NodeCopy( node->left ) );
        return true;
    }
    return false;
}

static bool SimplifyDiv( Node_t **node_ptr ) {
    Node_t *node = *node_ptr;
    if ( IsNumber( node->right, 1.0 ) && node->left ) {
        ReplaceNode( node_ptr, NodeCopy( node->left ) );
        return true;
    }
    return false;
}

static bool SimplifyPow( Node_t **node_ptr ) {
    Node_t *node = *node_ptr;
    if ( IsNumber( node->right, 0.0 ) ) {
        ReplaceNode( node_ptr, NodeCreate( MakeNumber( 1.0 ), node->parent ) );
        return true;
    }
    if ( IsNumber( node->right, 1.0 ) && node->left ) {
        ReplaceNode( node_ptr, NodeCopy( node->left ) );
        return true;
    }
    if ( IsNumber( node->left, 1.0 ) ) {
        ReplaceNode( node_ptr, NodeCreate( MakeNumber( 1.0 ), node->parent ) );
        return true;
    }
    return false;
}
