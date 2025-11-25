#include "Differentiator.h"

int main() {
    Tree_t* tree = TreeCtor();

    TreeReadFromFile( tree, "base.txt" );
    // TreeSaveToFile( tree, "base.txt" );

    // printf( "Результат: %lg \n", EvaluateTree( tree ) );

    Tree_t* d_tree = DifferentiateTree( tree, 'x' );

    TreeDtor( &tree, NULL );
    TreeDtor( &d_tree, NULL );
}