#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "DebugUtils.h"
#include "Tree.h"
#include "UtilsRW.h"

const uint32_t fill_color = 0xb6b4b4;

Tree_t *TreeCtor() {
    Tree_t *new_tree = (Tree_t *)calloc( 1, sizeof( *new_tree ) );
    assert( new_tree && "Memory allocation error" );

    return new_tree;
}

void TreeDtor( Tree_t **tree, void ( *clean_function )( TreeData_t value, Tree_t *tree ) ) {
    my_assert( tree, "Null pointer on pointer on `tree`" );
    if ( *tree == NULL )
        return;

    NodeDelete( ( *tree )->root, *tree, clean_function );

    free( *tree );
    *tree = NULL;
}

Node_t *NodeCreate( const TreeData_t field, Node_t *parent ) {
    Node_t *new_node = (Node_t *)calloc( 1, sizeof( *new_node ) );
    assert( new_node && "Memory allocation error" );

    new_node->value = field;
    new_node->parent = parent;

    new_node->left = NULL;
    new_node->right = NULL;

    return new_node;
}

Node_t *NodeLeftCreate( const TreeData_t value, Node_t *parent ) {
    Node_t *node = NodeCreate( value, parent );
    parent->left = node;

    return node;
}

Node_t *NodeRightCreate( const TreeData_t value, Node_t *parent ) {
    Node_t *node = NodeCreate( value, parent );
    parent->right = node;

    return node;
}

void NodeDelete( Node_t *node, Tree_t *tree, void ( *clean_function )( TreeData_t value, Tree_t *tree ) ) {
    if ( !node ) {
        return;
    }

    if ( node->parent ) {
        if ( node->parent->left == node )
            node->parent->left = NULL;
        else
            node->parent->right = NULL;
    }

    if ( node->left )
        NodeDelete( node->left, tree, clean_function );

    if ( node->right )
        NodeDelete( node->right, tree, clean_function );

    if ( clean_function )
        clean_function( node->value, tree );

    free( node );
}

Node_t *NodeCopy( Node_t *node ) {
    if ( !node )
        return NULL;

    Node_t *new_node = NodeCreate( node->value, NULL );
    assert( new_node && "Memory allocation error" );

    new_node->parent = NULL;

    new_node->left = NodeCopy( node->left );
    if ( new_node->left )
        new_node->left->parent = new_node;

    new_node->right = NodeCopy( node->right );
    if ( new_node->right )
        new_node->right->parent = new_node;

    return new_node;
}

#ifdef _SIMPLIFIED_DUMP
#else
static uint32_t my_crc32_ptr( const void *ptr ) {
    uintptr_t val = (uintptr_t)ptr;
    uint32_t crc = 0xFFFFFFFF;

    for ( size_t i = 0; i < sizeof( val ); i++ ) {
        uint8_t byte = ( val >> ( i * 8 ) ) & 0xFF;
        crc ^= byte;
        for ( int j = 0; j < 8; j++ ) {
            if ( crc & 1 )
                crc = ( crc >> 1 ) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}
#endif

#define DOT_PRINT( format, ... ) fprintf( dot_stream, format, ##__VA_ARGS__ );

static void NodeDumpRecursively( const Node_t *node, FILE *dot_stream );
static void NodeInitDot( const Node_t *node, FILE *dot_stream );
static void NodeBondInitDot( const Node_t *node, FILE *dot_stream );

void NodeGraphicDump( const Node_t *node, const char *image_path_name, ... ) {
    if ( !node || !image_path_name ) {
        PRINT_ERROR( "Null pointer on `node` %s:%d \n", __FILE__, __LINE__ );
        return;
    }

    char dot_path[MAX_LEN_PATH] = {};
    char svg_path[MAX_LEN_PATH + 5] = {};

    va_list args;
    va_start( args, image_path_name );
    vsnprintf( dot_path, MAX_LEN_PATH, image_path_name, args );
    va_end( args );

    snprintf( svg_path, MAX_LEN_PATH + 5, "%s.svg", dot_path );

    FILE *dot_stream = fopen( dot_path, "w" );
    assert( dot_stream && "File opening error" );

    fprintf( dot_stream, "digraph {\n\tsplines=line;\n" );
    NodeDumpRecursively( node, dot_stream );
    fprintf( dot_stream, "}\n" );

    fclose( dot_stream );

    char cmd[MAX_LEN_PATH * 3] = {};
    snprintf( cmd, sizeof( cmd ), "dot -Tsvg %s -o %s", dot_path, svg_path );
    system( cmd );
}

static void NodeDumpRecursively( const Node_t *node, FILE *dot_stream ) {
    if ( node == NULL ) {
        return;
    }

    NodeInitDot( node, dot_stream );
    NodeBondInitDot( node, dot_stream );
}

static void NodeInitDot( const Node_t *node, FILE *dot_stream ) {
#ifdef _SIMPLIFIED_DUMP
    DOT_PRINT( "\tnode_%lX [style=filled, ", (uintptr_t)node );
    switch ( node->value.type ) {
        case NODE_NUMBER:
            DOT_PRINT( "fillcolor=\"#5DADE2\", label=\"%lg\"]; \n", node->value.data.number );
            break;
        case NODE_VARIABLE:
            DOT_PRINT( "fillcolor=\"#82E0AA\", label=\"`%c`\"]; \n", node->value.data.variable );
            break;
        case NODE_OPERATION:
            DOT_PRINT( "fillcolor=\"#F5B041\", label=\"%s\"]; \n",
                       operations_txt[node->value.data.operation] );
            break;

        case NODE_UNKNOWN:
        default:
            DOT_PRINT( "fillcolor=\"#ff3737b9\", label=\"?\"]; \n" );
            break;
    }
#else
    DOT_PRINT( "\tnode_%lX [shape=plaintext; style=filled; color=black; "
               "fillcolor=\"#%X\"; label=< \n",
               (uintptr_t)node, fill_color );

    DOT_PRINT( "\t<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" "
               "ALIGN=\"CENTER\"> \n" );

    DOT_PRINT( "\t\t<TR> \n" );
    DOT_PRINT( "\t\t\t<TD PORT=\"idx\" BGCOLOR=\"#%X\">idx=0x%lX</TD> \n", my_crc32_ptr( node ),
               (uintptr_t)node );
    DOT_PRINT( "\t\t</TR> \n" );

    DOT_PRINT( "\t\t<TR> \n" );
    DOT_PRINT( "\t\t\t<TD PORT=\"parent\" BGCOLOR=\"#%X\">parent=0x%lX</TD> \n", my_crc32_ptr( node->parent ),
               (uintptr_t)node->parent );
    DOT_PRINT( "\t\t</TR> \n" );

    DOT_PRINT( "\t\t<TR> \n" );

    switch ( node->value.type ) {
        case NODE_NUMBER:
            DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=NUMBER</TD> \n" );
            DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
            DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=%lg</TD> \n", node->value.data.number );
            break;
        case NODE_VARIABLE:
            DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=VARIABLE</TD> \n" );
            DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
            DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=`%c`</TD> \n", node->value.data.variable );
            break;
        case NODE_OPERATION: {
            DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=OPERATION</TD> \n" );
            DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
            DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=%s</TD> \n",
                       node->value.data.operation >= 0 ? operations_txt[node->value.data.operation]
                                                       : "NOPE" );
            break;
        }
        case NODE_UNKNOWN:
        default:
            DOT_PRINT( "<TD PORT=\"type\" BGCOLOR=\"#FF0000\">type=UNKOWN</TD> \n" );
            break;
    }

    DOT_PRINT( "\t\t</TR> \n" );

    DOT_PRINT( "\t\t<TR> \n" );
    DOT_PRINT( "\t\t\t<TD> \n" );
    DOT_PRINT( "\t\t\t\t<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"2\" "
               "ALIGN=\"CENTER\"> \n" );
    DOT_PRINT( "\t\t\t\t\t<TR> \n" );

    DOT_PRINT( "\t\t\t\t\t\t<TD PORT=\"left\" BGCOLOR=\"#%X\" "
               "ALIGN=\"CENTER\">%lX</TD> \n",
               ( node->left == 0 ? fill_color : my_crc32_ptr( node->left ) ), (uintptr_t)node->left );

    DOT_PRINT( "\t\t\t\t\t\t<TD><FONT POINT-SIZE=\"10\">│</FONT></TD> \n" );

    DOT_PRINT( "\t\t\t\t\t\t<TD PORT=\"right\" BGCOLOR=\"#%X\" "
               "ALIGN=\"CENTER\">%lX</TD> \n",
               ( node->right == 0 ? fill_color : my_crc32_ptr( node->right ) ), (uintptr_t)node->right );

    DOT_PRINT( "\t\t\t\t\t</TR> \n" );
    DOT_PRINT( "\t\t\t\t</TABLE> \n" );
    DOT_PRINT( "\t\t\t</TD> \n" );
    DOT_PRINT( "\t\t</TR> \n" );

    DOT_PRINT( "\t</TABLE> \n" );
    DOT_PRINT( "\t>]; \n" );
#endif
}

static void NodeBondInitDot( const Node_t *node, FILE *dot_stream ) {
#ifdef _SIMPLIFIED_DUMP
    if ( node->left ) {
        DOT_PRINT( "\tnode_%lX -> node_%lX;\n", (uintptr_t)node, (uintptr_t)node->left );
        NodeDumpRecursively( node->left, dot_stream );
    }
    if ( node->right ) {
        DOT_PRINT( "\tnode_%lX -> node_%lX;\n", (uintptr_t)node, (uintptr_t)node->right );
        NodeDumpRecursively( node->right, dot_stream );
    }
#else
    if ( node->left ) {
        DOT_PRINT( "\tnode_%lX:left:s->node_%lX\n", (uintptr_t)node, (uintptr_t)node->left );
        NodeDumpRecursively( node->left, dot_stream );
    }

    if ( node->right ) {
        DOT_PRINT( "\tnode_%lX:right:s->node_%lX\n", (uintptr_t)node, (uintptr_t)node->right );
        NodeDumpRecursively( node->right, dot_stream );
    }
#endif
}

#undef DOT_PRINT

static void TreeSaveNode( const Node_t *node, FILE *stream, bool *error ) {
    if ( !node ) {
        fprintf( stream, "nil" );
        return;
    }

    fprintf( stream, "( " );

    switch ( node->value.type ) {
        case NODE_NUMBER:
            fprintf( stream, "%lg ", node->value.data.number );
            break;
        case NODE_VARIABLE:
            fprintf( stream, "%c ", node->value.data.variable );
            break;
        case NODE_OPERATION:
            fprintf( stream, "%s ", operations_txt[node->value.data.operation] );
            break;

        case NODE_UNKNOWN:
        default:
            *error = true;
            return;
    }

    TreeSaveNode( node->left, stream, error );
    if ( *error )
        return;

    fprintf( stream, " " );

    TreeSaveNode( node->right, stream, error );
    if ( *error )
        return;

    fprintf( stream, " )" );
}

void TreeSaveToFile( const Tree_t *tree, const char *filename ) {
    my_assert( tree, "Null pointer on tree" );
    my_assert( filename, "Null pointer on filename" );

    FILE *file_with_base = fopen( filename, "w" );
    my_assert( file_with_base, "Failed to open file for writing tree" );

    bool error = false;
    TreeSaveNode( tree->root, file_with_base, &error );
    if ( error )
        PRINT_ERROR( "Error writing a tree to a file!!!\n" );

    int result = fclose( file_with_base );
    assert( !result && "Error while closing file with tree" );

    if ( !error )
        fprintf( stdout, "The tree was saved in %s\n", filename );
}

static void CleanSpace( char **position ) {
    while ( isspace( **position ) )
        ( *position )++;
}

#define OP_IS_IT( str, name, ... )                                                                           \
    if ( strncmp( *current_position, str, strlen( str ) ) == 0 ) {                                           \
        read_bytes = strlen( str );                                                                          \
        PRINT( "Parse operation: %s \n", str );                                                              \
        return name;                                                                                         \
    }

static OperationType IsItOperation( char **current_position ) {
    CleanSpace( current_position );

    int read_bytes = 0;

    INIT_OPERATIONS( OP_IS_IT )

    return OP_NOPE;
}

#undef OP_IS_IT

static TreeData_t NodeParseValue( char **current_position ) {
    my_assert( current_position, "Null pointer on `current position`" );

    TreeData_t value = {};
    CleanSpace( current_position );

    int read_bytes = 0;
    if ( sscanf( *current_position, " %lf%n", &( value.data.number ), &read_bytes ) >= 1 ) {
        PRINT( "Parse number: %lg \n", value.data.number );
        value.type = NODE_NUMBER;
        ( *current_position ) += read_bytes;
        return value;
    }
    PRINT( "It's not a number \n" );

    OperationType op = IsItOperation( current_position );
    if ( op != OP_NOPE ) {
        value.type = NODE_OPERATION;
        value.data.operation = op;
        return value;
    }
    PRINT( "It's not an operation \n" );

    if ( isalpha( **current_position ) ) {
        PRINT( "Parse variable: %c \n", **current_position )
        value.type = NODE_VARIABLE;
        value.data.variable = **current_position;
        ( *current_position )++;
        return value;
    }
    PRINT( "It's not a variable \n" );

    value.type = NODE_UNKNOWN;
    PRINT( COLOR_BRIGHT_RED "[ERROR] It's unclear what \n" );
    return value;
}

#ifdef _DEBUG
#define INTERMIDIATE_DUMP( node, ... )                                                                       \
    Tree_t temp_tree = {};                                                                                   \
    temp_tree.root = node;                                                                                   \
    temp_tree.current_position = tree->current_position;                                                     \
    temp_tree.logging.log_file = tree->logging.log_file;                                                     \
    temp_tree.logging.img_log_path = tree->logging.img_log_path;                                             \
    temp_tree.image_number = tree->image_number++;                                                           \
    TreeDump( &temp_tree, ##__VA_ARGS__ );                                                                   \
    tree->logging.log_file = temp_tree.logging.log_file;
#else
#define INTEDMIDIATE_DUMP( ... )
#endif

static Node_t *TreeParseNode( char **current_position, bool *error, Node_t *parent ) {
    PRINT( "\n" );
    PRINT( "Сurrent position status: `%s` \n", *current_position );

    CleanSpace( current_position );

    if ( **current_position == '(' ) {
        ( *current_position )++;

        CleanSpace( current_position );
        TreeData_t value = NodeParseValue( current_position );
        if ( value.type == NODE_UNKNOWN ) {
            *error = true;
            return NULL;
        }

        Node_t *node = NodeCreate( value, parent );
        if ( !node ) {
            *error = true;
            return NULL;
        }

        CleanSpace( current_position );
        node->left = TreeParseNode( current_position, error, node );
        if ( *error ) {
            NodeDelete( node, NULL, NULL );
            return NULL;
        }
        if ( node->left )
            node->left->parent = node;

        CleanSpace( current_position );
        node->right = TreeParseNode( current_position, error, node );
        if ( *error ) {
            NodeDelete( node, NULL, NULL );
            return NULL;
        }
        if ( node->right )
            node->right->parent = node;

        CleanSpace( current_position );
        if ( **current_position != ')' ) {
            *error = true;
            return NULL;
        }
        ( *current_position )++;

        PRINT( "Сurrent position status after parsing new node: `%s` \n", *current_position );
        // INTERMIDIATE_DUMP( node, "After parsing new node" );

        return node;
    } else if ( strncmp( *current_position, "nil", 3 ) == 0 ) {
        *current_position += 3;
        PRINT( "Сurrent position status after parsing new node: `%s` \n", *current_position );
        return NULL;
    } else {
        *error = true;
        return NULL;
    }
}

Tree_t *TreeReadFromBuffer( char *buffer ) {
    my_assert( buffer, "Null pointer on `buffer`" );

    Tree_t *tree = TreeCtor();

    char *current_position = buffer;
    bool error = false;
    tree->root = TreeParseNode( &current_position, &error, NULL );

    if ( error ) {
        PRINT_ERROR( "База не считалась корректно. \n" );
        return NULL;
    } else {
        PRINT( "База считалась корректно. \n" );
        return tree;
    }
}