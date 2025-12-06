#include <string.h>
#include <ctype.h>

#include "Differentiator.h"
#include "DebugUtils.h"
#include "Tree.h"

static Node_t* GetGrammar( char** cur_pos, Node_t* parent, bool* error );
static Node_t* GetExpression( char** cur_pos, Node_t* parent, bool* error );
static Node_t* GetTerm( char** cur_pos, Node_t* parent, bool* error );
static Node_t* GetPrimary( char** cur_pos, Node_t* parent, bool* error );
static Node_t* GetPow( char** cur_pos, Node_t* parent, bool* error );
static Node_t* GetNumber( char** cur_pos, Node_t* parent, bool* error );
static Node_t* GetFunction( char** cur_pos, Node_t* parent, bool* error );
static Node_t* GetVariable( char** cur_pos, Node_t* parent, bool* error );

static void SkipSpaces( char** position );


Tree_t* ExpressionParser( char* buffer ) {
    my_assert( buffer, "Null pointer on `buffer`" );

    Tree_t* tree = TreeCtor();

    char* current_position = buffer;
    bool  error = false;
    tree->root = GetGrammar( &current_position, NULL, &error );

    if ( error ) {
        PRINT_ERROR( "The database was not considered correct." );
        TreeDtor( &tree, NULL );
        return NULL;
    }
    else {
        PRINT( "The database was considered correct." );
        return tree;
    }
}


#define SyntaxError( string ) \
    PRINT_ERROR( "Syntax error in `%s` %s:%d --- `%s`\n", __func__, __FILE__, __LINE__, *string ); \
    *error = true;

#define DEBUG_PRINT_PARSE PRINT( "\n" ); PRINT( "Current position: `%s`", *cur_pos );

static Node_t* GetGrammar( char **cur_pos, Node_t* parent, bool* error ) {
    my_assert( cur_pos,  "Null pointer on `cur_pos`" );
    my_assert( *cur_pos, "Null pointer on `*cur_pos`" );
    my_assert( error,    "Null pointer on `error`" );

    SkipSpaces( cur_pos );

    Node_t* node = GetExpression( cur_pos, parent, error );

    SkipSpaces( cur_pos );

    if ( **cur_pos != '\0' ) {
        SyntaxError( cur_pos );
    }

    return node;
}

static Node_t* GetExpression( char** cur_pos, Node_t* parent, bool* error ) {
    my_assert( cur_pos,  "Null pointer on `cur_pos`" );
    my_assert( *cur_pos, "Null pointer on `*cur_pos`" );
    my_assert( error,    "Null pointer on `error`" );

    SkipSpaces( cur_pos );

    Node_t* node = GetTerm( cur_pos, parent, error );

    SkipSpaces( cur_pos );

    while ( **cur_pos == '+' || **cur_pos == '-' ) {
        TreeData_t op_value = {};
        op_value.type = NODE_OPERATION;
        op_value.data.operation = ( **cur_pos == '+' ) ? OP_ADD : OP_SUB;
        (*cur_pos)++;

        SkipSpaces( cur_pos );

        Node_t* new_root = NodeCreate( op_value, parent );
        new_root->left = node;
        node->parent = new_root;

        SkipSpaces( cur_pos );

        Node_t* right = GetTerm( cur_pos, new_root, error );
        new_root->right = right;

        node = new_root; 
    }

    return node;
}

static Node_t* GetPow( char** cur_pos, Node_t* parent, bool* error ) {
    my_assert( cur_pos,  "Null pointer on `cur_pos`" );
    my_assert( *cur_pos, "Null pointer on `*cur_pos`" );
    my_assert( error,    "Null pointer on `error`" );

    SkipSpaces( cur_pos );
    
    Node_t* node = GetPrimary( cur_pos, parent, error );

    if ( **cur_pos == '^' ) {
        ( *cur_pos )++;

        TreeData_t op_value = {};
        op_value.type = NODE_OPERATION;
        op_value.data.operation = OP_POW;

        SkipSpaces( cur_pos );

        Node_t* new_root = NodeCreate(op_value, parent);
        new_root->left = node;
        node->parent = new_root;

        SkipSpaces( cur_pos );

        Node_t* right = GetPow( cur_pos, new_root, error );
        new_root->right = right;

        node = new_root;
    }

    return node;
}

static Node_t* GetTerm( char** cur_pos, Node_t* parent, bool* error ) {
    my_assert( cur_pos,  "Null pointer on `cur_pos`" );
    my_assert( *cur_pos, "Null pointer on `*cur_pos`" );
    my_assert( error,    "Null pointer on `error`" );

    SkipSpaces( cur_pos );

    Node_t* node = GetPow( cur_pos, parent, error );

    SkipSpaces( cur_pos );

    while ( **cur_pos == '*' || **cur_pos == '/' ) {
        char op = **cur_pos;
        ( *cur_pos )++;

        TreeData_t op_value = {};
        op_value.type = NODE_OPERATION;
        op_value.data.operation = ( op == '*' ) ? OP_MUL : OP_DIV;

        SkipSpaces( cur_pos );

        Node_t* new_root = NodeCreate(op_value, parent);
        new_root->left = node;
        node->parent = new_root;

        SkipSpaces( cur_pos );

        Node_t* right = GetPow( cur_pos, new_root, error );
        new_root->right = right;

        node = new_root;
    }

    return node;
}

static Node_t* GetPrimary( char** cur_pos, Node_t* parent, bool* error ) {
    my_assert( cur_pos,  "Null pointer on `cur_pos`" );
    my_assert( *cur_pos, "Null pointer on `*cur_pos`" );
    my_assert( error,    "Null pointer on `error`" );

    SkipSpaces( cur_pos );

    DEBUG_PRINT_PARSE;

    Node_t* func = GetFunction( cur_pos, parent, error );
    if ( func )   return func;
    if ( *error ) return NULL;

    if ( **cur_pos == '(' ) {
        ( *cur_pos )++;

        SkipSpaces( cur_pos );

        Node_t* node = GetExpression( cur_pos, parent, error );
        if ( *error ) return NULL;

        SkipSpaces( cur_pos );

        if ( **cur_pos != ')' ) {
            SyntaxError(cur_pos);
            return NULL;
        }
        (*cur_pos)++;
        return node;
    }
    else {
        SkipSpaces( cur_pos );

        Node_t* node = GetVariable( cur_pos, parent, error );
        if ( node ) return node;
        return GetNumber( cur_pos, parent, error );
    }
}

static Node_t* GetNumber( char** cur_pos, Node_t* parent, bool* error ) {
    my_assert( cur_pos,  "Null pointer on `cur_pos`" );
    my_assert( *cur_pos, "Null pointer on `*cur_pos`" );
    my_assert( error,    "Null pointer on `error`" );

    SkipSpaces( cur_pos );

    char* start = *cur_pos;
    char* end   = NULL;
    double val  = strtod( start, &end );

    if ( end == start ) {
        return NULL; 
    }

    *cur_pos = end; 

    TreeData_t value = {};
    value.type = NODE_NUMBER;
    value.data.number = val;

    Node_t* node = NodeCreate( value, parent );
    if ( !node ) {
        *error = true;
        return NULL;
    }

    return node;
}

#define OPERATION_COMPARE( str, name, value, is_function, num_args, ... )    \
    if ( operation == OP_NOPE && is_function == Function &&                  \
        strncmp( *cur_pos, str, strlen( str ) ) == 0 ) {                     \
        operation = name;                                                    \
        func_len  = ( int ) strlen( str );                                   \
        args_count = num_args;                                               \
    }

static Node_t* GetFunction( char** cur_pos, Node_t* parent, bool* error ) {
    my_assert( cur_pos,  "Null pointer on `cur_pos`" );
    my_assert( *cur_pos, "Null pointer on `*cur_pos`" );
    my_assert( error,    "Null pointer on `error`" );

    SkipSpaces( cur_pos );

    OperationType operation = OP_NOPE;
    int func_len   = 0;
    int args_count = 0;

    INIT_OPERATIONS( OPERATION_COMPARE );

    if ( operation == OP_NOPE ) {
        return NULL;
    }

    *cur_pos += func_len;

    TreeData_t val = {};
    val.type = NODE_OPERATION;
    val.data.operation = operation;

    Node_t* func_node = NodeCreate( val, parent );
    if ( !func_node ) {
        *error = true;
        return NULL;
    }

    if ( **cur_pos != '(' ) {
        SyntaxError( cur_pos );
        *error = true;
        NodeDelete( func_node, NULL, NULL );
        return NULL;
    }
    ( *cur_pos )++;

    SkipSpaces( cur_pos );

    if ( args_count == ONE_ARG || args_count == TWO_ARGS ) {
        Node_t* arg1 = GetExpression( cur_pos, func_node, error );
        if ( *error || !arg1 ) {
            NodeDelete( func_node, NULL, NULL );
            return NULL;
        }

        func_node->left = arg1;
        arg1->parent    = func_node;

        if ( args_count == TWO_ARGS ) {
            if (**cur_pos != ',') {
                SyntaxError( cur_pos );
                *error = true;
                NodeDelete( func_node, NULL, NULL );
                return NULL;
            }
            ( *cur_pos )++;

            Node_t* arg2 = GetExpression( cur_pos, func_node, error );
            if ( *error || !arg2 ) {
                NodeDelete( func_node, NULL, NULL );
                return NULL;
            }

            func_node->right = arg2;
            arg2->parent     = func_node;
        }
    }

    if ( **cur_pos != ')' ) {
        SyntaxError( cur_pos );
        *error = true;
        NodeDelete( func_node, NULL, NULL );
        return NULL;
    }
    ( *cur_pos )++;

    return func_node;
}

#undef OPERATION_COMPARE

static Node_t* GetVariable(char** cur_pos, Node_t* parent, bool* error) {
    my_assert(cur_pos,  "Null pointer on `cur_pos`");
    my_assert(*cur_pos, "Null pointer on `*cur_pos`");
    my_assert(error,    "Null pointer on `error`");

    if ( isalpha( **cur_pos ) ) {
        TreeData_t value = {};
        value.type = NODE_VARIABLE;
        value.data.variable = **cur_pos;

        ( *cur_pos )++;

        Node_t* node = NodeCreate( value, parent );
        if ( !node ) {
            *error = true;
            return NULL;
        }

        return node;
    }

    return NULL;
}

static void SkipSpaces( char** position ) {
    while ( isspace( **position ) ) 
        ( *position )++;
}