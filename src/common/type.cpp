#include "type.h"

char const *const Remain_List[9] = { "const", "int", "void", "if", "else", "while", "continue", "break", "return" };

char const *const Op_List[15] = { "+", "-", "!", "*", "/", "%", ">", "<", ">=", "<=", "==", "!=", "&&", "||", "=" };

char const *const Other_Symbol_List[8] = { ";", "(", ")", ",", "{", "}", "[", "]" };

char const *const Multi_Char_Symbol_List[6] = { ">=", "<=", "==", "!=", "&&", "||" };

char const *const Single_Char_Symbol_List[17] = { "+", "-", "!", "*", "/", "%", ">", "<", "=", ";", "(", ")", ",", "{", "}", "[", "]" };

char const *const Sys_Func_List[12] = { "getint", "getch", "getarray", "putint", "putch", "putarray", "putf", "starttime", "stoptime", "_sysy_starttime", "_sysy_stoptime", "malloc" };

const unsigned int Len_Remain_List = 9;
const unsigned int Len_Op_List = 15;
const unsigned int Len_Other_Symbol_List = 8;
const unsigned int Len_Multi_Char_Symbol_List = 6;
const unsigned int Lne_Single_Char_Symbol_List = 17;
const unsigned int Len_Sys_Func_List = 12;
char const *const Comment_Symbols[3] = { SINGLE_LINE_COMMENT_START, MULTI_LINE_COMMENT_START, MULTI_LINE_COMMENT_END };
