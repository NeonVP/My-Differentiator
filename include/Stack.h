#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "Colors.h"

typedef int StackData_t;

#define PASS
#define printerr(str) fprintf( stderr, COLOR_RED "%s" COLOR_RESET, str );

#ifdef _CANARY
const int canary = 0xbabe;

#define ON_CANARY(...) __VA_ARGS__
#else
#define ON_CANARY(...)
#endif

#ifdef _DEBUG
#define ON_DEBUG(...) __VA_ARGS__

#define CHECK_FOR_ERR( stk ) if ( IS_CRITICAL_ERROR( stk.varInfo.err_code ) ) { StackDtor( &stk ); return 1; } else { stk.varInfo.err_code = ERR_NONE; }

#define INIT( stk ) \
    ON_CANARY( canary, ) 0, 0, 0, { ERR_NONE, #stk, __func__, __LINE__, __FILE__ } ON_CANARY( , canary )

#define IS_CRITICAL_ERROR( code ) ( code & ( ERR_SIZE_OVER_CAPACITY | ERR_BAD_PTR_DATA ) )

#define DEBUG_IN_FUNC(...)                             \
    if ( StackVerify( stk ) != ERR_NONE ) return;      \
    __VA_ARGS__                                        \
   StackVerify( stk );
#else
#define ON_DEBUG(...)

#define CHECK_FOR_ERR( code )

#define INIT( stk ) \
    ON_CANARY( canary, ) 0, 0, 0 ON_CANARY( , canary )

#define DEBUG_IN_FUNC(...) __VA_ARGS__
#endif //_DEBUG

const int poison = 777;
const int large_capacity = 10000000;

enum ErrStack_t {
    ERR_NONE                    = 0,
    ERR_BAD_PTR_STRUCT          = 1 << 1,
    ERR_BAD_PTR_DATA            = 1 << 2,
    ERR_CORRUPTED_CANARY_DATA   = 1 << 3,
    ERR_CORRUPTED_CANARY_STRUCT = 1 << 4,
    ERR_SIZE_OVER_CAPACITY      = 1 << 5,
    ERR_LARGE_CAPACITY          = 1 << 6,
    ERR_POISON_IN_FILLED_CELLS  = 1 << 7
};

struct VarInfo {
    long        err_code = ERR_NONE;
    const char* name     = 0;
    const char* func     = 0;
    size_t      line     = 0;
    const char* file     = 0;
};

struct Stack_t {
    ON_CANARY( int canary1 = canary; )

    StackData_t* data = 0;
    size_t size       = 0;
    size_t capacity   = 0;

    ON_DEBUG( VarInfo varInfo = {}; )
    ON_CANARY( int canary2 = canary; )
};

void         StackCtor( Stack_t* stk, size_t size );
void         StackDtor( Stack_t* stk );
void         StackPush( Stack_t* stk, int element );
StackData_t  StackPop ( Stack_t* stk );

void StackRealloc( Stack_t* stk, size_t capacity );
void StackToPoison( Stack_t* stk );

long StackVerify( Stack_t* stk );
void StackDump  ( Stack_t* stk );
void ErrorProcessing( long err_code );

#endif //STACK_H