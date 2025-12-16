#ifndef OPERATION_H
#define OPERATION_H

enum IsFunction { // TODO: NonCustomOp, CustomOp
    NotFunction = 0,
    Function    = 1
};

enum NumberOfParams {
    ZERO_ARG = 0,
    ONE_ARG  = 1,
    TWO_ARGS = 2
};

#define INIT_OPERATIONS(macros) \
    macros("+",       OP_ADD,     0,  NotFunction, ZERO_ARG, "%e \n+ %e", \
           "О, чудо! Складываем два выражения — получаем новое магическое число!") \
    macros("-",       OP_SUB,     1,  NotFunction, ZERO_ARG, "%e \n- %e", \
           "Вычитаем одно выражение из другого: пусть числа знают своё место!") \
    macros("*",       OP_MUL,     2,  NotFunction, ZERO_ARG, "%e \n\\cdot %e", \
           "Умножаем, потому что два плюс два иногда всё-таки четыре, а иногда больше!") \
    macros("/",       OP_DIV,     3,  NotFunction, ZERO_ARG, "\\frac{%e}{%e}", \
           "Деление: смотрим, что осталось после дележа пирога.") \
    macros("^",       OP_POW,     4,  NotFunction, ZERO_ARG, "%e^{%e}", \
           "Возводим в степень — космическая энергия математических сил!") \
    macros("log",     OP_LOG,     5,  Function,    TWO_ARGS, "\\log_{%e}(%e)", \
           "Логарифм — тайное оружие математика, чтобы числа выглядели меньше.") \
    macros("sin",     OP_SIN,     6,  Function,    ONE_ARG,  "\\sin(%e)", \
           "Синус танцует по оси X, как будто никто не смотрит.") \
    macros("cos",     OP_COS,     7,  Function,    ONE_ARG,  "\\cos(%e)", \
           "Косинус всегда сдержан, но надёжно!") \
    macros("tg",      OP_TAN,     8,  Function,    ONE_ARG,  "\\tan(%e)", \
           "Тангенс: наклон, который иногда слишком резок для школьников.") \
    macros("ctg",     OP_CTAN,    9,  Function,    ONE_ARG,  "\\cot(%e)", \
           "Котангенс — скрытая альтернатива тангенсу, чтобы путать друзей.") \
    macros("sh",      OP_SH,     10,  Function,    ONE_ARG,  "\\sinh(%e)", \
           "Гиперболический синус: когда обычный синус уже не достаточно эпичен.") \
    macros("ch",      OP_CH,     11,  Function,    ONE_ARG,  "\\cosh(%e)", \
           "Гиперболический косинус: красивое выражение для ленивых.") \
    macros("arcsin",  OP_ARCSIN, 12,  Function,    ONE_ARG,  "\\arcsin(%e)", \
           "Арксинус возвращает синус на землю. Спокойно, всё под контролем.") \
    macros("arccos",  OP_ARCCOS, 13,  Function,    ONE_ARG,  "\\arccos(%e)", \
           "Арккосинус: строгий, но справедливый математический судья.") \
    macros("arctg",   OP_ARCTAN, 14,  Function,    ONE_ARG,  "\\arctan(%e)", \
           "Арктангенс: наклонная философия чисел.") \
    macros("arcctg",  OP_ARCCTAN,15,  Function,    ONE_ARG,  "\\arccot(%e)", \
           "Арккотангенс: тайная магия, чтобы все удивились.") \
    macros("arcsh",   OP_ARSINH, 16,  Function,    ONE_ARG,  "\\operatorname{arsinh}(%e)", \
           "Арксинус гиперболический — слегка драматично, но работает.") \
    macros("arcch",   OP_ARCH,   17,  Function,    ONE_ARG,  "\\operatorname{arccosh}(%e)", \
           "Арккосинус гиперболический: просто добавим немного эпичности.") \
    macros("arcth",   OP_ARTANH, 18,  Function,    ONE_ARG,  "\\operatorname{arctanh}(%e)", \
           "Арктангенс гиперболический — для тех, кто любит сложные выражения.") \
    macros("ln",      OP_LN,     19,  Function,    ONE_ARG,  "\\ln(%e)", \
           "Натуральный логарифм: логика с приправой e!")

#endif