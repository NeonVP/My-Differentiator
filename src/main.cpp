#include <assert.h>

#include "Differentiator.h"

int main() {
    const char *filename = "expr.txt";

    Differentiator_t *diff = DifferentiatorCtor( filename );
    assert( diff && diff->expr_tree && diff->expr_tree->root && "Tree is empty!" );

    DifferentiatiorDump( diff, DUMP_ORIGINAL, "After creation expr_tree" );
    DifferentiatorAddOrigExpression( diff, 1 );

    DifferentiatiorDump( diff, DUMP_DIFFERENTIATED, "After optimization" );

    DifferentiatorAddEvaluation( diff, 'x' );
    DifferentiatorAddTaylorSeries( diff, 'x', 3 );

    DifferentiatorPlotFunctionAndTaylor( diff, 'x', -5.0, 5.0, 200, "plot.png" );

    DifferentiatorDtor( &diff );

    return 0;
}