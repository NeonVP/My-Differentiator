#include "Differentiator.h"
#include "DebugUtils.h"
#include "Tree.h"

static Node_t* DifferentiateNode( Node_t* node, char independent_variable );

static TreeData_t MakeNumber( double number );
static TreeData_t MakeOperation( OperationType operation );
static TreeData_t MakeVariable( char variable );

Tree_t* DifferentiateTree( Tree_t* tree, char independent_variable ) {
    my_assert( tree, "Null pointer on `tree`" );

    Tree_t* differentiable_tree = TreeCtor();

#ifdef _DEBUG
    differentiable_tree->logging.log_file = tree->logging.log_file;
    differentiable_tree->image_number = tree->image_number;
#endif

    differentiable_tree->root = DifferentiateNode( tree->root, independent_variable );

#ifdef _DEBUG
    TreeDump( differentiable_tree, "After differentiation" );

    tree->logging.log_file = differentiable_tree->logging.log_file;
    tree->image_number = differentiable_tree->image_number;

    differentiable_tree->logging.log_file = NULL;
#endif

    if ( differentiable_tree->root ) {
        return differentiable_tree;
    }
    else {
        PRINT_ERROR( "Error differentiating the expression!!! \n" );
        return NULL;
    }
}


#define NUM_( n ) NodeCreate(MakeNumber(n), NULL)
#define VAR_( v ) NodeCreate(MakeVariable(v), NULL)

#define ADD_( L, R ) MakeNode( OP_ADD, L, R )
#define SUB_( L, R ) MakeNode( OP_SUB, L, R )
#define MUL_( L, R ) MakeNode( OP_MUL, L, R )
#define DIV_( L, R ) MakeNode( OP_DIV, L, R )
#define POW_( L, R ) MakeNode( OP_POW, L, R )

#define cL NodeCopy( node->left )
#define cR NodeCopy( node->right )

#define dL DifferentiateNode( node->left,  independent_variable )
#define dR DifferentiateNode( node->right, independent_variable )

static Node_t* MakeNode( OperationType op, Node_t* L, Node_t* R ) {
    Node_t* n = NodeCreate( MakeOperation( op ), NULL );
    n->left = L;
    if (L) L->parent = n;
    n->right = R;
    if (R) R->parent = n;
    return n;
}

static Node_t* DifferentiateNode( Node_t* node, char independent_variable ) {
    if ( !node )
        return NULL;

    switch ( node->value.type ) {
        case NODE_NUMBER:
            return NUM_( 0 );
        case NODE_OPERATION:
            switch ( node->value.data.operation ) {

                case OP_ADD:
                    return ADD_( dL, dR );

                case OP_SUB:
                    return SUB_( dL, dR );

                case OP_MUL:
                    return ADD_( MUL_( dL, cR ), MUL_( cL, dR ) );

                case OP_DIV:
                    return DIV_(
                        SUB_( MUL_( dL, cR ), MUL_( cL, dR ) ),
                        MUL_( cR, cR )
                    );

                case OP_POW:
                    return MUL_(
                        POW_( cL, cR ),
                        ADD_(
                            MUL_( dR, MakeNode( OP_LOG, cL, NULL ) ),
                            MUL_( cR, DIV_( dL, cL ) )
                        )
                    );

                case OP_LOG:
                    return DIV_( dL, cL );

                case OP_SIN:
                    return MUL_( MakeNode( OP_COS, cL, NULL ), dL );

                case OP_COS:
                    return MUL_( NUM_(-1),
                                MUL_( MakeNode( OP_SIN, cL, NULL ), dL ) );

                case OP_TAN:
                    return DIV_(
                        dL,
                        POW_( MakeNode( OP_COS, cL, NULL ), NUM_(2) )
                    );

                default:
                    PRINT_ERROR("Unknown operation in differentiation!\n");
                    return NULL;
            }
            break;

        case NODE_VARIABLE:
            if ( node->value.data.variable == independent_variable )
                return NUM_( 1 );
            else
                return NUM_( 0 );
        
        case NODE_UNKNOWN:
        default:
            PRINT_ERROR( "Error while defferentiationg the expression!!! \n" );
    }

    return NULL;
}

static TreeData_t MakeNumber( double number ) {
    TreeData_t value = {};
    value.type = NODE_NUMBER;
    value.data.number = number;

    return value;
}

static TreeData_t MakeOperation( OperationType operation ) {
    TreeData_t value = {};

    value.type = NODE_OPERATION;
    value.data.operation = operation;

    return value;
}

static TreeData_t MakeVariable( char variable ) {
    TreeData_t value = {};

    value.type = NODE_VARIABLE;
    value.data.variable = variable;

    return value;
}