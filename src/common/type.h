#ifndef SRC_COMMON_TYPE_H_
#define SRC_COMMON_TYPE_H_

#define SINGLE_LINE_COMMENT_START "//"
#define MULTI_LINE_COMMENT_START "/*"
#define MULTI_LINE_COMMENT_END "*/"

enum Value_Type {
	VA_UNKNOWN, VA_INT, VA_VOID
};

enum Token_Type {
	Unknown_Token,
	Op_Token, Symbol_Token, Dec_Dig_Token, Hex_Dig_Token, Oct_Dig_Token,
	Const_Token, Int_Token, Void_Token, If_Token, Else_Token, While_Token, Continue_Token, Break_Token, Return_Token,
	Id_Token
};

enum Node_Type {
	Unknown_Node,
	Type_Node,
	Var_Type_Node,
	Def_Node,
	Var_Def_Node,
	Func_Def_Node,
	Refer_Node,
	Var_Node,
	Func_Node,
	Stmt_Node,
	Insert_Node,
	Dig_Node,
	Array_Node,
	Array_Access_Node,
	Op_Node,
	If_Else_Node,
	While_Node,
	Continue_Node,
	Break_Node,
	Return_Node,
	Block_Node
};

enum Op_Type {
	Unknown_Op, Non_Op,

	/* +  */Unary_Pos_Op,
	/* -  */Unary_Neg_Op,
	/* !  */Not_Op,
	/* *  */Mul_Op,
	/* /  */Div_Op,
	/* %  */Mod_Op,
	/* +  */Plus_Op,
	/* -  */Minus_Op,
	/* >  */Greater_Op,
	/* <  */Less_Op,
	/* >= */GEqual_Op,
	/* <= */LEqual_Op,
	/* == */Equal_Op,
	/* != */NEqual_Op,
	/* && */And_Op,
	/* || */Or_Op,
	/* =  */Assign_Op,
};

extern char const *const Remain_List[9];
extern char const *const Op_List[15];
extern char const *const Other_Symbol_List[8];
extern char const *const Multi_Char_Symbol_List[6];
extern char const *const Single_Char_Symbol_List[17];
extern char const *const Sys_Func_List[12];
extern char const *const Comment_Symbols[3];

extern const unsigned int Len_Remain_List;
extern const unsigned int Len_Op_List;
extern const unsigned int Len_Other_Symbol_List;
extern const unsigned int Len_Multi_Char_Symbol_List;
extern const unsigned int Lne_Single_Char_Symbol_List;
extern const unsigned int Len_Sys_Func_List;

#endif /* SRC_COMMON_TYPE_H_ */
