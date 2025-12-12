#include <assert.h>

#include "Differentiator.h"

int main() {
    const char *filename = "expr.txt";

    Differentiator_t *diff = DifferentiatorCtor( filename );

    DifferentiatiorDump( diff, DUMP_ORIGINAL, "After creation expr_tree" );

    DifferentiatorAddOrigExpression( diff, 3 );
    DifferentiatiorDump( diff, DUMP_DIFFERENTIATED, "After optimization" );

    DifferentiatorAddEvaluation( diff, 'x' );
    DifferentiatorAddTaylorSeries( diff, 'x', 7 );

    // TODO: убрать поля дифференциатора как аргумента (передевать только дифференциатор)
    DifferentiatorPlotFunctionAndTaylor( diff, 'x', diff->plot_x_min, diff->plot_x_max, diff->plot_y_min,
                                         diff->plot_y_max, 250, "tex/plot.png" );

    DifferentiatorDtor( &diff );
    
    return 0;
}