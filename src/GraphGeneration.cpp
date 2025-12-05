#include "DebugUtils.h"
#include "Differentiator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void DifferentiatorPlotFunctionAndTaylor( Differentiator_t *diff, char var,
                                          double x_min, double x_max,
                                          int n_points,
                                          const char *output_image ) {
    my_assert( diff, "Null pointer on `diff`" );
    my_assert( output_image, "Null pointer on `output_image`" );

    const char *func_data_file = "func_data.tmp";
    const char *taylor_data_file = "taylor_data.tmp";

    FILE *f_func = fopen( func_data_file, "w" );
    FILE *f_taylor = fopen( taylor_data_file, "w" );
    if ( !f_func || !f_taylor ) {
        perror( "Не удалось создать временный файл" ); // Better use Russian / English only
        return;
    }

    double step = ( x_max - x_min ) / ( n_points - 1 );
    for ( int i = 0; i < n_points; i++ ) {
        double x = x_min + i * step;

        // Обновляем переменную в таблице
        VarTableSet( &diff->var_table, var, x );

        double y_func = EvaluateTree( diff->expr_tree, diff );
        double y_taylor = EvaluateTree( diff->taylor_tree, diff );

        fprintf( f_func, "%.10g %.10g\n", x, y_func );
        fprintf( f_taylor, "%.10g %.10g\n", x, y_taylor );
    }

    fclose( f_func );
    fclose( f_taylor );

    // Создаем скрипт для Gnuplot
    const char *gnuplot_script = "plot_script.gp";
    FILE *f_gp = fopen( gnuplot_script, "w" );
    if ( !f_gp ) {
        perror( "Не удалось создать скрипт Gnuplot" );
        return;
    }

    fprintf( f_gp,
             "set terminal pngcairo enhanced size 800,600\n"
             "set output '%s'\n"
             "set title 'Функция и ряд Тейлора'\n"
             "set xlabel '%c'\n"
             "set ylabel 'f(x)'\n"
             "set grid\n"
             "plot '%s' with lines lw 2 lc rgb 'blue' title 'Функция', \\\n"
             "     '%s' with lines lw 2 lc rgb 'red' title 'Ряд Тейлора'\n",
             output_image, var, func_data_file, taylor_data_file );

    fclose( f_gp );

    char cmd[512];
    snprintf( cmd, sizeof( cmd ), "gnuplot %s -o tex/plot.png",
              gnuplot_script );
    system( cmd );

    remove( func_data_file );
    remove( taylor_data_file );
    remove( gnuplot_script );
}
