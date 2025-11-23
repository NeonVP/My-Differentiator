#include "Expression.h"

int main() {
    Tree_t* tree = TreeCtor();

    TreeReadFromFile( tree, "base.txt" );
    // TreeSaveToFile( tree, "base.txt" );

    printf( "Результат: %lg \n", EvaluateTree( tree ) );

    TreeDtor( &tree, NULL );
}