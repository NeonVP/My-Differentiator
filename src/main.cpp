#include "Differentiator.h"

int main() {
    const char *filename = "expr.txt";

    Differentiator_t *diff = DifferentiatorCtor( filename );

    DifferentiatiorDump( diff, DUMP_ORIGINAL, "After creation expr_tree" );

    DifferentiatorAddOrigExpression( diff, 3 );
    DifferentiatiorDump( diff, DUMP_DIFFERENTIATED, "After optimization" );

    DifferentiatorAddEvaluation( diff, 'x' );

    int extent = 0;
    printf( "To what extent does Taylor count? " );
    scanf( "%d", &extent );
    DifferentiatorAddTaylorSeries( diff, 'x', extent );

    DifferentiatorPlotFunctionAndTaylor( diff, 'x', 250, "tex/plot.png" );

    DifferentiatorDtor( &diff );

    return 0;
}