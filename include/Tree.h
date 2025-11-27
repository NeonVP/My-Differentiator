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
    macros( "+",       OP_ADD,      0 ) \
    macros( "-",       OP_SUB,      1 ) \
    macros( "*",       OP_MUL,      2 ) \
    macros( "/",       OP_DIV,      3 ) \
    macros( "^",       OP_POW,      4 ) \
    macros( "log",     OP_LOG,      5 ) \
    macros( "sin",     OP_SIN,      6 ) \
    macros( "cos",     OP_COS,      7 ) \
    macros( "tan",     OP_TAN,      8 ) \
    macros( "ctan",    OP_CTAN,     9 ) \
    macros( "sh",      OP_SH,       10 ) \
    macros( "ch",      OP_CH,       11 ) \
    macros( "arcsin",  OP_ARCSIN,   12 ) \
    macros( "arccos",  OP_ARCCOS,   13 ) \
    macros( "arctg",   OP_ARCTAN,   14 ) \
    macros( "arcctg",  OP_ARCCTAN,  15 ) \
    macros( "arcsh",   OP_ARSINH,   16 ) \
    macros( "arcch",   OP_ARCH,     17 ) \
    macros( "artanh",  OP_ARTANH,   18 )

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
};

Tree_t* TreeCtor();
void    TreeDtor( Tree_t** tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

void TreeSaveToFile( const Tree_t* tree, const char* filename );
Tree_t* TreeReadFromBuffer( char* buffer );

Node_t* NodeCreate( const TreeData_t field, Node_t* parent );
void    NodeDelete( Node_t* node, Tree_t* tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

Node_t* NodeLeftCreate ( const TreeData_t field, Node_t* parent );
Node_t* NodeRightCreate( const TreeData_t field, Node_t* parent );

Node_t* NodeCopy( Node_t* node );

// void TreeDump( Tree_t* tree, const char* format_string, ... );
void NodeGraphicDump( const Node_t* node, const char* image_path_name, ... );

#endif//TREE_H