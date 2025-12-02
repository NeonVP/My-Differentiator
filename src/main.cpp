#include "Differentiator.h"

int main() {
    const char* filename = "expr.txt";
    Differentiator_t* diff = DifferentiatorCtor( filename );
    DifferentiatiorDump( diff, DUMP_ORIGINAL, "After creation expr_tree" );

    OptimizeTree( diff->expr_tree, diff, 'x' );
    DifferentiatiorDump( diff, DUMP_ORIGINAL, "After optimization" );
    // printf( "Результат: %lg \n", EvaluateTree( diff->expr_tree ) );

    // DifferentiateExpression( diff, 'x', 1 );
    // DifferentiatiorDump( diff, DUMP_DIFFERENTIATED, "After differentiation" );

    DifferentiatorDtor( &diff );
}