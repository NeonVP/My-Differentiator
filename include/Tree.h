#ifndef TREE_H
#define TREE_H

#include <stdio.h>

#ifdef _LINUX
#include <linux/limits.h>
const size_t MAX_LEN_PATH = PATH_MAX;
#else
const size_t MAX_LEN_PATH = 256;
#endif

#define INIT_OPERATIONS( macros ) \
    macros( "+",   OP_ADD, 0 ) \
    macros( "-",   OP_SUB, 1 ) \
    macros( "*",   OP_MUL, 2 ) \
    macros( "/",   OP_DIV, 3 ) \
    macros( "^",   OP_POW, 4 ) \
    macros( "log", OP_LOG, 5 ) \
    macros( "sin", OP_SIN, 6 ) \
    macros( "cos", OP_COS, 7 ) \
    macros( "tan", OP_TAN, 8 )

#define OPERATIONS_ENUM( str, name, value ) \
    name = value,
    
#define OPERATIONS_STRINGS( string, ... ) \
    string,

enum NodeType {
    NODE_UNKNOWN = -1,

    NODE_NUMBER    = 0,
    NODE_VARIABLE  = 1,
    NODE_OPERATION = 2
};

enum OperationType {
    OP_NOPE = -1,

    INIT_OPERATIONS( OPERATIONS_ENUM )
};

static const char* operations_txt[] = { INIT_OPERATIONS( OPERATIONS_STRINGS ) };

#undef INIT_OP_ENUM
#undef OPERATIONS_STRINGS

struct NodeValue {
    enum NodeType type;

    union {
        double number; 
        char   variable; 
        int    operation; 
    } data;
};

typedef NodeValue TreeData_t;

struct Node_t {
    TreeData_t value;

    Node_t* right;
    Node_t* left;

    Node_t* parent;
};

struct Tree_t {
    Node_t* root;

    /* TODO: from Egor
    I think that it's bad idea to store buffer with initial expression
    in Tree struct. You should make Differentiator struct, that will contain:
    Tree_t *tree;
    ExprInfo_t *expr_info;
    e.t.c
    because tree shouldn't provide any functional that implements any interaction with
    initial expression, but differentiator should.
    */
    char* buffer;
    char* current_position;
    off_t buffer_size;

#ifdef _DEBUG
    struct Log_t {
        FILE* log_file;
        char* log_path;
        char* img_log_path;
    } logging;

    size_t image_number;
#endif
};

Tree_t* TreeCtor();
void    TreeDtor( Tree_t** tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

void TreeSaveToFile( const Tree_t* tree, const char* filename );
void TreeReadFromFile    ( Tree_t* tree, const char* filename );

Node_t* NodeCreate( const TreeData_t field, Node_t* parent );
void    NodeDelete( Node_t* node, Tree_t* tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

Node_t* NodeLeftCreate ( const TreeData_t field, Node_t* parent );
Node_t* NodeRightCreate( const TreeData_t field, Node_t* parent );

Node_t* NodeCopy( Node_t* node );

void TreeDump( Tree_t* tree, const char* format_string, ... );
void NodeGraphicDump( const Node_t* node, const char* image_path_name, ... );

#endif//TREE_H