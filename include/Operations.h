#ifndef OPERATION_H
#define OPERATION_H

enum IsFunction { // TODO: NonCustomOp, CustomOp
    NotFunction = 0,
    Function    = 1
};

enum NumberOfParams {
    ZERO_ARG = 0,
    ONE_ARG  = 1,
    TWO_ARGS = 2
};

// TODO: add joke_lines here
#define INIT_OPERATIONS( macros ) \
    macros( "+",       OP_ADD,     0,   NotFunction,  ZERO_ARG,  "%e \n+ %e"                  ) \
    macros( "-",       OP_SUB,     1,   NotFunction,  ZERO_ARG,  "%e \n- %e"                  ) \
    macros( "*",       OP_MUL,     2,   NotFunction,  ZERO_ARG,  "%e \n\\cdot %e"             ) \
    macros( "/",       OP_DIV,     3,   NotFunction,  ZERO_ARG,  "\\frac{%e}{%e}"             ) \
    macros( "^",       OP_POW,     4,   NotFunction,  ZERO_ARG,  "%e ^ {%e}"                  ) \
    macros( "log",     OP_LOG,     5,   Function,     TWO_ARGS,  "\\log_{%e}{%e}"             ) \
    macros( "sin",     OP_SIN,     6,   Function,     ONE_ARG,   "\\sin %e "                  ) \
    macros( "cos",     OP_COS,     7,   Function,     ONE_ARG,   "\\cos %e "                  ) \
    macros( "tg",      OP_TAN,     8,   Function,     ONE_ARG,   "\\tan %e "                  ) \
    macros( "ctg",     OP_CTAN,    9,   Function,     ONE_ARG,   "\\cot %e "                  ) \
    macros( "sh",      OP_SH,      10,  Function,     ONE_ARG,   "\\sinh %e "                 ) \
    macros( "ch",      OP_CH,      11,  Function,     ONE_ARG,   "\\cosh %e)"                 ) \
    macros( "arcsin",  OP_ARCSIN,  12,  Function,     ONE_ARG,   "\\arcsin %e "               ) \
    macros( "arccos",  OP_ARCCOS,  13,  Function,     ONE_ARG,   "\\arccos %e "               ) \
    macros( "arctg",   OP_ARCTAN,  14,  Function,     ONE_ARG,   "\\arctan %e "               ) \
    macros( "arcctg",  OP_ARCCTAN, 15,  Function,     ONE_ARG,   "\\arccot %e "               ) \
    macros( "arcsh",   OP_ARSINH,  16,  Function,     ONE_ARG,   "\\operatorname{arsinh} %e " ) \
    macros( "arcch",   OP_ARCH,    17,  Function,     ONE_ARG,   "\\operatorname{arccosh} %e ") \
    macros( "arcth",   OP_ARTANH,  18,  Function,     ONE_ARG,   "\\operatorname{arctanh} %e ") \
    macros( "ln",      OP_LN,      19,  Function,     ONE_ARG,   "\\ln %e "                   )

#endif