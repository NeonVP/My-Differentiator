#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "DebugUtils.h"
#include "Differentiator.h"
#include "Tree.h"


static bool ContainsVariable( Node_t *node, char independent_var );
static bool EvaluateConstant( Node_t *node, VarTable_t *var_table, double *result );
static bool IsNumber( Node_t *node, double value );
static bool NodesEqual( Node_t *a, Node_t *b );
static void ReplaceNode( Node_t **node_ptr, Node_t *new_node );

static void OptimizeConstantsNode( Node_t **node_ptr, VarTable_t *var_table, char independent_var );
static void TryEvaluateAndReplaceIfConstant( Node_t **node_ptr, VarTable_t *var_table, char independent_var );

// Упрощение переменных
static bool SimplifyVariablesNode( Node_t **node_ptr, char independent_var );
static bool ApplySimplificationRule( Node_t **node_ptr, char independent_var );

// Правила упрощения по операциям
static bool TrySimplifySub( Node_t **node_ptr, char independent_var );
static bool TrySimplifyPow( Node_t **node_ptr, char independent_var );
static bool TrySimplifyMul( Node_t **node_ptr, char independent_var );
static bool TrySimplifyAdd( Node_t **node_ptr, char independent_var );
static bool TrySimplifyDiv( Node_t **node_ptr, char independent_var );

// Утилиты замены
static void ReplaceWithZero( Node_t **node_ptr );
static void ReplaceWithOne( Node_t **node_ptr );
static void ReplaceWithCopy( Node_t **node_ptr, Node_t *original );

bool OptimizeTree( Tree_t *tree, Differentiator_t *diff, char independent_var ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( diff, "Null pointer on `diff`" );

    if ( !tree->root )
        return true;

    const int max_passes = 100;
    int passes = 0;
    bool changed = true;

    while ( changed && passes < max_passes ) {
        changed = false;

        OptimizeConstantsNode( &tree->root, &diff->var_table, independent_var );
        changed = SimplifyVariablesNode( &tree->root, independent_var );

        passes++;
    }

    return true;
}

static void OptimizeConstantsNode( Node_t **node_ptr, VarTable_t *var_table, char independent_var ) {
    if ( !node_ptr || !*node_ptr )
        return;

    Node_t *node = *node_ptr;

    OptimizeConstantsNode( &node->left, var_table, independent_var );
    OptimizeConstantsNode( &node->right, var_table, independent_var );

    if ( node->value.type == NODE_OPERATION ) {
        TryEvaluateAndReplaceIfConstant( node_ptr, var_table, independent_var );
    }
}

static void TryEvaluateAndReplaceIfConstant( Node_t **node_ptr, VarTable_t *var_table,
                                             char independent_var ) {
    Node_t *node = *node_ptr;

    bool left_const = !node->left || !ContainsVariable( node->left, independent_var );
    bool right_const = !node->right || !ContainsVariable( node->right, independent_var );

    if ( left_const && right_const ) {
        double result = 0.0;
        if ( EvaluateConstant( node, var_table, &result ) ) {
            Node_t *new_node = NodeCreate( MakeNumber( result ), node->parent );
            ReplaceNode( node_ptr, new_node );
        }
    }
}

static bool SimplifyVariablesNode( Node_t **node_ptr, char independent_var ) {
    if ( !node_ptr || !*node_ptr )
        return false;

    Node_t *node = *node_ptr;
    bool changed = false;

    changed |= SimplifyVariablesNode( &node->left, independent_var );
    changed |= SimplifyVariablesNode( &node->right, independent_var );

    if ( node->value.type == NODE_OPERATION ) {
        return ApplySimplificationRule( node_ptr, independent_var ) || changed;
    }

    return changed;
}

static bool ApplySimplificationRule( Node_t **node_ptr, char independent_var ) {
    Node_t *node = *node_ptr;
    OperationType op = (OperationType)node->value.data.operation;

    switch ( op ) {
        case OP_SUB:
            return TrySimplifySub( node_ptr, independent_var );
        case OP_POW:
            return TrySimplifyPow( node_ptr, independent_var );
        case OP_MUL:
            return TrySimplifyMul( node_ptr, independent_var );
        case OP_ADD:
            return TrySimplifyAdd( node_ptr, independent_var );
        case OP_DIV:
            return TrySimplifyDiv( node_ptr, independent_var );
        default:
            return false;
    }
}

static bool TrySimplifySub( Node_t **node_ptr, char independent_var ) {
    Node_t *node = *node_ptr;

    if ( node->left && node->right && NodesEqual( node->left, node->right ) ) {
        ReplaceWithZero( node_ptr );
        return true;
    }

    if ( node->right && IsNumber( node->right, 0.0 ) && node->left ) {
        ReplaceWithCopy( node_ptr, node->left );
        return true;
    }

    return false;
}

static bool TrySimplifyPow( Node_t **node_ptr, char independent_var ) {
    Node_t *node = *node_ptr;

    if ( node->right && IsNumber( node->right, 0.0 ) ) {
        ReplaceWithOne( node_ptr );
        return true;
    }

    if ( node->right && IsNumber( node->right, 1.0 ) && node->left ) {
        ReplaceWithCopy( node_ptr, node->left );
        return true;
    }

    if ( node->left && IsNumber( node->left, 0.0 ) && node->right ) {
        if ( ContainsVariable( node->right, independent_var ) ) {
            ReplaceWithZero( node_ptr );
            return true;
        }
    }

    return false;
}

static bool TrySimplifyMul( Node_t **node_ptr, char independent_var ) {
    Node_t *node = *node_ptr;

    if ( ( node->left && IsNumber( node->left, 0.0 ) ) || ( node->right && IsNumber( node->right, 0.0 ) ) ) {
        ReplaceWithZero( node_ptr );
        return true;
    }

    if ( node->left && IsNumber( node->left, 1.0 ) && node->right ) {
        ReplaceWithCopy( node_ptr, node->right );
        return true;
    }

    if ( node->right && IsNumber( node->right, 1.0 ) && node->left ) {
        ReplaceWithCopy( node_ptr, node->left );
        return true;
    }

    return false;
}

static bool TrySimplifyAdd( Node_t **node_ptr, char independent_var ) {
    Node_t *node = *node_ptr;

    if ( node->right && IsNumber( node->right, 0.0 ) && node->left ) {
        ReplaceWithCopy( node_ptr, node->left );
        return true;
    }

    if ( node->left && IsNumber( node->left, 0.0 ) && node->right ) {
        ReplaceWithCopy( node_ptr, node->right );
        return true;
    }

    return false;
}

static bool TrySimplifyDiv( Node_t **node_ptr, char independent_var ) {
    Node_t *node = *node_ptr;

    if ( node->right && IsNumber( node->right, 1.0 ) && node->left ) {
        ReplaceWithCopy( node_ptr, node->left );
        return true;
    }

    if ( node->left && IsNumber( node->left, 0.0 ) && node->right ) {
        if ( ContainsVariable( node->right, independent_var ) ) {
            ReplaceWithZero( node_ptr );
            return true;
        }
    }

    return false;
}

static void ReplaceWithZero( Node_t **node_ptr ) {
    ReplaceNode( node_ptr, NodeCreate( MakeNumber( 0.0 ), NULL ) );
}

static void ReplaceWithOne( Node_t **node_ptr ) {
    ReplaceNode( node_ptr, NodeCreate( MakeNumber( 1.0 ), NULL ) );
}

static void ReplaceWithCopy( Node_t **node_ptr, Node_t *original ) {
    ReplaceNode( node_ptr, NodeCopy( original ) );
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
            double val = 0.0;
            if ( VarTableGet( var_table, node->value.data.variable, &val ) && isfinite( val ) ) {
                *result = val;
                return true;
            }
            return false;
        }

        case NODE_OPERATION: {
            OperationType op = (OperationType)node->value.data.operation;
            double left_val = 0.0, right_val = 0.0;
            bool left_ok = node->left ? EvaluateConstant( node->left, var_table, &left_val ) : true;
            bool right_ok = node->right ? EvaluateConstant( node->right, var_table, &right_val ) : true;

            if ( !left_ok || !right_ok )
                return false;

            switch ( op ) {
                case OP_ADD:
                    *result = left_val + right_val;
                    return true;
                case OP_SUB:
                    *result = left_val - right_val;
                    return true;
                case OP_MUL:
                    *result = left_val * right_val;
                    return true;
                case OP_DIV:
                    if ( CompareDoubleToDouble( right_val, 0.0 ) == 0 )
                        return false;
                    *result = left_val / right_val;
                    return true;
                case OP_POW:
                    *result = pow( left_val, right_val );
                    return isfinite( *result );
                default:
                    return false;
            }
        }

        default:
            return false;
    }
}

static bool IsNumber( Node_t *node, double value ) {
    return node && node->value.type == NODE_NUMBER &&
           CompareDoubleToDouble( node->value.data.number, value, 1e-10 ) == 0;
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
            return CompareDoubleToDouble( a->value.data.number, b->value.data.number, 1e-10 ) == 0;
        case NODE_VARIABLE:
            return a->value.data.variable == b->value.data.variable;
        case NODE_OPERATION:
            return a->value.data.operation == b->value.data.operation && NodesEqual( a->left, b->left ) &&
                   NodesEqual( a->right, b->right );
        default:
            return false;
    }
}

static void ReplaceNode( Node_t **node_ptr, Node_t *new_node ) {
    if ( !node_ptr )
        return;
    Node_t *old = *node_ptr;
    if ( old == new_node )
        return;

    Node_t *parent = old ? old->parent : NULL;

    if ( old ) {
        old->parent = NULL;
        if ( old->left )
            old->left->parent = NULL;
        if ( old->right )
            old->right->parent = NULL;
        NodeDelete( old, NULL, NULL );
    }

    *node_ptr = new_node;
    if ( new_node ) {
        new_node->parent = parent;
        if ( new_node->left )
            new_node->left->parent = new_node;
        if ( new_node->right )
            new_node->right->parent = new_node;
    }
}