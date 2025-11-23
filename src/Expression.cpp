// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #include "Expression.h"

// Node_t* NodeCreateNumber(double value, Node_t* parent) {
//     Node_t* node = ( Node_t* ) calloc ( 1, sizeof( *node ) );
    
//     node->parent = parent;

//     node->value.type  = EXPR_NUMBER;
//     node->value.data.number = value;

//     return node;
// }

// Node_t* NodeCreateVariable( const char* name, Node_t* parent ) {
//     Node_t* node = (Node_t*)calloc(1, sizeof(Node_t));
//     node->parent      = parent;

//     node->value.type  = EXPR_VARIABLE;
//     node->value.data.var_name = strdup(name);

//     return node;
// }

// Node_t* NodeCreateOperator(OpType op, Node_t* parent) {
//     Node_t* node = (Node_t*) calloc (1, sizeof(Node_t));
//     node->parent      = parent;

//     node->value.type  = EXPR_OPERATOR;
//     node->value.data.op = op;

//     return node;
// }

// void CleanExprValue(ExprValue_t* value) {
//     if ( value->type == EXPR_VARIABLE ) {
//         free( value->data.var_name );
//     }
// }