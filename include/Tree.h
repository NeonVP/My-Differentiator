#include <stdio.h>

#ifndef TREE_H
#define TREE_H

#ifdef _LINUX
#include <linux/limits.h>
const size_t MAX_LEN_PATH = PATH_MAX;
#else
const size_t MAX_LEN_PATH = 256;
#endif

#define INIT_OPERATIONS( macros ) \
    macros( '+', OP_ADD, 0 ) \
    macros( '-', OP_SUB, 1 ) \
    macros( '*', OP_MUL, 2 ) \
    macros( '/', OP_DIV, 3 ) \
    macros( '^', OP_POW, 4 )

#define INIT_OP_ENUM( str, name, value ) \
    name = value,

enum NodeType {
    NODE_UNKNOWN = -1,

    NODE_NUMBER    = 0,
    NODE_VARIABLE  = 1,
    NODE_OPERATION = 2
};

enum OperationType {
    OP_NOPE = -1,

    INIT_OPERATIONS( INIT_OP_ENUM )
};

struct NodeValue {
    enum NodeType type;

    union {
        double number; 
        char   variable; 
        char   operation; 
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

enum TreeStatus_t {
    SUCCESS = 0,
    FAIL    = 1
};

enum DirectionType {
    RIGHT = 0,
    LEFT  = 1
};

Tree_t* TreeCtor();
void    TreeDtor( Tree_t** tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

void TreeSaveToFile( const Tree_t* tree, const char* filename );
void TreeReadFromFile    ( Tree_t* tree, const char* filename );

Node_t* NodeCreate( const TreeData_t field, Node_t* parent );
void    NodeDelete( Node_t* node, Tree_t* tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

void TreeDump( Tree_t* tree, const char* format_string, ... );
void NodeGraphicDump( const Node_t* node, const char* image_path_name, ... );

#endif//TREE_H