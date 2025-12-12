#include "DebugUtils.h"
#include "Differentiator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static bool GeneratePlotData( Differentiator_t *diff, char var, double x_min, double x_max, int n_points,
                              double *out_y_min, double *out_y_max ) {
    const char *func_data_file = "func_data.tmp";
    const char *taylor_data_file = "taylor_data.tmp";

    FILE *f_func = fopen( func_data_file, "w" );
    FILE *f_taylor = fopen( taylor_data_file, "w" );
    if ( !f_func || !f_taylor ) {
        if ( f_func )
            fclose( f_func );
        if ( f_taylor )
            fclose( f_taylor );
        perror( "Failed to create data files" );
        return false;
    }

    double computed_y_min = INFINITY;
    double computed_y_max = -INFINITY;
    double step = ( x_max - x_min ) / ( n_points - 1 );

    for ( int i = 0; i < n_points; i++ ) {
        double x = x_min + i * step;
        VarTableSet( &diff->var_table, var, x );

        double y_func = EvaluateTree( diff->expr_tree, diff );
        double y_taylor = EvaluateTree( diff->taylor_tree, diff );

        if ( isfinite( y_func ) ) {
            fprintf( f_func, "%.10g %.10g\n", x, y_func );
            if ( y_func < computed_y_min )
                computed_y_min = y_func;
            if ( y_func > computed_y_max )
                computed_y_max = y_func;
        } else {
            fprintf( f_func, "%.10g NaN\n", x );
        }

        if ( isfinite( y_taylor ) ) {
            fprintf( f_taylor, "%.10g %.10g\n", x, y_taylor );
            if ( y_taylor < computed_y_min )
                computed_y_min = y_taylor;
            if ( y_taylor > computed_y_max )
                computed_y_max = y_taylor;
        } else {
            fprintf( f_taylor, "%.10g NaN\n", x );
        }
    }

    fclose( f_func );
    fclose( f_taylor );

    *out_y_min = computed_y_min;
    *out_y_max = computed_y_max;
    return true;
}

static void WriteTangentPoint( double x0, double y0 ) {
    const char *tangent_point_file = "tangent_point.tmp";
    FILE *f = fopen( tangent_point_file, "w" );
    if ( f ) {
        if ( isfinite( x0 ) && isfinite( y0 ) ) {
            fprintf( f, "%.10g %.10g\n", x0, y0 );
        }
        fclose( f );
    }
}

static void DetermineYRange( double user_y_min, double user_y_max, double computed_y_min,
                             double computed_y_max, double *final_y_min, double *final_y_max ) {
    bool auto_y = ( !isfinite( user_y_min ) || !isfinite( user_y_max ) );

    if ( auto_y ) {
        if ( !isfinite( computed_y_min ) || !isfinite( computed_y_max ) ) {
            *final_y_min = -1.0;
            *final_y_max = 1.0;
        } else {
            double y_range = computed_y_max - computed_y_min;
            if ( y_range < 1e-10 )
                y_range = 1.0;
            *final_y_min = computed_y_min - 0.1 * y_range;
            *final_y_max = computed_y_max + 0.1 * y_range;
        }
    } else {
        *final_y_min = user_y_min;
        *final_y_max = user_y_max;
    }
}

static bool WriteGnuplotScript( const char *output_image, char var, double x_min, double x_max, double y_min,
                                double y_max, double f_x0, double f_prime_x0, double x0 ) {
    const char *gnuplot_script = "plot_script.gp";
    FILE *f_gp = fopen( gnuplot_script, "w" );
    if ( !f_gp ) {
        perror( "Failed to create Gnuplot script" );
        return false;
    }

    fprintf(
        f_gp,
        "set terminal pngcairo enhanced size 800,600\n"
        "set output '%s'\n"
        "set title 'Функция, ряд Тейлора и касательная'\n"
        "set xlabel '%c'\n"
        "set ylabel 'f(x)'\n"
        "set xrange [%.10g:%.10g]\n"
        "set yrange [%.10g:%.10g]\n"
        "set grid\n"
        "f_tangent(x) = %.10g + %.10g * (x - %.10g)\n"
        "plot "
        "'func_data.tmp' with lines lw 2 lc rgb 'blue'   title 'Функция', \\\n"
        "     'taylor_data.tmp' with lines lw 2 lc rgb 'red'    title 'Ряд Тейлора', \\\n"
        "     f_tangent(x) with lines lw 2 lc rgb 'green' title 'Касательная', \\\n"
        "     'tangent_point.tmp' using 1:2 with points pt 7 ps 2 lc rgb 'black' title 'Точка касания'\n",
        output_image, var, x_min, x_max, y_min, y_max, f_x0, f_prime_x0, x0 );

    fclose( f_gp );
    return true;
}

static void CleanupTempFiles( void ) {
    remove( "func_data.tmp" );
    remove( "taylor_data.tmp" );
    remove( "tangent_point.tmp" );
    remove( "plot_script.gp" );
}

void DifferentiatorPlotFunctionAndTaylor( Differentiator_t *diff, char var,
                                          double x_min, double x_max,
                                          double y_min, double y_max, 
                                          int n_points,
                                          const char *output_image ) {
    my_assert( diff, "Null pointer on `diff`" );
    my_assert( output_image, "Null pointer on `output_image`" );
    my_assert( n_points > 1, "n_points must be > 1" );

    PRINT( "y_min = %g; y_max = %g", y_min, y_max );
    PRINT( "x_min = %g; x_max = %g", x_min, x_max );

    double x0 = 0.0;
    if ( !VarTableGet( &diff->var_table, var, &x0 ) ) {
        printf( "Warning: variable '%c' not found, using x0 = 0\n", var );
        x0 = 0.0;
    }

    PRINT( "x_0 = %g", x0 );

    VarTableSet( &diff->var_table, var, x0 );
    double f_x0 = EvaluateTree( diff->expr_tree, diff );
    DifferentiateExpression( diff, var, 1 );
    double f_prime_x0 = EvaluateTree( diff->diff_tree, diff );

    WriteTangentPoint( x0, f_x0 );

    double computed_y_min = 0, computed_y_max = 0;
    if ( !GeneratePlotData( diff, var, x_min, x_max, n_points, &computed_y_min, &computed_y_max ) ) {
        CleanupTempFiles();
        return;
    }

    double final_y_min = 0, final_y_max = 0;
    DetermineYRange( y_min, y_max, computed_y_min, computed_y_max, &final_y_min, &final_y_max );

    if ( !WriteGnuplotScript( output_image, var, x_min, x_max, final_y_min, final_y_max, f_x0, f_prime_x0,
                              x0 ) ) {
        CleanupTempFiles();
        return;
    }

    char cmd[512];
    snprintf( cmd, sizeof( cmd ), "gnuplot plot_script.gp" );
    int sys_result = system( cmd );
    (void)sys_result;

    CleanupTempFiles();
}
