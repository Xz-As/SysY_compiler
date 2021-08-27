#include <string.h>
#include <stack>
#include <sstream>
#include <iostream>
#include "./parser.h"
#include "./word_ana.h"
#include "../common/global.h"
using namespace std;

char const *last_parse_error_msg = nullptr;

inline Op_Type op_type_(char const *const op, bool is_unary) {
	if (is_unary) {
		if (!strcmp("+", op))
			return Unary_Pos_Op;
		if (!strcmp("-", op))
			return Unary_Neg_Op;
		if (!strcmp("!", op))
			return Not_Op;
	}
	else {
		if (!strcmp("*", op))
			return Mul_Op;
		if (!strcmp("/", op))
			return Div_Op;
		if (!strcmp("%", op))
			return Mod_Op;
		if (!strcmp("+", op))
			return Plus_Op;
		if (!strcmp("-", op))
			return Minus_Op;
		if (!strcmp(">", op))
			return Greater_Op;
		if (!strcmp("<", op))
			return Less_Op;
		if (!strcmp(">=", op))
			return GEqual_Op;
		if (!strcmp("<=", op))
			return LEqual_Op;
		if (!strcmp("==", op))
			return Equal_Op;
		if (!strcmp("!=", op))
			return NEqual_Op;
		if (!strcmp("&&", op))
			return And_Op;
		if (!strcmp("||", op))
			return Or_Op;
		if (!strcmp("=", op))
			return Assign_Op;
	}
	return Unknown_Op;
}

inline OPERATOR_INFO const& op_info_(Op_Type op) {
	return OPERATOR_INFO_MAP[op];
}

inline int op_precedence_compare_(Op_Type op0, Op_Type op1) {
	return op_info_(op0).precedence - op_info_(op1).precedence;
}

inline SY_ASSOCIATIVITY op_associativity_(Op_Type op) {
	return op_info_(op).associativity;
}

inline int op_par_number_(Op_Type op) {
	return op_info_(op).parameter_number;
}

inline bool expect(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end, char const *const str) {
	return current != end && (*current)->get_name_() == str && (++current, 1);
}

inline bool gen_top_op_node(std::stack<Op_Type> &operator_stack, std::stack<std::shared_ptr<Node>> &node_stack) {
	if (operator_stack.empty())
		return false;
	Op_Type top_operator = operator_stack.top();
	operator_stack.pop();
	std::shared_ptr<OperationNode> expr_node(new OperationNode());
	expr_node->set_type_(Op_Node);
	/*if (!node_stack.empty() && top_operator == Assign_Op){
		std::shared_ptr<Node> &node = node_stack.top();
		if (node->_get_type() == Var_Node){
			cout << "const is not a valid left value!" << std::endl;
			last_parse_error_msg = "left value set const";
			return false
		}
	}*/
	for (int i = 0, end = op_par_number_(top_operator); i < end; ++i) {
		if (node_stack.empty())
			return false;
		std::shared_ptr<Node> &node = node_stack.top();
		expr_node->add_parameter(node);
		node->setParent(std::dynamic_pointer_cast<Node>(expr_node));
		node_stack.pop();
	}
	expr_node->reverse_parameters();
	expr_node->setOperatorType(top_operator);
	std::shared_ptr<Node> node = std::dynamic_pointer_cast<Node>(expr_node);
	node_stack.push(node);
	return true;
}

std::shared_ptr<Node> gen_expr_node(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end) {

	bool previous_is_operator = true, for_end = false;

	std::stack<Op_Type> operator_stack;
	std::stack<std::shared_ptr<Node>> node_stack;

	for (Token_Type type = (*current)->get_token_type_(); end != current; type = (*current)->get_token_type_()) {
		switch (type) {
		// Op
		case Op_Token: {
			// Unary
			if (previous_is_operator) {
				Op_Type type = op_type_((*current)->get_name_().c_str(), true);
				if (Unknown_Op == type)
					return nullptr;
				operator_stack.push(type);
			}
			else {
				Op_Type type = op_type_((*current)->get_name_().c_str(), false);
				if (Unknown_Op == type)
					return nullptr;
				if (AS_L2R == op_associativity_(type)) {
					while (!operator_stack.empty() && 0 <= op_precedence_compare_(operator_stack.top(), type))
						gen_top_op_node(operator_stack, node_stack);
					operator_stack.push(type);
				}
				else {
					while (!operator_stack.empty() && 0 < op_precedence_compare_(operator_stack.top(), type))
						gen_top_op_node(operator_stack, node_stack);
					operator_stack.push(type);
				}
			}
			++current;
			previous_is_operator = true;
		}
			break;
		// Digit
		case Dec_Dig_Token:
		case Hex_Dig_Token:
		case Oct_Dig_Token: {
			std::shared_ptr<LiteralNode> literal_node(new LiteralNode());
			literal_node->set_type_(Dig_Node);
			if (!literal_node->setValue((*current)->get_name_().c_str()))
				return nullptr;
			std::shared_ptr<Node> node = std::dynamic_pointer_cast<Node>(literal_node);
			node_stack.push(node);
			++current;
			previous_is_operator = false;
		}
			break;
		case Id_Token: {
			std::string const &name = (*(current++))->get_name_();
			// function
			if (expect(current, end, "(")) {
				std::shared_ptr<FunNode> fun_node(new FunNode(nullptr, Func_Node, name.c_str()));
				node_stack.push(std::dynamic_pointer_cast<Node>(fun_node));
				if (!expect(current, end, ")")) {
					do {
						std::shared_ptr<Node> node = gen_expr_node(current, end);
						if (!node)
							return nullptr;
						node->setParent(std::dynamic_pointer_cast<Node>(fun_node));
						fun_node->add_parameter(node);
					}
					while (expect(current, end, ","));
					if (!expect(current, end, ")")){
						cout << "expected a ) !" << std::endl;
						last_parse_error_msg = "no )";
						return nullptr;
					}
				} else {
					// No parameter
				}
			}
			else { // variable
				std::shared_ptr<VarNode> var_node(new VarNode(nullptr, Var_Node, name.c_str()));
				node_stack.push(std::dynamic_pointer_cast<Node>(var_node));
			}
			previous_is_operator = false;
		}
			break;
		case Symbol_Token:
			if ((*current)->get_name_() == "(") {
				++current;
				std::shared_ptr<Node> node = gen_expr_node(current, end);
				if (!node)
					return nullptr;
				std::shared_ptr<ParenthesisNode> parenthesis_node(new ParenthesisNode(nullptr, Insert_Node, node));
				node->setParent(std::dynamic_pointer_cast<Node>(parenthesis_node));
				node_stack.push(std::dynamic_pointer_cast<Node>(parenthesis_node));
				if (!expect(current, end, ")")){
					cout << "expected a ) !" << std::endl;
					last_parse_error_msg = "no )";
					return nullptr;
				}
				previous_is_operator = false;
				break;
			}
			if ((*current)->get_name_() == "{") {
				++current;
				std::shared_ptr<ArrayNode> array_node(new ArrayNode(nullptr, Array_Node));
				node_stack.push(std::dynamic_pointer_cast<Node>(array_node));
				if (!expect(current, end, "}")) { // have element
					do {
						std::shared_ptr<Node> node = gen_expr_node(current, end);
						if (!node)
							return nullptr;
						node->setParent(std::dynamic_pointer_cast<Node>(array_node));
						array_node->add_array(node);
					} while (expect(current, end, ","));
					if (!expect(current, end, "}"))
						return nullptr;
				}
				previous_is_operator = false;
				break;
			}
			if ((*current)->get_name_() == "[") {
				++current;
				if (node_stack.empty())
					return nullptr;
				std::shared_ptr<ArrayAccessNode> array_access_node;
				std::shared_ptr<Node> &top_node = node_stack.top();
				if (Array_Access_Node != top_node->get_type_()) {
					array_access_node = std::shared_ptr<ArrayAccessNode>(new ArrayAccessNode(nullptr, Array_Access_Node, top_node));
					top_node->setParent(std::dynamic_pointer_cast<Node>(array_access_node));
					std::shared_ptr<Node> node = std::dynamic_pointer_cast<Node>(array_access_node);
					node_stack.pop();
					node_stack.push(node);
				} else {
					array_access_node = std::dynamic_pointer_cast<ArrayAccessNode>(top_node);
				}
				std::shared_ptr<Node> node = gen_expr_node(current, end);
				if (!node)
					return nullptr;
				node->setParent(std::dynamic_pointer_cast<Node>(array_access_node));
				array_access_node->add_index(node);
				if (!expect(current, end, "]")){
					cout << "expected a ] !" << std::endl;
					last_parse_error_msg = "no ]";
					return nullptr;
				}
				previous_is_operator = false;
				break;
			}
		default:
			for_end = true;
		}
		if(for_end)
			break;
	}

	while (!operator_stack.empty())
		if (!gen_top_op_node(operator_stack, node_stack))
			return nullptr;
	if (1 != node_stack.size())
		return nullptr;
	std::shared_ptr<Node> node = node_stack.top();
	node_stack.pop();
	return node;
}

std::shared_ptr<TypeNode> gen_type_node(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end) {
	Token_Type const type = (*current)->get_token_type_();
	std::shared_ptr<TypeNode> _type(new TypeNode(nullptr, Type_Node, Int_Token == type ? VA_INT : VA_VOID));
	++current;
	while (expect(current, end, "[")) {
		if (expect(current, end, "]")) { // empty dimension [][] set to -1;
			std::shared_ptr<LiteralNode> literal_node(new LiteralNode(nullptr, Dig_Node, -1));
			std::shared_ptr<Node> node = std::dynamic_pointer_cast<Node>(literal_node);
			_type->add_dimension(node);
			node->setParent(std::dynamic_pointer_cast<Node>(_type));
		} else {
			std::shared_ptr<Node> node = gen_expr_node(current, end);
			_type->add_dimension(node);
			node->setParent(std::dynamic_pointer_cast<Node>(_type));
			if (!expect(current, end, "]")){
				cout << "expected a ] !" << std::endl;
				last_parse_error_msg = "no ]";
				return nullptr;
			}
		}
	}
	return _type;
}

std::shared_ptr<VarTypeNode> gen_var_type_node(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end) {
	std::string const &name = (*current)->get_name_();
	std::shared_ptr<VarTypeNode> var_type_node(new VarTypeNode(nullptr, Var_Type_Node, name.c_str()));
	++current;
	while (expect(current, end, "[")) {
		if (expect(current, end, "]")) {
			std::shared_ptr<LiteralNode> literal_node(new LiteralNode(nullptr, Dig_Node, -1));
			std::shared_ptr<Node> node = std::dynamic_pointer_cast<Node>(literal_node);
			var_type_node->add_dimension(node);
			node->setParent(std::dynamic_pointer_cast<Node>(var_type_node));
		}
		else {
			std::shared_ptr<Node> node = gen_expr_node(current, end);
			var_type_node->add_dimension(node);
			node->setParent(std::dynamic_pointer_cast<Node>(var_type_node));
			if (!expect(current, end, "]")){
				cout << "expected a ] !" << std::endl;
				last_parse_error_msg = "no ]";
				return nullptr;
			}
		}
	}
	return var_type_node;
}

std::shared_ptr<VarDefNode> gen_single_variable_definition_node(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end) {

	std::shared_ptr<TypeNode> _type = gen_type_node(current, end);
	if (!_type)
		return nullptr;
	std::shared_ptr<VarDefNode> var_def_node(new VarDefNode(nullptr, Var_Def_Node, "", _type, false));
	std::shared_ptr<VarTypeNode> var_type = gen_var_type_node(current, end);
	if (!var_type)
		return nullptr;
	std::shared_ptr<Node> var_init_value;
	if (expect(current, end, "=")) {
		var_init_value = gen_expr_node(current, end);
		if (!var_init_value){
			cout << "invalid init value!" << std::endl;
			last_parse_error_msg = "invalide value";
			return nullptr;
		}
	}
	var_def_node->add_var(var_type, var_init_value);
	var_type->setParent(std::dynamic_pointer_cast<Node>(var_def_node));
	if (var_init_value)
		var_init_value->setParent(std::dynamic_pointer_cast<Node>(var_def_node));

	return var_def_node;
}

std::shared_ptr<VarDefNode> gen_multi_variable_definition_node(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end) {
	std::shared_ptr<TypeNode> _type = gen_type_node(current, end);
	if (!_type)
		return nullptr;
	std::shared_ptr<VarDefNode> var_def_node(new VarDefNode(nullptr, Var_Def_Node, "", _type, false));
	while (current != end) {
		std::shared_ptr<VarTypeNode> var_type = gen_var_type_node(current, end);
		if (!var_type)
			return nullptr;
		std::shared_ptr<Node> var_init_value;
		if (expect(current, end, "=")) {
			var_init_value = gen_expr_node(current, end);
			if (!var_init_value){
				cout << "invalid init value!" << std::endl;
				last_parse_error_msg = "invalide value";
				return nullptr;
			}
		}
		var_def_node->add_var(var_type, var_init_value);
		var_type->setParent(std::dynamic_pointer_cast<Node>(var_def_node));
		if (var_init_value)
			var_init_value->setParent(std::dynamic_pointer_cast<Node>(var_def_node));
		if (expect(current, end, ","))
			continue; //move to next
		if (expect(current, end, ";"))
			break;
		cout << "expected a ; at the end of this line!" << std::endl;
		last_parse_error_msg = "no ;";
		return nullptr;
	}
	return var_def_node;
}

std::shared_ptr<BlockNode> gen_block_or_single_line_block(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end) {
	std::shared_ptr<BlockNode> block_node;
	if (expect(current, end, "{")) {
		block_node = gen_node_(current, end);
		if (!block_node)
			return nullptr;
		if (!expect(current, end, "}")){
			cout << "expected a } at the end of block!" << std::endl;
			last_parse_error_msg = "no }";
			return nullptr;
		}
	}
	else {
		bool is_reach_end = false;
		std::shared_ptr<Node> child_node = gen_child_node_(current, end, is_reach_end);
		if (is_reach_end)
			return nullptr;
		if (!child_node)
			return nullptr;
		block_node = std::shared_ptr<BlockNode>(new BlockNode(nullptr, Block_Node, child_node));
		child_node->setParent(std::dynamic_pointer_cast<Node>(block_node));
	}
	return block_node;
}

std::shared_ptr<IfElseNode> gen_if_else_node(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end) {
	if (!expect(current, end, "if")){
		cout << "expected a if here" << std::endl;
		last_parse_error_msg = "no if";
		return nullptr;
		//never reach
	}
	if (!expect(current, end, "(")){
		cout << "expected condition of if!" << std::endl;
		last_parse_error_msg = "no cond of if";
		return nullptr;
	}
	std::shared_ptr<Node> condition = gen_expr_node(current, end);
	if (!condition)
		return nullptr;
	if (!expect(current, end, ")")){
		cout << "expected a )!" << std::endl;
		last_parse_error_msg = "no )";
		return nullptr;
	}
	std::shared_ptr<BlockNode> if_body = gen_block_or_single_line_block(current, end);
	if (!if_body)
		return nullptr;
	std::shared_ptr<BlockNode> else_body;
	if (expect(current, end, "else")) {  // else block
		else_body = gen_block_or_single_line_block(current, end);
		if (!else_body){
			cout << "expected some stmts here!" << std::endl;
			last_parse_error_msg = "no else body";
			return nullptr;
		}
	}
	std::shared_ptr<IfElseNode> if_else_node(new IfElseNode(nullptr, If_Else_Node, condition, if_body, else_body));
	condition->setParent(std::dynamic_pointer_cast<Node>(if_else_node));
	if_body->setParent(std::dynamic_pointer_cast<Node>(if_else_node));
	if (else_body)
		else_body->setParent(std::dynamic_pointer_cast<Node>(if_else_node));
	return if_else_node;
}

std::shared_ptr<Node> gen_child_node_(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end, bool &is_reach_end) {
	is_reach_end = false;
	if (current == end) {
		is_reach_end = true;
		return nullptr;
	}
	Token_Type type = (*current)->get_token_type_();
	switch (type) {
	case Symbol_Token: {
		std::string const &name = (*current)->get_name_();
		if ("(" != name) {
			switch (*name.c_str()) {
			case ';': //empty expression
			{
				std::shared_ptr<StmtNode> stmt_node(new StmtNode(nullptr, Stmt_Node, nullptr));
				++current;
				return std::dynamic_pointer_cast<Node>(stmt_node);
			}
			case '{': //block
			{
				++current;
				std::shared_ptr<BlockNode> _block_node = gen_node_(current, end);
				if (!_block_node)
					return nullptr;
				if (!expect(current, end, "}")){
					cout << "expected a }!" << std::endl;
					last_parse_error_msg = "no }";
					return nullptr;
				}
				return std::dynamic_pointer_cast<Node>(_block_node);
			}
			default:
				is_reach_end = true;
				return nullptr;
			}
		}
	}
	case Id_Token: 
	case Op_Token:
	case Dec_Dig_Token:
	case Hex_Dig_Token:
	case Oct_Dig_Token: {
		// expression statement
		std::shared_ptr<Node> node = gen_expr_node(current, end);
		if (!node)
			return nullptr;
		std::shared_ptr<StmtNode> stmt_node(new StmtNode(nullptr, Stmt_Node, node));
		node->setParent(std::dynamic_pointer_cast<Node>(stmt_node));
		if (!expect(current, end, ";")){
			cout << "expected a ; at the end of this line!" << std::endl;
			last_parse_error_msg = "no ;";
			return nullptr;
		}
		return std::dynamic_pointer_cast<Node>(stmt_node);
	}
	case Const_Token: {
		++current;
		std::shared_ptr<VarDefNode> var_def_node = gen_multi_variable_definition_node(current, end);
		if (!var_def_node){
			cout << "no defination of this const!" << std::endl;
			last_parse_error_msg = "no def const";
			return nullptr;
		}
		var_def_node->setIsConst(true);
		std::vector<std::shared_ptr<Node> >const &var_init_value = var_def_node->getVarInitValue();
		bool is_init = true;
		for (std::vector<std::shared_ptr<Node> >::const_iterator it = var_init_value.begin(), end = var_init_value.end(); it != end; ++it) {
			if (!(*it)){
				is_init = false;
				break;
			}
		}
		if(!is_init){
			std::cout <<  "const not init! Please check your code!" << std::endl;
			last_parse_error_msg = "const not init";
		}
		return std::dynamic_pointer_cast<Node>(var_def_node);
	}
	case Int_Token:
	case Void_Token: {
		// variable & function definition
		std::shared_ptr<TypeNode> _type = gen_type_node(current, end);
		if (!_type)
			return nullptr;
		Token_Type type = (*current)->get_token_type_();
		if (Id_Token != type)
			return nullptr;
		std::string const &name = (*current)->get_name_();
		// function definition / declaration
		++current;
		if (expect(current, end, "(")) {
			std::shared_ptr<FunDefNode> fun_def_node(new FunDefNode());
			fun_def_node->set_type_(Func_Def_Node);
			fun_def_node->setReturnType(_type);
			fun_def_node->set_name_(name);
			_type->setParent(fun_def_node);

			// Function parameters
			if (!expect(current, end, ")")) {
				do {
					std::shared_ptr<VarDefNode> var_def_node = gen_single_variable_definition_node(current, end);
					if (!var_def_node)
						return nullptr;
					fun_def_node->add_parameter(var_def_node);
					var_def_node->setParent(std::dynamic_pointer_cast<Node>(fun_def_node));
				} while (expect(current, end, ","));
				if (!expect(current, end, ")"))
					return nullptr;
			}
			// Function body
			if (!expect(current, end, ";")) {
				if (!expect(current, end, "{")){
					cout << "expected a ; at the end of this line!" << std::endl;
					last_parse_error_msg = "no ;";
					return nullptr;
				}
				std::shared_ptr<BlockNode> block_node = gen_node_(current, end);
				if (!block_node){
					cout << "expected a block!" << std::endl;
					last_parse_error_msg = "no block in function body";
					return nullptr;
				}
				fun_def_node->setBody(block_node);
				block_node->setParent(std::dynamic_pointer_cast<Node>(fun_def_node));
				if (!expect(current, end, "}"))
					return nullptr;
			}
			return std::dynamic_pointer_cast<Node>(fun_def_node);
		}
		else { // variable definition
			--current;
			std::shared_ptr<VarDefNode> var_def_node(new VarDefNode(nullptr, Var_Def_Node, "", _type, false));
			while (current != end) {
				std::shared_ptr<VarTypeNode> var_type = gen_var_type_node(current, end);
				if (!var_type)
					return nullptr;
				std::shared_ptr<Node> var_init_value;
				if (expect(current, end, "=")) { // Have init value
					var_init_value = gen_expr_node(current, end);
					if (!var_init_value)
						return nullptr;
				}
				var_def_node->add_var(var_type, var_init_value);
				var_type->setParent(std::dynamic_pointer_cast<Node>(var_def_node));
				if (var_init_value)
					var_init_value->setParent(std::dynamic_pointer_cast<Node>(var_def_node));
				if (expect(current, end, ","))
					continue; //move to next
				if (expect(current, end, ";"))
					break;
				cout << "expected a ; at the end of this line!" << std::endl;
				last_parse_error_msg = "no ;";
				return nullptr;
			}
			return std::dynamic_pointer_cast<Node>(var_def_node);
		}
	}
	case If_Token: {
		std::shared_ptr<IfElseNode> if_else_node = gen_if_else_node(current, end);
		if (!if_else_node)
			return nullptr;
		return std::dynamic_pointer_cast<Node>(if_else_node);
	}
	case While_Token: {
		++current;
		if (!expect(current, end, "("))
			return nullptr;
		std::shared_ptr<Node> condition = gen_expr_node(current, end);
		if (!condition)
			return nullptr;
		if (!expect(current, end, ")"))
			return nullptr;
		std::shared_ptr<BlockNode> body = gen_block_or_single_line_block(current, end);
		if (!body)
			return nullptr;
		std::shared_ptr<WhileNode> while_node(new WhileNode(nullptr, While_Node, condition, body));
		condition->setParent(std::dynamic_pointer_cast<Node>(while_node));
		body->setParent(std::dynamic_pointer_cast<Node>(while_node));
		return std::dynamic_pointer_cast<Node>(while_node);
	}
	case Continue_Token: {
		++current;
		if (!expect(current, end, ";")){
			cout << "expected a ; at the end of this line!" << std::endl;
			last_parse_error_msg = "no ;";
			return nullptr;
		}
		return std::dynamic_pointer_cast<Node>(std::shared_ptr<ContinueNode>(new ContinueNode(nullptr, Continue_Node)));
	}
	case Break_Token: {
		++current;
		if (!expect(current, end, ";")){
			cout << "expected a ; at the end of this line!" << std::endl;
			last_parse_error_msg = "no ;";
			return nullptr;
		}
		return std::dynamic_pointer_cast<Node>(std::shared_ptr<BreakNode>(new BreakNode(nullptr, Break_Node)));
	}
	case Return_Token: {
		++current;
		std::shared_ptr<Node> return_value;
		if (!expect(current, end, ";")) { // Have return value
			return_value = gen_expr_node(current, end);
			if (!return_value){
				cout << "No return value!" << std::endl;
				last_parse_error_msg = "no return value";
				return nullptr;
			}
			if (!expect(current, end, ";")){
				cout << "expected a ; at the end of this line!" << std::endl;
				last_parse_error_msg = "no ;";
				return nullptr;
			}
		}
		std::shared_ptr<ReturnNode> return_node(new ReturnNode(nullptr, Return_Node, return_value));
		if (return_value)
			return_value->setParent(return_node);
		return std::dynamic_pointer_cast<Node>(return_node);
	}
	case Else_Token:
	case Unknown_Token:
	default:{
		cout << "Unknown or invalid token!" << std::endl;
		last_parse_error_msg = "invalid else or invalid id!";
		return nullptr;
	}
	}
	return nullptr;
}

std::shared_ptr<BlockNode> gen_node_(std::vector<std::shared_ptr<Token>>::const_iterator &current, std::vector<std::shared_ptr<Token>>::const_iterator &end) {
	std::shared_ptr<BlockNode> block_node(new BlockNode(nullptr, Block_Node));
	while (end != current) {
		bool is_reach_end = false;
		std::shared_ptr<Node> child_node = gen_child_node_(current, end, is_reach_end);
		if (is_reach_end)
			return block_node;
		if (!child_node)
			return nullptr;
		block_node->add_child(std::dynamic_pointer_cast<Node>(child_node));
		child_node->setParent(std::dynamic_pointer_cast<Node>(block_node));
	}
	return block_node;
}

// Every new node will set the parent type
std::shared_ptr<BlockNode> parse_(std::vector<std::shared_ptr<Token>>const &tokens) {
	std::vector<std::shared_ptr<Token>>::const_iterator begin = tokens.begin(), end = tokens.end(), current = begin;
	std::shared_ptr<BlockNode> root_block = gen_node_(current, end);
	if (!root_block)
		return nullptr;
	if (current != end)
		return nullptr;
	return root_block;
}

inline void build_array(std::vector<std::shared_ptr<Node> >const &array, std::ostringstream &oss) {
	oss << "[";
	for (std::vector<std::shared_ptr<Node> >::const_iterator it = array.begin(), end = array.end(); it != end; ++it) {
		node2json_dump_(std::dynamic_pointer_cast<Node>(*it), oss);
	}
	oss << "]" << std::endl;// << std::endl;
}

bool node2json_dump_(std::shared_ptr<Node> const &node, std::ostringstream &oss) {
	if (last_parse_error_msg){
		oss << last_parse_error_msg;
		return false;
	}
	Node_Type type = node->get_type_();
	oss << std::endl;
	switch (type) {
	case Type_Node: {
		std::shared_ptr<TypeNode> const &_node = std::dynamic_pointer_cast<TypeNode>(node);
		oss << "type: " << "Type Node" << std::endl;
		oss << "variable_type: " << _node->getVariableType() << std::endl;
		std::vector<std::shared_ptr<Node>> const &dimensions = _node->getDimensions();
		oss << "dimenssions: ";
		build_array(dimensions, oss);
		oss << std::endl;// << std::endl;
	}
		break;
	case Var_Type_Node: {
		std::shared_ptr<VarTypeNode> const &_node = std::dynamic_pointer_cast<VarTypeNode>(node);
		oss << "type: " << "Var Type Node" << std::endl;
		oss << "name: " << _node->get_name_() << std::endl;
		std::vector<std::shared_ptr<Node>> const &dimensions = _node->getDimensions();
		oss << "dimenssions: ";
		build_array(dimensions, oss);
		oss << std::endl;// << std::endl;

	}
		break;
	case Def_Node: {
		std::shared_ptr<DefNode> const &_node = std::dynamic_pointer_cast<DefNode>(node);
		oss << "type: " << "Def Node" << std::endl;
		oss << "name: " << _node->get_name_() << std::endl;// << std::endl;

	}
		break;
	case Var_Def_Node: {
		std::shared_ptr<VarDefNode> const &_node = std::dynamic_pointer_cast<VarDefNode>(node);
		oss << "type: " << "Var Def Node" << std::endl;
		oss << "name: " << _node->get_name_() << std::endl;
		oss << "variable_type: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getVariableType()), oss);
		std::vector<std::shared_ptr<VarTypeNode> >const &var_type = _node->getVarType();
		oss << "var_type: ";
		for (std::vector<std::shared_ptr<VarTypeNode> >::const_iterator it = var_type.begin(), end = var_type.end(); it != end; ++it) {
			node2json_dump_(std::dynamic_pointer_cast<Node>(*it), oss);
		}
		oss << std::endl;
		std::vector<std::shared_ptr<Node> >const &var_init_value = _node->getVarInitValue();
		oss << "var_init_value: ";
		for (std::vector<std::shared_ptr<Node> >::const_iterator it = var_init_value.begin(), end = var_init_value.end(); it != end; ++it) {
			if (*it)
				node2json_dump_(std::dynamic_pointer_cast<Node>(*it), oss);
		}
		oss << std::endl;
		oss << "is_const: " << _node->isIsConst() << std::endl;// << std::endl;

	}
		break;
	case Func_Def_Node: {
		std::shared_ptr<FunDefNode> const &_node = std::dynamic_pointer_cast<FunDefNode>(node);
		oss << "type: " << "Func Def Node" << std::endl;
		oss << "name: " << _node->get_name_() << std::endl;
		oss << "return_type: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getReturnType()), oss);
		std::vector<std::shared_ptr<VarDefNode> >const &parameters = _node->getParameters();
		oss << "parameters: ";
		for (std::vector<std::shared_ptr<VarDefNode> >::const_iterator it = parameters.begin(), end = parameters.end(); it != end; ++it) {
			node2json_dump_(std::dynamic_pointer_cast<Node>(*it), oss);
		}
		oss << std::endl;
		oss << "body: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getBody()), oss);
	}
		break;
	case Refer_Node: {
		std::shared_ptr<RefNode> const &_node = std::dynamic_pointer_cast<RefNode>(node);
		oss << "type: " << "Refer Node" << std::endl;
		oss << "name: " << _node->get_name_() << std::endl;// << std::endl;

	}
		break;
	case Var_Node: {
		std::shared_ptr<VarNode> const &_node = std::dynamic_pointer_cast<VarNode>(node);
		oss << "type: " << "Var Node" << std::endl;
		oss << "name: " << _node->get_name_() << std::endl;// << std::endl;

	}
		break;
	case Func_Node: {
		std::shared_ptr<FunNode> const &_node = std::dynamic_pointer_cast<FunNode>(node);
		oss << "type: " << "Func Node" << std::endl;
		oss << "name: " << _node->get_name_() << std::endl;
		std::vector<std::shared_ptr<Node> >const &parameters = _node->getParameters();
		oss << "parameters: ";
		for (std::vector<std::shared_ptr<Node> >::const_iterator it = parameters.begin(), end = parameters.end(); it != end; ++it) {
			node2json_dump_(std::dynamic_pointer_cast<Node>(*it), oss);
		}
		oss << std::endl;// << std::endl;

	}
		break;
	case Stmt_Node: {
		std::shared_ptr<StmtNode> const &_node = std::dynamic_pointer_cast<StmtNode>(node);
		oss << "type: " << "Stmt_Node" << std::endl;
		std::shared_ptr<Node> stmt = _node->getStmt();
		if (stmt) {
			oss << "stmt: ";
			node2json_dump_(std::dynamic_pointer_cast<Node>(stmt), oss);
		}
		oss << std::endl;
	}
		break;
	case Insert_Node: {
		std::shared_ptr<ParenthesisNode> const &_node = std::dynamic_pointer_cast<ParenthesisNode>(node);
		oss << "type: " << "Parent Node" << std::endl;
		oss << "node: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getNode()), oss);

	}
		break;
	case Dig_Node: {
		std::shared_ptr<LiteralNode> const &_node = std::dynamic_pointer_cast<LiteralNode>(node);
		oss << "type: " << "Dig_Node" << std::endl;
		oss << "value: " << _node->getValue() << std::endl;// << std::endl;

	}
		break;
	case Array_Node: {
		std::shared_ptr<ArrayNode> const &_node = std::dynamic_pointer_cast<ArrayNode>(node);
		oss << "type: " << "Array_Node" << std::endl;
		std::vector<std::shared_ptr<Node> >const &array = _node->getArray();
		oss << "array: ";
		for (std::vector<std::shared_ptr<Node> >::const_iterator it = array.begin(), end = array.end(); it != end; ++it) {
			node2json_dump_(std::dynamic_pointer_cast<Node>(*it), oss);
		}
		oss << std::endl;// << std::endl;

	}
		break;
	case Array_Access_Node: {
		std::shared_ptr<ArrayAccessNode> const &_node = std::dynamic_pointer_cast<ArrayAccessNode>(node);
		oss << "type: " << "Array Access Node" << std::endl;
		std::vector<std::shared_ptr<Node> >const &indexes = _node->getIndexes();
		oss << "indexes: ";
		build_array(indexes, oss);
		oss << std::endl;
		oss << "target: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getTarget()), oss);
	}
		break;
	case Op_Node: {
		std::shared_ptr<OperationNode> const &_node = std::dynamic_pointer_cast<OperationNode>(node);
		oss << "type: " << "Op Node" << std::endl;
		Op_Type operator_type = _node->getOperatorType();
		oss << "operator_type: ";
		switch (operator_type) {
		case Unary_Pos_Op:
			oss << "+";
			break;
		case Unary_Neg_Op:
			oss << "-";
			break;
		case Not_Op:
			oss << "!";
			break;
		case Mul_Op:
			oss << "*";
			break;
		case Div_Op:
			oss << "/";
			break;
		case Mod_Op:
			oss << "%";
			break;
		case Plus_Op:
			oss << "+";
			break;
		case Minus_Op:
			oss << "-";
			break;
		case Greater_Op:
			oss << ">";
			break;
		case Less_Op:
			oss << "<";
			break;
		case GEqual_Op:
			oss << ">=";
			break;
		case LEqual_Op:
			oss << "<=";
			break;
		case Equal_Op:
			oss << "==";
			break;
		case NEqual_Op:
			oss << "!=";
			break;
		case And_Op:
			oss << "&&";
			break;
		case Or_Op:
			oss << "||";
			break;
		case Assign_Op:
			oss << "=";
			break;
		default:
			oss << "Unknown Operator!";
			break;
		}
		oss << std::endl;
		std::vector<std::shared_ptr<Node> >const &parameters = _node->getParameters();
		oss << "parameters: ";
		build_array(parameters, oss);
		oss << std::endl;// << std::endl;
	}
		break;
	case If_Else_Node: {
		std::shared_ptr<IfElseNode> const &_node = std::dynamic_pointer_cast<IfElseNode>(node);
		oss << "type: " << "If Else Node" << std::endl;
		oss << "condition: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getCondition()), oss);
		oss << "if body: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getIfBody()), oss);
		if (_node->getElseBody()) {
			oss << "else body: ";
			node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getElseBody()), oss);
		}
		oss << std::endl;
	}
		break;
	case While_Node: {
		std::shared_ptr<WhileNode> const &_node = std::dynamic_pointer_cast<WhileNode>(node);
		oss << "type: " << "While Node" << std::endl;
		oss << "condition: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getCondition()), oss);
		oss << "body: ";
		node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getBody()), oss);
		oss << std::endl;// << std::endl;
	}
		break;
	case Continue_Node: {
		std::shared_ptr<ContinueNode> const &_node = std::dynamic_pointer_cast<ContinueNode>(node);
		oss << "type: " << "Continue Node" << std::endl;// << std::endl;

	}
		break;
	case Break_Node: {
		std::shared_ptr<BreakNode> const &_node = std::dynamic_pointer_cast<BreakNode>(node);
		oss << "type: " << "Break Node" << std::endl;// << std::endl;

	}
		break;
	case Return_Node: {
		std::shared_ptr<ReturnNode> const &_node = std::dynamic_pointer_cast<ReturnNode>(node);
		oss << "type: " << "Return Node" << std::endl;
		if (_node->getReturnValue()) {
			oss << "return value: ";
			node2json_dump_(std::dynamic_pointer_cast<Node>(_node->getReturnValue()), oss);
		}
		oss << std::endl;
	}
		break;
	case Block_Node: {
		std::shared_ptr<BlockNode> const &_node = std::dynamic_pointer_cast<BlockNode>(node);
		oss << "type: " << "Block Node" << std::endl;
		std::vector<std::shared_ptr<Node> >const &children = _node->getChildren();
		oss << "children: ";
		build_array(children, oss);
		oss << std::endl;// << std::endl;
	}
		break;
	case Unknown_Node:{
		oss << "type: " << "Unknown Node" << std::endl;// << std::endl;
	}
		break;
	default: return false;
	}
	oss << std::endl;
	return true;
}
