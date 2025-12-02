#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "Differentiator.h"
#include "DebugUtils.h"

static bool ContainsVariable(Node_t* node, char independent_var) {
    if (!node) return false;
    
    if (node->value.type == NODE_VARIABLE) {
        return node->value.data.variable == independent_var;
    }
    
    if (node->value.type == NODE_NUMBER) {
        return false;
    }
    
    return ContainsVariable(node->left, independent_var) || 
           ContainsVariable(node->right, independent_var);
}

static bool EvaluateConstant(Node_t* node, char independent_var, double* result) {
    if (!node || !result) return false;
    
    switch (node->value.type) {
        case NODE_NUMBER:
            *result = node->value.data.number;
            return true;
            
        case NODE_VARIABLE:
            if ( node->value.data.variable != independent_var ) {
                *result = 1.0;
                return true;
            }
            return false;  // Не можем вычислить
            
        case NODE_OPERATION: {
            double left = 0, right = 0;
            bool left_ok = true, right_ok = true;
            
            if (node->left) {
                left_ok = EvaluateConstant(node->left, independent_var, &left);
            }
            if (node->right) {
                right_ok = EvaluateConstant(node->right, independent_var, &right);
            }
            
            if (!left_ok && !right_ok) return false;
            if (!left_ok || !right_ok) {
                // Один из операндов содержит переменную
                return false;
            }
            
            OperationType op = node->value.data.operation;
            
            switch (op) {
                case OP_ADD: *result = left + right; return true;
                case OP_SUB: *result = left - right; return true;
                case OP_MUL: *result = left * right; return true;
                case OP_DIV: 
                    if (right != 0) {
                        *result = left / right;
                        return true;
                    }
                    return false;
                case OP_POW: *result = pow(left, right); return true;
                case OP_LOG: *result = log(left) / log(right); return true;
                case OP_SIN: *result = sin(left); return true;
                case OP_COS: *result = cos(left); return true;
                case OP_TAN: *result = tan(left); return true;
                case OP_CTAN: *result = 1.0 / tan(left); return true;
                case OP_SH: *result = sinh(left); return true;
                case OP_CH: *result = cosh(left); return true;
                case OP_ARCSIN: *result = asin(left); return true;
                case OP_ARCCOS: *result = acos(left); return true;
                case OP_ARCTAN: *result = atan(left); return true;
                case OP_ARCCTAN: *result = atan(1.0 / left); return true;
                case OP_ARSINH: *result = asinh(left); return true;
                case OP_ARCH: *result = acosh(left); return true;
                case OP_ARTANH: *result = atanh(left); return true;
                default:
                    return false;
            }
        }
        
        case NODE_UNKNOWN:
        default:
            return false;
    }
}

static void OptimizeConstantsNode( Node_t* node, char independent_var ) {
    my_assert( node, "Null pointer on `node`" );
    
    if ( node->left  ) OptimizeConstantsNode( node->left,  independent_var );
    if ( node->right ) OptimizeConstantsNode( node->right, independent_var );
    
    if ( node->value.type == NODE_OPERATION ) {
        bool left_const  = node->left  ? !ContainsVariable( node->left,  independent_var ) : true;
        bool right_const = node->right ? !ContainsVariable( node->right, independent_var ) : true;
        
        if ( left_const && right_const ) {
            double result = 0;
            if ( EvaluateConstant( node, independent_var, &result ) ) {
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

static bool IsNumber( Node_t* node, double value ) {
    return node && 
           node->value.type == NODE_NUMBER && 
           fabs(node->value.data.number - value) < 1e-10;
}

static bool IsAnyNumber(Node_t* node) {
    return node && node->value.type == NODE_NUMBER;
}

static bool IsVariable(Node_t* node) {
    return node && node->value.type == NODE_VARIABLE;
}

static bool NodesEqual(Node_t* a, Node_t* b) {
    if ( !a && !b ) return true;
    if ( !a || !b ) return false;
    
    if ( a->value.type != b->value.type ) return false;
    
    switch ( a->value.type ) {
        case NODE_NUMBER:
            return fabs(a->value.data.number - b->value.data.number) < 1e-10;
        case NODE_VARIABLE:
            return a->value.data.variable == b->value.data.variable;
        case NODE_OPERATION:
            return a->value.data.operation == b->value.data.operation &&
                   NodesEqual(a->left, b->left) &&
                   NodesEqual(a->right, b->right);
        default:
            return false;
    }
}

static Node_t* NodeDuplicate(Node_t* node) {
    if (!node) return NULL;
    
    Node_t* copy = NodeCreate(node->value, NULL);
    if (!copy) return NULL;
    
    if (node->left) {
        copy->left = NodeDuplicate(node->left);
        if (copy->left) copy->left->parent = copy;
    }
    
    if (node->right) {
        copy->right = NodeDuplicate(node->right);
        if (copy->right) copy->right->parent = copy;
    }
    
    return copy;
}

static void ReplaceNode( Node_t** node_ptr, Node_t* new_node ) {
    if ( !node_ptr ) return;
    
    Node_t* old_node = *node_ptr;
    if (old_node && old_node != new_node) {
        // Копируем parent указатель
        if (new_node) {
            new_node->parent = old_node->parent;
        }
        
        // Удаляем старый узел без его потомков
        if (old_node->left) {
            NodeDelete(old_node->left, NULL, NULL);
        }
        if (old_node->right) {
            NodeDelete(old_node->right, NULL, NULL);
        }
        free(old_node);
    }
    
    *node_ptr = new_node;
}

/**
 * Рекурсивно упрощает дерево
 * Возвращает true если произошла оптимизация
 */
static bool SimplifyTreeNode(Node_t* node) {
    if (!node) return false;
    
    bool changed = false;
    
    // Рекурсивно упрощаем поддеревья
    if (node->left) {
        changed |= SimplifyTreeNode(node->left);
    }
    if (node->right) {
        changed |= SimplifyTreeNode(node->right);
    }
    
    if (node->value.type != NODE_OPERATION) {
        return changed;
    }
    
    OperationType op = node->value.data.operation;
    
    // ============ Упрощения для сложения ============
    if (op == OP_ADD) {
        // 0 + x -> x
        if (IsNumber(node->left, 0.0)) {
            Node_t* right_copy = NodeDuplicate(node->right);
            ReplaceNode(&node, right_copy);
            return true;
        }
        // x + 0 -> x
        if (IsNumber(node->right, 0.0)) {
            Node_t* left_copy = NodeDuplicate(node->left);
            ReplaceNode(&node, left_copy);
            return true;
        }
    }
    
    // ============ Упрощения для вычитания ============
    if (op == OP_SUB) {
        // x - 0 -> x
        if (IsNumber(node->right, 0.0)) {
            Node_t* left_copy = NodeDuplicate(node->left);
            ReplaceNode(&node, left_copy);
            return true;
        }
        // x - x -> 0
        if (NodesEqual(node->left, node->right)) {
            TreeData_t zero = {.type = NODE_NUMBER, .data.number = 0.0};
            Node_t* zero_node = NodeCreate(zero, node->parent);
            ReplaceNode(&node, zero_node);
            return true;
        }
    }
    
    // ============ Упрощения для умножения ============
    if (op == OP_MUL) {
        // 1 * x -> x
        if (IsNumber(node->left, 1.0)) {
            Node_t* right_copy = NodeDuplicate(node->right);
            ReplaceNode(&node, right_copy);
            return true;
        }
        // x * 1 -> x
        if (IsNumber(node->right, 1.0)) {
            Node_t* left_copy = NodeDuplicate(node->left);
            ReplaceNode(&node, left_copy);
            return true;
        }
        // 0 * x -> 0
        if (IsNumber(node->left, 0.0)) {
            TreeData_t zero = {.type = NODE_NUMBER, .data.number = 0.0};
            Node_t* zero_node = NodeCreate(zero, node->parent);
            ReplaceNode(&node, zero_node);
            return true;
        }
        // x * 0 -> 0
        if (IsNumber(node->right, 0.0)) {
            TreeData_t zero = {.type = NODE_NUMBER, .data.number = 0.0};
            Node_t* zero_node = NodeCreate(zero, node->parent);
            ReplaceNode(&node, zero_node);
            return true;
        }
    }
    
    if ( op == OP_DIV ) {
        // x / 1 -> x
        if (IsNumber(node->right, 1.0)) {
            Node_t* left_copy = NodeDuplicate(node->left);
            ReplaceNode(&node, left_copy);
            return true;
        }
    }
    
    if (op == OP_POW) {
        // x ^ 0 -> 1
        if (IsNumber(node->right, 0.0)) {
            TreeData_t one = {.type = NODE_NUMBER, .data.number = 1.0};
            Node_t* one_node = NodeCreate(one, node->parent);
            ReplaceNode(&node, one_node);
            return true;
        }
        // x ^ 1 -> x
        if ( IsNumber( node->right, 1.0 ) ) {
            Node_t* left_copy = NodeDuplicate(node->left);
            ReplaceNode(&node, left_copy);
            return true;
        }
        // 1 ^ x -> 1
        if ( IsNumber( node->left, 1.0 ) ) {
            TreeData_t one = {};
            one.type = NODE_NUMBER; 
            one.data.number = 1.0;

            Node_t* one_node = NodeCreate(one, node->parent);
            ReplaceNode( &node, one_node );
            return true;
        }
    }
    
    return changed;
}

bool OptimizeConstants(Tree_t* tree, char independent_var) {
    my_assert( tree, "Null pointer on `tree`" );
    
    if ( !tree->root ) {
        return true;
    }
    
    OptimizeConstantsNode( tree->root, independent_var );
    return true;
}

bool SimplifyTree( Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );
    
    if ( !tree->root ) {
        return true;
    }
    
    bool changed = true;
    int iterations = 0;
    const int max_iterations = 100;
    
    while ( changed && iterations < max_iterations ) {
        changed = SimplifyTreeNode( tree->root );
        iterations++;
    }
    
    if ( iterations >= max_iterations ) {
        PRINT_ERROR( "SimplifyTree: max iterations exceeded\n" );
        return false;
    }
    
    return true;
}

bool OptimizeTree( Tree_t* tree, char independent_var ) {
    my_assert( tree, "Null pointer on `tree`" );
    
    if ( !OptimizeConstants( tree, independent_var ) ) {
        PRINT_ERROR( "OptimizeTree: OptimizeConstants failed\n" );
        return false;
    }
    
    if ( !SimplifyTree( tree ) ) {
        PRINT_ERROR("OptimizeTree: SimplifyTree failed\n");
        return false;
    }
    
    return true;
}
