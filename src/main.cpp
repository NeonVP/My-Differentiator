#include "Expression.h"

int main() {
    Tree_t* tree = TreeCtor();

    TreeReadFromFile( tree, "base.txt" );
    TreeSaveToFile( tree, "base.txt" );

    TreeDtor( &tree, NULL );
}