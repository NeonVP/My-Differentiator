#ifndef OPERATION_H
#define OPERATION_H

enum IsFunction {
    NotFunction = 0,
    Function    = 1
};

enum NumberOfParams {
    ZERO_ARG = 0,
    ONE_ARG  = 1,
    TWO_ARGS = 2
};

// 
#define INIT_OPERATIONS( macros ) \
    macros( "+",       OP_ADD,      0,  NotFunction, ZERO_ARG ) \
    macros( "-",       OP_SUB,      1,  NotFunction, ZERO_ARG ) \
    macros( "*",       OP_MUL,      2,  NotFunction, ZERO_ARG ) \
    macros( "/",       OP_DIV,      3,  NotFunction, ZERO_ARG ) \
    macros( "^",       OP_POW,      4,  NotFunction, ZERO_ARG ) \
    macros( "log",     OP_LOG,      5,  Function, TWO_ARGS ) \
    macros( "sin",     OP_SIN,      6,  Function, ONE_ARG ) \
    macros( "cos",     OP_COS,      7,  Function, ONE_ARG ) \
    macros( "tan",     OP_TAN,      8,  Function, ONE_ARG ) \
    macros( "ctan",    OP_CTAN,     9,  Function, ONE_ARG ) \
    macros( "sh",      OP_SH,       10, Function, ONE_ARG ) \
    macros( "ch",      OP_CH,       11, Function, ONE_ARG ) \
    macros( "arcsin",  OP_ARCSIN,   12, Function, ONE_ARG ) \
    macros( "arccos",  OP_ARCCOS,   13, Function, ONE_ARG ) \
    macros( "arctg",   OP_ARCTAN,   14, Function, ONE_ARG ) \
    macros( "arcctg",  OP_ARCCTAN,  15, Function, ONE_ARG ) \
    macros( "arcsh",   OP_ARSINH,   16, Function, ONE_ARG ) \
    macros( "arcch",   OP_ARCH,     17, Function, ONE_ARG ) \
    macros( "artanh",  OP_ARTANH,   18, Function, ONE_ARG )

#define OPERATIONS_ENUM( str, name, value, ... ) \
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

#endif