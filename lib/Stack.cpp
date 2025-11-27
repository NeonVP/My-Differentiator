#include <cassert>
#include <stdint.h>

#include "Stack.h"

Stack_t* StackCtor( size_t capacity ) {
    Stack_t* stk = ( Stack_t* ) calloc ( 1, sizeof( *stk ) );
    assert( stk && "Memory allocation error for `stk`" );

    ON_DEBUG(
        if ( capacity > large_capacity ) {
            stk->varInfo.err_code |= ERR_LARGE_CAPACITY;
        }
    )

    stk->data = ( StackData_t* ) calloc ( capacity ON_CANARY( + 2 ), sizeof( *( stk->data ) ) );
    assert( stk->data && "Memory allocation error for `stk->data`" );

    stk->capacity = capacity;
    stk->size     = 0;

    #ifdef _CANARY
        *( stk->data ) = canary;
        *( stk->data + stk->capacity + 1 ) = canary;

        stk->data++;
    #endif

    StackToPoison( stk );

    return stk;
}

void StackDtor( Stack_t** stk ) {
    ON_CANARY( stk->data--; )

    free( ( *stk )->data );
    ( *stk )->data     = NULL;
    
    free( *stk );
    *stk = NULL;
}

void StackPush( Stack_t* stk, StackData_t element ) {
    DEBUG_IN_FUNC(
        if ( stk->size == stk->capacity ) {
            StackRealloc( stk, stk->capacity * 2 );
        }

        *( stk->data + stk->size ) = element;
        stk->size++;
    )
}

void StackPop( Stack_t* stk, StackElemDtor dtor ) {
    if ( StackVerify( stk ) != ERR_NONE ) {
        return;
    }

    if ( stk->size == 0 ) {
        return;
    }

    stk->size--;

    if ( dtor != NULL ) {
        dtor( *( stk->data + stk->size ) );
    }

    *( stk->data + stk->size ) = poison;

    if ( stk->size * 4 <= stk->capacity ) {
        StackRealloc( stk, stk->capacity / 2 );
    }

    ON_DEBUG(
        if ( StackVerify( stk ) != ERR_NONE ) {
            return;
        }
    )
}


void StackRealloc( Stack_t* stk, size_t capacity ) {
    DEBUG_IN_FUNC(
        stk->capacity = capacity;
        ON_CANARY( stk->data--; )

        StackData_t* new_ptr = ( StackData_t* ) realloc ( stk->data, ( capacity ON_CANARY( + 2 ) ) * sizeof( *new_ptr ) );
        if ( new_ptr == NULL ) {
            free( stk->data );
            assert( new_ptr != NULL );
        }
        stk->data = new_ptr;


        #ifdef _CANARY
            *( stk->data ) = canary;
            *( stk->data + stk->capacity + 1 ) = canary;

            stk->data++;
        #endif

        StackToPoison( stk );
    )
}

void StackToPoison( Stack_t* stk ) {
    DEBUG_IN_FUNC(
        for ( size_t i = stk->size; i < stk->capacity; i++ ) {
            *( stk->data + i ) = poison;
        }
    )
}

#ifdef _DEBUG
long StackVerify( Stack_t* stk ) {
    if ( stk == NULL ) {
        stk->varInfo.err_code |= ERR_BAD_PTR_STRUCT;
    }

    if ( stk->data == NULL ) {
        stk->varInfo.err_code |= ERR_BAD_PTR_DATA;
    }

    #ifdef _CANARY
    if ( stk->data != NULL ) {
        if ( *( stk->data - 1 ) != canary || *( stk->data + stk->capacity ) != canary ) {
            stk->varInfo.err_code |= ERR_CORRUPTED_CANARY_DATA;
        }

        if ( stk->canary1 != canary || stk->canary2 != canary ) {
            stk->varInfo.err_code |= ERR_CORRUPTED_CANARY_STRUCT;
        }
    }
    #endif

    if ( stk->capacity < stk->size ) {
        stk->varInfo.err_code |= ERR_SIZE_OVER_CAPACITY;
    }
    else if ( stk->data != NULL ) {
        for ( size_t i = 0; i < stk->size; i++ ) {
            if ( *( stk->data + i ) == poison ) {
                stk->varInfo.err_code |= ERR_POISON_IN_FILLED_CELLS;
                break;
            }
        }
    }

    if ( stk->capacity > large_capacity ) {
        stk->varInfo.err_code |= ERR_LARGE_CAPACITY;
    }

    if ( stk->varInfo.err_code != ERR_NONE ) {
        StackDump( stk );
    }

    return stk->varInfo.err_code;
}

struct ErrMsg {
    long        code;
    const char* msg;
};

static const ErrMsg ERR_MSGS[] = {
    { ERR_BAD_PTR_STRUCT,          "\tERROR: INVALID pointer to the structure with parameters  \n" },
    { ERR_BAD_PTR_DATA,            "\tERROR: INVALID pointer to the stack \n" },
    { ERR_CORRUPTED_CANARY_DATA,   "\tERROR: Corrupted canaries in data \n" },
    { ERR_CORRUPTED_CANARY_STRUCT, "\tERROR: Corrupted canaries in struct of stack \n" },
    { ERR_SIZE_OVER_CAPACITY,      "\tERROR: Size over capacity \n" },
    { ERR_POISON_IN_FILLED_CELLS,  "\tERROR: Poison in filled cells \n" },
    { ERR_LARGE_CAPACITY,          "\tWARNING: Too big capacity \n" }
};

static const size_t ERR_MSGS_COUNT = sizeof( ERR_MSGS ) / sizeof( ERR_MSGS[ 0 ] );

void ErrorProcessing( long err_code ) {
    for ( size_t i = 0; i < ERR_MSGS_COUNT; i++ ) {
        if ( err_code & ERR_MSGS[ i ].code ) {
            printerr( ERR_MSGS[ i ].msg );
        }
    }
}

void StackDump( Stack_t* stk, FILE* out ) {
    if ( out == NULL ) {
        out = stderr;
    }

    if ( stk == NULL ) {
        fprintf( out, COLOR_RED "Error in DUMP: Null Pointer on stack STRUCT\n" COLOR_RESET );
        return;
    }

    fprintf( out, COLOR_YELLOW "stack<int>[%p]" 
                 ON_DEBUG( " --- name: \"%s\" --- FILE: %s --- LINE: %lu --- FUNC: %s" )
                 "\n" COLOR_RESET,
                 stk
                 ON_DEBUG( , stk->varInfo.name, stk->varInfo.file, stk->varInfo.line, stk->varInfo.func ) );

    ON_DEBUG(
        if ( stk->varInfo.err_code != ERR_NONE ) {
            fprintf( out, COLOR_RED "  ERROR CODE %ld:\n" COLOR_RESET, stk->varInfo.err_code );
            ErrorProcessing( stk->varInfo.err_code );
        }
    )

    fprintf( out, COLOR_CYAN
                    "  {              \n"
                    "  capacity = %lu\n"
                    "  size     = %lu\n"
                    "  data     = %p \n"
                    COLOR_RESET,
                    stk->capacity,
                    stk->size,
                    stk->data );

    if ( stk->data == NULL || stk->size > stk->capacity ) {
        PASS;
    }
    else {
        fprintf( out, "\t{\n" );

        for ( size_t idx = 0; idx < stk->capacity; idx++ ) {
            if ( stk->data[ idx ] != poison ) {
                fprintf( out,
                         "\t*[%lu] = %lX\n",
                         idx, ( uintptr_t ) stk->data[ idx ] );
            }
            else {
                fprintf( out,
                         "\t [%lu] = %lX ( poison )\n",
                         idx, ( uintptr_t ) stk->data[ idx ] );
            }
        }

        fprintf( out, "\t}\n" );
    }

    fprintf( out, COLOR_CYAN "  }\n" COLOR_RESET );
}
#endif
