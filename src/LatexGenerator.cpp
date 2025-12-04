#include "Differentiator.h"
#include "DebugUtils.h"


#define LATEX_PRINT( format, ... ) fprintf( latex_file, format, ##__VA_ARGS__ );

static void LatexInsertChildren( const Node_t* node, FILE* latex_file, int parent_priority );

#define OPERATIONS_LATEX( str, name, value, is_func, n_args, latex_fmt ) \
    latex_fmt,

static const char* latex_format[] = {
    INIT_OPERATIONS( OPERATIONS_LATEX )
};

#undef OPERATIONS_LATEX

void DifferentiatorAddOrigExpression( Differentiator_t* diff, int order ) {
    my_assert( diff, "Null pointer on diff" );

    FILE* latex_file = diff->latex.tex_file;

    LATEX_PRINT( "\\section{Исходное выражение и производная}\n" );
    LATEX_PRINT( "\\subsection{\\textbf{Функция}} \\[ f(x) = " );
    TreeDumpLatex( diff->expr_tree, latex_file );
    LATEX_PRINT( " \\]\n" );

    for ( int i = 1; i <= order; i++ ) {
        LATEX_PRINT( "\\subsection{\\textbf{Производная порядка %d}} \n\n", i )
        
        DifferentiateExpression( diff, 'x', i );
        DifferentiatiorDump( diff, DUMP_DIFFERENTIATED, "Differentiative (%d)", i );

        LATEX_PRINT( "\\textbf{Итог: } \n\n" );
        LATEX_PRINT( "\\begin{math} \n" );
        LATEX_PRINT( "f^{(%d)}(x) = ", i );
        TreeDumpLatex( diff->diff_tree, latex_file );
        LATEX_PRINT( "\\end{math} \n" );
    }

    fflush( latex_file );
}

void TreeDumpLatex( const Tree_t* tree, FILE* latex_file ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( latex_file, "Null pointer on `latex_file`" );

    NodeToLatex( tree->root, latex_file, 0 );
}

static int GetOperationPriority( OperationType op ) {
    switch( op ) {
        case OP_ADD: return 0;
        case OP_SUB: return 1;
        case OP_MUL:
        case OP_DIV: return 2;
        case OP_POW: return 3;
        default: return 4; 
    }
}

void NodeToLatex( const Node_t* node, FILE* latex_file, int parent_priority ) {
    my_assert( node, "Null pointer on `node`" );

    switch( node->value.type ) {
        case NODE_NUMBER:
            LATEX_PRINT(" %.3g ", node->value.data.number);
            break;
        case NODE_VARIABLE:
            LATEX_PRINT(" %c ", node->value.data.variable);
            break;
        case NODE_OPERATION: 
            LatexInsertChildren( node, latex_file, parent_priority );
            break;
        case NODE_UNKNOWN:
        default:
            LATEX_PRINT("???");
            break;
    }
}


static void LatexInsertChildren( const Node_t* node, FILE* latex_file, int parent_priority ) {
    OperationType op = (OperationType)node->value.data.operation;
    const char* format = latex_format[op];
    int curr_priority = GetOperationPriority(op);

    bool need_brackets = (curr_priority < parent_priority);
    if (need_brackets) LATEX_PRINT("(");

    int child_id = 0;
    for (size_t idx = 0; format[idx]; idx++) {
        if (format[idx] == '%' && format[idx+1] == 'e') {
            const Node_t* child = (child_id == 0 ? node->left : node->right);
            if (child) NodeToLatex(child, latex_file, curr_priority);
            else LATEX_PRINT("<?>");
            child_id++;
            idx++;
        } else {
            fputc(format[idx], latex_file);
        }
    }

    if (need_brackets) LATEX_PRINT(")");
}

void DifferentiatorAddEvaluation( Differentiator_t* diff, char name ) {
    my_assert( diff, "Null pointer on diff" );

    double point = 0;
    if ( !VarTableGet( &diff->var_table, name, &point ) ) {
        VarTableAskUser( &diff->var_table );
    }

    double val = EvaluateTree( diff->expr_tree, diff );

    FILE* latex_file = diff->latex.tex_file;
    LATEX_PRINT( "\\subsection{Вычисление значения функции в точке}\n" );
    LATEX_PRINT( "Для $%c = %.6g$ получаем \\[ f(%c) = %.6g \\]\n\n", name, point, name, val );
    fflush( latex_file );
}

#define PRINT_O \
    LATEX_PRINT( " + o(" ); \
    if ( order > 1 ) { \
        LATEX_PRINT( "(x-%.3g)^%d", point, order ); \
    } \
    else { \
        LATEX_PRINT( "x-%.3g", point ); \
    } \
    LATEX_PRINT( ")\n" );

void DifferentiatorAddTaylorSeries( Differentiator_t* diff, char var, int order ) {
    my_assert( diff, "Null pointer on `diff`" );

    double point = 0;
    if ( !VarTableGet( &diff->var_table, var, &point ) ) {
        VarTableAskUser( &diff->var_table );
    }

    diff->taylor_tree = DifferentiatorBuildTaylorTree( diff, var, point, order );

    FILE* latex_file = diff->latex.tex_file;

#ifdef _DEBUG
    LATEX_PRINT( "\\subsection{Ряд Тейлора порядка %d в точке %.3lg (до оптимизации)}\n", order, point );
    LATEX_PRINT( "\\begin{math} \n" );
    LATEX_PRINT( "T_{%d}(%c) = ", order, var );
    TreeDumpLatex( diff->taylor_tree, diff->latex.tex_file );
    PRINT_O;
    LATEX_PRINT( "\\end{math} \n" );

    DifferentiatiorDump( diff, DUMP_TAYLOR, "Before optimization" );
#endif

    OptimizeTree( diff->taylor_tree, diff, var );

    LATEX_PRINT( "\\subsection{Ряд Тейлора порядка %d в точке %.3lg}\n", order, point );
    LATEX_PRINT( "\\begin{math} \n" );
    LATEX_PRINT( "T_{%d}(%c) = ", order, var );
    TreeDumpLatex( diff->taylor_tree, diff->latex.tex_file );
    PRINT_O;
    LATEX_PRINT( "\\end{math} \n" );

    DifferentiatiorDump( diff, DUMP_TAYLOR, "After optimization" );
}


static const char* joke_lines[] = {
    [OP_ADD]     = "О, чудо! Складываем два выражения — получаем новое магическое число!",
    [OP_SUB]     = "Вычитаем одно выражение из другого: пусть числа знают своё место!",
    [OP_MUL]     = "Умножаем, потому что два плюс два иногда всё-таки четыре, а иногда больше!",
    [OP_DIV]     = "Деление: смотрим, что осталось после дележа пирога.",
    [OP_POW]     = "Возводим в степень — космическая энергия математических сил!",
    [OP_LOG]     = "Логарифм — тайное оружие математика, чтобы числа выглядели меньше.",
    [OP_SIN]     = "Синус танцует по оси X, как будто никто не смотрит.",
    [OP_COS]     = "Косинус всегда сдержан, но надёжно!",
    [OP_TAN]     = "Тангенс: наклон, который иногда слишком резок для школьников.",
    [OP_CTAN]    = "Котангенс — скрытая альтернатива тангенсу, чтобы путать друзей.",
    [OP_SH]      = "Гиперболический синус: когда обычный синус уже не достаточно эпичен.",
    [OP_CH]      = "Гиперболический косинус: красивое выражение для ленивых.",
    [OP_ARCSIN]  = "Арксинус возвращает синус на землю. Спокойно, всё под контролем.",
    [OP_ARCCOS]  = "Арккосинус: строгий, но справедливый математический судья.",
    [OP_ARCTAN]  = "Арктангенс: наклонная философия чисел.",
    [OP_ARCCTAN] = "Арккотангенс: тайная магия, чтобы все удивились.",
    [OP_ARSINH]  = "Арксинус гиперболический — слегка драматично, но работает.",
    [OP_ARCH]    = "Арккосинус гиперболический: просто добавим немного эпичности.",
    [OP_ARTANH]  = "Арктангенс гиперболический — для тех, кто любит сложные выражения.",
    [OP_LN]      = "Натуральный логарифм: логика с приправой e!"
};

// Пример использования:
const char* GetJokeLine( OperationType op ) {
    if ( op < 0 ) {
        return "Операция неизвестна, но всё равно весело!";
    }

    return joke_lines[ op ];
}
