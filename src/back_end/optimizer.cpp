#include "optimizer.h"
#include <iostream>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <cstring>
using namespace std;
char const *last_optimize_error_msg = nullptr;

//
template<typename OPERATE>
bool calculation(std::vector<std::shared_ptr<Node> > const &parameters, std::shared_ptr<Node> const &parent, OPERATE operate, std::shared_ptr<LiteralNode> &literal) {
	std::shared_ptr<LiteralNode> operand0;
	std::shared_ptr<LiteralNode> operand1;
	if (!evaluate_dig_node(parameters[0], operand0))
		return false;
	if (!evaluate_dig_node(parameters[1], operand1))
		return false;
	literal = std::shared_ptr<LiteralNode>(new LiteralNode(parent, Dig_Node, calculation(operand0->getValue(), operand1->getValue(), operate)));
	return true;
}

//
bool evaluate_dig_node(std::shared_ptr<Node> const &node, std::shared_ptr<LiteralNode> &digit) {
	last_optimize_error_msg = nullptr;
	Node_Type type = node->get_type_();
	switch (type) {
	case Insert_Node: {
		std::shared_ptr<ParenthesisNode> _node = std::dynamic_pointer_cast<ParenthesisNode>(node);
		if (!evaluate_dig_node(std::dynamic_pointer_cast<Node>(_node->getNode()), digit))
			return false;
	}
		break;
	case Dig_Node: {
		std::shared_ptr<LiteralNode> _node = std::dynamic_pointer_cast<LiteralNode>(node);
		digit = std::shared_ptr<LiteralNode>(new LiteralNode(_node));
	}
		break;
	case Op_Node: {
		std::shared_ptr<OperationNode> _node = std::dynamic_pointer_cast<OperationNode>(node);
		Op_Type operator_type = _node->getOperatorType();
		std::vector<std::shared_ptr<Node>> const &parameters = _node->getParameters();
		std::shared_ptr<Node> const &parent = _node->getParent();
		switch (operator_type) {
		case Unary_Pos_Op:
			if (!evaluate_dig_node(parameters[0], digit))
				return false;
			digit->setParent(_node->getParent());
			break;
		case Unary_Neg_Op:
			if (!evaluate_dig_node(parameters[0], digit))
				return false;
			digit->setParent(_node->getParent());
			digit->setValue(-digit->getValue());
			break;
		case Not_Op:
			if (!evaluate_dig_node(parameters[0], digit))
				return false;
			digit->setParent(_node->getParent());
			digit->setValue(!digit->getValue());
			break;
		case Mul_Op:
			if (!calculation(parameters, parent, OPERATE_MUL(), digit))
				return false;
			break;
		case Div_Op:
			if (!calculation(parameters, parent, OPERATE_DIV(), digit))
				return false;
			break;
		case Mod_Op:
			if (!calculation(parameters, parent, OPERATE_MOD(), digit))
				return false;
			break;
		case Plus_Op:
			if (!calculation(parameters, parent, OPERATE_ADD(), digit))
				return false;
			break;
		case Minus_Op:
			if (!calculation(parameters, parent, OPERATE_SUB(), digit))
				return false;
			break;
		case Greater_Op:
			if (!calculation(parameters, parent, OPERATE_GREATER(), digit))
				return false;
			break;
		case Less_Op:
			if (!calculation(parameters, parent, OPERATE_LESS(), digit))
				return false;
			break;
		case GEqual_Op:
			if (!calculation(parameters, parent, OPERATE_GREATER_EQUAL(), digit))
				return false;
			break;
		case LEqual_Op:
			if (!calculation(parameters, parent, OPERATE_LESS_EQUAL(), digit))
				return false;
			break;
		case Equal_Op:
			if (!calculation(parameters, parent, OPERATE_EQUAL(), digit))
				return false;
			break;
		case NEqual_Op:
			if (!calculation(parameters, parent, OPERATE_NOT_EQUAL(), digit))
				return false;
			break;
		case And_Op:
			if (!calculation(parameters, parent, OPERATE_LOGICAL_AND(), digit))
				return false;
			break;
		case Or_Op:
			if (!calculation(parameters, parent, OPERATE_LOGICAL_OR(), digit))
				return false;
			break;
		case Assign_Op:
			last_optimize_error_msg = "Expression cannot contain assignment operation!";
			return false;
			break;
		default:
			last_optimize_error_msg = "Unknown operation!";
			return false;
			break;
		}
	}
		break;
	case Var_Node: {
		std::shared_ptr<VarNode> _node = std::dynamic_pointer_cast<VarNode>(node);
		std::shared_ptr<VarTypeNode> const &target_var = _node->getTargetVar();
		std::shared_ptr<VarDefNode> const &var_def = std::dynamic_pointer_cast<VarDefNode>(target_var->getParent());
		if (!var_def->isIsConst()) {
			last_optimize_error_msg = "Target variable is not const in evaluate_dig_node!!!";
			return false;
		}
		std::vector<std::shared_ptr<VarTypeNode>> const &var_type = var_def->getVarType();
		int var_type_size = var_type.size();
		int index = -1;
		while (++index < var_type_size && target_var != var_type[index])
			;
		if (index >= var_type_size) {
			last_optimize_error_msg = "target_var is not in it parent's var_type vector!!!";
			return false;
		}
		std::shared_ptr<Node> const &_inti_value = var_def->getVarInitValue()[index];
		if (Dig_Node != _inti_value->get_type_()) {
			last_optimize_error_msg = "Const variable init value is not literal!!!";
			return false;
		}
		std::shared_ptr<LiteralNode> const &inti_value = std::dynamic_pointer_cast<LiteralNode>(_inti_value);
		digit = std::shared_ptr<LiteralNode>(new LiteralNode(inti_value));
	}
		break;
	case Type_Node:
	case Var_Type_Node:
	case Def_Node:
	case Var_Def_Node:
	case Func_Def_Node:
	case Refer_Node:
	case Func_Node:
	case Stmt_Node:
	case If_Else_Node:
	case While_Node:
	case Continue_Node:
	case Break_Node:
	case Return_Node:
	case Block_Node:
	case Array_Node:
	case Array_Access_Node:
	case Unknown_Node:
	default:
		last_optimize_error_msg = "Not supported node type in ex calculating!";
		return false;
	}
	return true;
}

//
template<class NODE_TYPE>
bool array_replace_node_function_call(std::vector<std::shared_ptr<NODE_TYPE> > &nodes, std::vector<std::string> &function_call_name, std::map<std::string, std::shared_ptr<Node>> &function_call) {
	for (typename std::vector<std::shared_ptr<NODE_TYPE>>::iterator it = nodes.begin(), end = nodes.end(); it != end; it++) {
		if (!replace_node_function_call(*it, function_call_name, function_call))
			return false;
	}
	return true;
}

bool is_volatile_function(char const *const function_name) {
	for (int i = 0; i < Len_Sys_Func_List; ++i) {
		char const *const system_fun = Sys_Func_List[i];
		if (!strcmp(system_fun, function_name))
			return true;
	}
	return false;
}

bool replace_node_function_call(std::shared_ptr<Node> &node, std::vector<std::string> &function_call_name, std::map<std::string, std::shared_ptr<Node>> &function_call) {
	Node_Type type = node->get_type_();
	switch (type) {
	case Func_Node: {
		std::shared_ptr<FunNode> _node = std::dynamic_pointer_cast<FunNode>(node);
		if (is_volatile_function(_node->get_name_().c_str()))
			break;
		std::ostringstream oss;
		oss << "_____" << _node->get_name_();

		std::vector<std::shared_ptr<Node>> &parameters = _node->getParameters();
		for (std::vector<std::shared_ptr<Node>>::iterator it = parameters.begin(), end = parameters.end(); it != end; ++it) {
			Node_Type type = (*it)->get_type_();
			switch (type) {
			case Var_Node:
				oss << "_" << std::dynamic_pointer_cast<VarNode>(*it)->get_name_();
				break;
			case Array_Access_Node: {
				std::shared_ptr<ArrayAccessNode> _node = std::dynamic_pointer_cast<ArrayAccessNode>(*it);
				if (!array_replace_node_function_call(_node->getIndexes(), function_call_name, function_call))
					return false;

				std::shared_ptr<Node> const &target = _node->getTarget();
				if (Var_Node == target->get_type_())
					oss << "_" << std::dynamic_pointer_cast<VarNode>(target)->get_name_() << "[]";
			}
				break;
			case Dig_Node:
				oss << "_" << std::dynamic_pointer_cast<LiteralNode>(*it)->getValue();
				break;
			case Func_Node:
				if (!replace_node_function_call(*it, function_call_name, function_call))
					return false;
				oss << "_" << std::dynamic_pointer_cast<VarNode>(*it)->get_name_();
				break;
			case Insert_Node: {
				std::shared_ptr<ParenthesisNode> _node = std::dynamic_pointer_cast<ParenthesisNode>(*it);
				if (!replace_node_function_call(_node->getNode(), function_call_name, function_call))
					return false;
			}
				break;
			case Op_Node: {
				std::shared_ptr<OperationNode> _node = std::dynamic_pointer_cast<OperationNode>(*it);
				if (!array_replace_node_function_call(_node->getParameters(), function_call_name, function_call))
					return false;
				oss << "_operation_type" << _node->getOperatorType();
			}
				break;
			default:
				oss << "_type" << type;
				break;
			}
		}
		std::string name = oss.str();
		if (function_call.find(name) == function_call.end()) {
			function_call_name.push_back(name);
			function_call.insert(std::pair<std::string, std::shared_ptr<Node> >(name, _node));
		}
		std::shared_ptr<VarNode> var_node(new VarNode(node, Var_Node, name.c_str()));
		node = std::dynamic_pointer_cast<Node>(var_node);
	}
		break;
	case Stmt_Node:
	case Insert_Node: {
		std::shared_ptr<ParenthesisNode> _node = std::dynamic_pointer_cast<ParenthesisNode>(node);
		if (!replace_node_function_call(_node->getNode(), function_call_name, function_call))
			return false;
	}
		break;
	case Array_Access_Node: {
		std::shared_ptr<ArrayAccessNode> _node = std::dynamic_pointer_cast<ArrayAccessNode>(node);
		if (!array_replace_node_function_call(_node->getIndexes(), function_call_name, function_call))
			return false;
	}
		break;
	case Op_Node: {
		std::shared_ptr<OperationNode> _node = std::dynamic_pointer_cast<OperationNode>(node);
		if (!array_replace_node_function_call(_node->getParameters(), function_call_name, function_call))
			return false;
	}
		break;

	case Var_Node:
	case Dig_Node:
	case Array_Node:
		break;

	case Type_Node:
	case Var_Type_Node:
	case Var_Def_Node:
	case Func_Def_Node:
	case If_Else_Node:
	case While_Node:
	case Continue_Node:
	case Break_Node:
	case Return_Node:
	case Block_Node:
	case Def_Node:
	case Refer_Node:
	case Unknown_Node:
	default:
		last_optimize_error_msg = "Not supported node type while replace_node_function_call!!";
		return false;
	}
	return true;
}

bool merge_block_function_call(std::shared_ptr<BlockNode> &block) {
	std::vector<std::shared_ptr<Node>> &children = block->getChildren();
	std::map<std::string, std::shared_ptr<Node>> function_call;
	std::vector<std::string> function_call_name;
	int index = 0;
	for (int index = 0, end = children.size(); index < end; ++index) {
		std::shared_ptr<Node> &node = children[index];
		if (Stmt_Node == node->get_type_()) {
			std::shared_ptr<StmtNode> stmt = std::dynamic_pointer_cast<StmtNode>(node);
			std::shared_ptr<Node> &expression = stmt->getStmt();
			if (!replace_node_function_call(expression, function_call_name, function_call))
				return false;

			for (std::vector<std::string>::iterator _it = function_call_name.begin(), _end = function_call_name.end(); _it != _end; ++_it) {
				std::string &first = *_it;
				std::shared_ptr<Node> &second = function_call[*_it];
				if (nullptr == second)
					continue;
				std::shared_ptr<VarTypeNode> var_type_node(new VarTypeNode(nullptr, Var_Type_Node, first.c_str()));
				std::shared_ptr<TypeNode> _type(new TypeNode(nullptr, Type_Node, VA_INT));
				std::shared_ptr<VarDefNode> var_def_node(new VarDefNode(std::dynamic_pointer_cast<Node>(block), Var_Def_Node, "", _type, false));
				var_def_node->add_var(var_type_node, second);
				second->setParent(std::dynamic_pointer_cast<Node>(var_def_node));
				var_type_node->setParent(std::dynamic_pointer_cast<Node>(var_def_node));
				_type->setParent(std::dynamic_pointer_cast<Node>(var_def_node));
				children.insert(children.begin() + index, std::dynamic_pointer_cast<Node>(var_def_node));
				++index;
				++end;
				second = nullptr;
			}
		}
	}
	return true;
}
