#include "semantic_analyzer.h"
#include "../back_end/optimizer.h"
#include "../common/global.h"
using namespace std;
char const *last_semantic_analyze_error_msg = nullptr;

inline bool semantic_analyze_(std::shared_ptr<Node> const &node, int block_id);
bool semantic_analyze_(std::shared_ptr<Node> &node, int block_id);

bool find_previous_node_(std::shared_ptr<Node> const &node, Node_Type type, std::shared_ptr<Node> &_node) {
	std::shared_ptr<Node> parent = node;
	while (parent = parent->getParent()) {
		if (parent->get_type_() != type)
			continue;
		_node = parent;
		return true;
	}
	return false;
}

bool find_previous_while_(std::shared_ptr<Node> const &node, std::shared_ptr<WhileNode> &while_node) {
	std::shared_ptr<Node> _node;
	if (!find_previous_node_(node, While_Node, _node))
		return false;
	while_node = std::dynamic_pointer_cast<WhileNode>(_node);
	return true;
}

bool find_previous_fundef_(std::shared_ptr<Node> const &node, std::shared_ptr<FunDefNode> &fun_def_node) {
	std::shared_ptr<Node> _node;
	if (!find_previous_node_(node, Func_Def_Node, _node))
		return false;
	fun_def_node = std::dynamic_pointer_cast<FunDefNode>(_node);
	return true;
}

bool find_target_var_(std::shared_ptr<VarNode> const &var_node, std::shared_ptr<VarTypeNode> &target_var, bool const is_const) {
	std::shared_ptr<Node> parent = std::dynamic_pointer_cast<Node>(var_node);
	std::string const &name = var_node->get_name_();
	while (parent = parent->getParent()) {
		Node_Type type = parent->get_type_();
		if (Block_Node == type) {
			std::shared_ptr<BlockNode> _parent = std::dynamic_pointer_cast<BlockNode>(parent);
			std::vector<std::shared_ptr<Node> > const &children = _parent->getChildren();
			for (std::vector<std::shared_ptr<Node>>::const_iterator it = children.begin(), end = children.end(); it != end; ++it) {
				if (Var_Def_Node != (*it)->get_type_())
					continue;
				std::vector<std::shared_ptr<VarTypeNode>> const &var_type = std::dynamic_pointer_cast<VarDefNode>(*it)->getVarType();
				for (std::vector<std::shared_ptr<VarTypeNode>>::const_iterator _it = var_type.begin(), end = var_type.end(); _it != end; ++_it) {
					if (name == (*_it)->get_name_()) {
						if (is_const){
							if (std::dynamic_pointer_cast<VarDefNode>(*it)->isIsConst()){
								std::cout << "const cannot be a left value" << std::endl;
								last_semantic_analyze_error_msg = "invalid left value";
								return false;
							}
							else
								return true;
						}
						else{
							target_var = *_it;
							return true;
						}
					}
				}
			}
		} 
		else if (Func_Def_Node == type) {
			std::vector<std::shared_ptr<VarDefNode>> const &var_def = std::dynamic_pointer_cast<FunDefNode>(parent)->getParameters();
			for (std::vector<std::shared_ptr<VarDefNode>>::const_iterator it = var_def.begin(), end = var_def.end(); it != end; ++it) {
				std::vector<std::shared_ptr<VarTypeNode>> const &var_type = (*it)->getVarType();
				for (std::vector<std::shared_ptr<VarTypeNode>>::const_iterator it = var_type.begin(), end = var_type.end(); it != end; ++it) {
					if (name == (*it)->get_name_()) {
						target_var = *it;
						return true;
					}
				}
			}
		}

	}
	return false;
}

bool find_target_fun_(std::shared_ptr<FunNode> const &fun_node, bool &is_system_fun, std::shared_ptr<FunDefNode> &target_fun) {
	is_system_fun = false;
	std::string const &name = fun_node->get_name_();
	for (int i = 0; i < Len_Sys_Func_List; ++i) {
		char const *const system_fun = Sys_Func_List[i];
		if (name == system_fun) {
			is_system_fun = true;
			return true;
		}
	}
	std::shared_ptr<Node> parent = std::dynamic_pointer_cast<Node>(fun_node);
	while (parent = parent->getParent()) {
		Node_Type type = parent->get_type_();
		if (Block_Node != type)
			continue;
		std::shared_ptr<BlockNode> _parent = std::dynamic_pointer_cast<BlockNode>(parent);
		std::vector<std::shared_ptr<Node> > const &children = _parent->getChildren();
		for (std::vector<std::shared_ptr<Node>>::const_iterator it = children.begin(), end = children.end(); it != end; ++it) {
			if (Func_Def_Node != (*it)->get_type_())
				continue;
			std::shared_ptr<FunDefNode> const &current_fun = std::dynamic_pointer_cast<FunDefNode>(*it);
			if (current_fun->get_name_() == name) {
				target_fun = current_fun;
				return true;
			}
		}
	}
	return false;
}

template<class NODE_TYPE>
bool array_semantic_analyze(std::vector<std::shared_ptr<NODE_TYPE>> &nodes, int block_id, bool allow_nullptr = false) {
	for (typename std::vector<std::shared_ptr<NODE_TYPE>>::iterator it = nodes.begin(), end = nodes.end(); it != end; ++it) {
		if (allow_nullptr && !*it)
			continue;
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(*it), block_id))
			return false;
	}
	return true;
}

inline bool semantic_analyze_(std::shared_ptr<Node> const &node, int block_id) {
	std::shared_ptr<Node> __node(node);
	return semantic_analyze_(__node, block_id);
}

bool semantic_analyze_(std::shared_ptr<Node> &node, int block_id) {
	last_semantic_analyze_error_msg = nullptr;
	Node_Type type = node->get_type_();
	switch (type) {
	case Type_Node: {
		std::shared_ptr<TypeNode> _node = std::dynamic_pointer_cast<TypeNode>(node);
		std::vector<std::shared_ptr<Node>> &dimensions = _node->getDimensions();
		for (std::vector<std::shared_ptr<Node>>::iterator it = dimensions.begin(), end = dimensions.end(); it != end; ++it) {
			std::shared_ptr<LiteralNode> literal;
			if (!evaluate_dig_node(*it, literal)) {
				last_semantic_analyze_error_msg = last_optimize_error_msg;
				return false;
			}
			literal->setParent(node);
			*it = std::dynamic_pointer_cast<Node>(literal);
		}
	}
		break;
	case Var_Type_Node: {
		std::shared_ptr<VarTypeNode> _node = std::dynamic_pointer_cast<VarTypeNode>(node);
		std::vector<std::shared_ptr<Node>> &dimensions = _node->getDimensions();
		for (std::vector<std::shared_ptr<Node>>::iterator it = dimensions.begin(), end = dimensions.end(); it != end; ++it) {
			std::shared_ptr<LiteralNode> literal;
			if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(*it), block_id))
				return false;
			if (!evaluate_dig_node(*it, literal)) {
				last_semantic_analyze_error_msg = last_optimize_error_msg;
				return false;
			}
			literal->setParent(node);
			*it = std::dynamic_pointer_cast<Node>(literal);
		}
	}
		break;
	case Var_Def_Node: {
		std::shared_ptr<VarDefNode> _node = std::dynamic_pointer_cast<VarDefNode>(node);
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getVariableType()), block_id))
			return false;
		std::vector<std::shared_ptr<VarTypeNode> > &var_type = _node->getVarType();
		if (!array_semantic_analyze(var_type, block_id))
			return false;
		std::vector<std::shared_ptr<Node> > &var_inti_value = _node->getVarInitValue();
		if (!array_semantic_analyze(var_inti_value, block_id, true))
			return false;
		if (_node->isIsConst()) {
			for (int i = 0, end = var_type.size(); i < end; ++i) {
				std::shared_ptr<Node> const &init_value = var_inti_value[i];
				if (init_value && !var_type[i]->getDimensions().size()) {
					std::shared_ptr<LiteralNode> literal;
					if (!evaluate_dig_node(init_value, literal)) {
						last_semantic_analyze_error_msg = last_optimize_error_msg;
						return false;
					}
					literal->setParent(node);
				}
			}
		}
		_node->setBlockId(block_id);
	}
		break;
	case Func_Def_Node: {
		std::shared_ptr<FunDefNode> _node = std::dynamic_pointer_cast<FunDefNode>(node);
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getReturnType()), block_id))
			return false;
		if (!array_semantic_analyze(_node->getParameters(), block_id))
			return false;
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getBody()), block_id))
			return false;
		_node->setBlockId(block_id);
	}
		break;
	case Var_Node: {
		std::shared_ptr<VarNode> _node = std::dynamic_pointer_cast<VarNode>(node);
		std::shared_ptr<VarTypeNode> target_var;
		if (!find_target_var_(_node, target_var, false)) {
			last_semantic_analyze_error_msg = "Cannot find target variable definition!";
			return false;
		}
		_node->setTargetVar(target_var);

	}
		break;
	case Func_Node: {
		std::shared_ptr<FunNode> _node = std::dynamic_pointer_cast<FunNode>(node);
		if (!array_semantic_analyze(_node->getParameters(), block_id))
			return false;
		bool is_system_fun = false;
		std::shared_ptr<FunDefNode> target_fun;
		if (!find_target_fun_(_node, is_system_fun, target_fun)) {
			last_semantic_analyze_error_msg = "Cannot find target function definition!";
			return false;
		}
		_node->setIsSystemFun(is_system_fun);
		_node->setTargetFun(target_fun);

	}
		break;
	case Stmt_Node: {
		std::shared_ptr<StmtNode> _node = std::dynamic_pointer_cast<StmtNode>(node);
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getStmt()), block_id))
			return false;
	}
		break;
	case Insert_Node: {
		std::shared_ptr<ParenthesisNode> _node = std::dynamic_pointer_cast<ParenthesisNode>(node);
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getNode()), block_id))
			return false;

	}
		break;
	case Dig_Node: {
	}
		break;
	case Array_Node: {
		std::shared_ptr<ArrayNode> _node = std::dynamic_pointer_cast<ArrayNode>(node);
		if (!array_semantic_analyze(_node->getArray(), block_id))
			return false;

	}
		break;
	case Array_Access_Node: {
		std::shared_ptr<ArrayAccessNode> _node = std::dynamic_pointer_cast<ArrayAccessNode>(node);
		if (!array_semantic_analyze(_node->getIndexes(), block_id))
			return false;
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getTarget()), block_id))
			return false;
	}
		break;
	case Op_Node: {
		std::shared_ptr<OperationNode> _node = std::dynamic_pointer_cast<OperationNode>(node);
		std::vector<std::shared_ptr<Node> > &parameters = _node->getParameters();

		std::vector<std::shared_ptr<Node>>::iterator it = parameters.begin();
		if (_node->getOperatorType() == Assign_Op && (*it)->get_type_() == Var_Node){
			std::shared_ptr<VarNode> _it = std::dynamic_pointer_cast<VarNode>(*it);
			std::shared_ptr<VarTypeNode> target_var;
			if (!find_target_var_(_it, target_var, true)) {
				return false;
			}
		}
		for (std::vector<std::shared_ptr<Node>>::iterator end = parameters.end(); it != end; ++it) {
			if (!semantic_analyze_(*it, block_id))
				return false;
		}

	}
		break;
	case If_Else_Node: {
		std::shared_ptr<IfElseNode> _node = std::dynamic_pointer_cast<IfElseNode>(node);
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getCondition()), block_id))
			return false;
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getIfBody()), block_id))
			return false;
		const std::shared_ptr<BlockNode> &else_body = _node->getElseBody();
		if (else_body)
			if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(else_body), block_id))
				return false;
	}
		break;
	case While_Node: {
		std::shared_ptr<WhileNode> _node = std::dynamic_pointer_cast<WhileNode>(node);
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getCondition()), block_id))
			return false;
		if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(_node->getBody()), block_id))
			return false;
	}
		break;
	case Continue_Node: {
		std::shared_ptr<ContinueNode> _node = std::dynamic_pointer_cast<ContinueNode>(node);
		std::shared_ptr<WhileNode> while_node;
		if (!find_previous_while_(node, while_node)) {
			last_semantic_analyze_error_msg = "continue statement not in while loop!";
			return false;
		}
		_node->setTarget(while_node);

	}
		break;
	case Break_Node: {
		std::shared_ptr<BreakNode> _node = std::dynamic_pointer_cast<BreakNode>(node);
		std::shared_ptr<WhileNode> while_node;
		if (!find_previous_while_(node, while_node)) {
			last_semantic_analyze_error_msg = "break statement not in while loop!";
			return false;
		}
		_node->setTarget(while_node);
	}
		break;
	case Return_Node: {
		std::shared_ptr<ReturnNode> _node = std::dynamic_pointer_cast<ReturnNode>(node);
		std::shared_ptr<FunDefNode> fun_def_node;
		if (!find_previous_fundef_(node, fun_def_node)) {
			last_semantic_analyze_error_msg = "return statement not in function definition node!";
			return false;
		}
		_node->setTarget(fun_def_node);
		const std::shared_ptr<Node> &return_value = _node->getReturnValue();
		if (return_value)
			if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(return_value), block_id))
				return false;
	}
		break;
	case Block_Node: {
		std::shared_ptr<BlockNode> _node = std::dynamic_pointer_cast<BlockNode>(node);
		int block_id = next_global_id();
		_node->setId(block_id);

		merge_block_function_call(_node);

		if (!array_semantic_analyze(_node->getChildren(), block_id))
			return false;
	}
		break;
	case Refer_Node:
	case Def_Node:
	case Unknown_Node:
	default:{
		last_semantic_analyze_error_msg = "Not supported node type!!";
		return false;
	}
	}
	return true;
}

bool semantic_analyze_(std::shared_ptr<BlockNode> &root) {
	last_semantic_analyze_error_msg = nullptr;
	if (!semantic_analyze_(std::dynamic_pointer_cast<Node>(root), -1))
		return false;

	std::shared_ptr<FunDefNode> main;
	std::dynamic_pointer_cast<FunDefNode>(main);
	std::shared_ptr<BlockNode> block_node(new BlockNode(nullptr, Block_Node));

	std::vector<std::shared_ptr<Node> > const &children = root->getChildren();
	for (std::vector<std::shared_ptr<Node>>::const_iterator it = children.begin(), end = children.end(); it != end; ++it) {
		switch ((*it)->get_type_()) {
		case Var_Def_Node: {
			std::shared_ptr<VarDefNode> var_def = std::dynamic_pointer_cast<VarDefNode>(*it);
			var_def->setIsGlobal(true);
			std::vector<std::shared_ptr<VarTypeNode>> const &var_type = var_def->getVarType();
			std::vector<std::shared_ptr<Node>> const &var_init_value = var_def->getVarInitValue();
			int index = 0;
			for (std::vector<std::shared_ptr<VarTypeNode>>::const_iterator it = var_type.begin(), end = var_type.end(); it != end; ++it) {
				std::vector<std::shared_ptr<Node>> &dimensions = (*it)->getDimensions();
				if (dimensions.size() && !var_init_value[index]) {
					int total_size = 1;
					for (std::vector<std::shared_ptr<Node>>::iterator it = dimensions.begin(), end = dimensions.end(); it != end; ++it) {
						if (Dig_Node == (*it)->get_type_())
							total_size *= std::dynamic_pointer_cast<LiteralNode>(*it)->getValue();
						else {
							last_semantic_analyze_error_msg = "Array dimension must be literal!";
							return false;
						}
					}
					std::shared_ptr<LiteralNode> literal(new LiteralNode(nullptr, Dig_Node, total_size * 4));
					std::shared_ptr<FunNode> fun_node(new FunNode(nullptr, Func_Node, "malloc", literal));
					std::shared_ptr<VarNode> var_node(new VarNode(nullptr, Var_Node, (*it)->get_name_().c_str()));
					std::shared_ptr<OperationNode> expr_node(new OperationNode(nullptr, Op_Node, Assign_Op, var_node, fun_node));
					std::shared_ptr<StmtNode> stmt_node(new StmtNode(std::dynamic_pointer_cast<Node>(block_node), Stmt_Node, expr_node));
					block_node->add_child(std::dynamic_pointer_cast<Node>(stmt_node));
					expr_node->setParent(std::dynamic_pointer_cast<Node>(stmt_node));
					var_node->setParent(std::dynamic_pointer_cast<Node>(expr_node));
					fun_node->setParent(std::dynamic_pointer_cast<Node>(expr_node));
					literal->setParent(std::dynamic_pointer_cast<Node>(fun_node));
					fun_node->setIsSystemFun(true);
					var_node->setTargetVar(*it);
				}
				++index;
			}
		}
			break;
		case Func_Def_Node: {
			std::shared_ptr<FunDefNode> fun = std::dynamic_pointer_cast<FunDefNode>(*it);
			if ("main" == fun->get_name_())
				main = std::dynamic_pointer_cast<FunDefNode>(*it);
		}
			break;
		}
	}
	std::shared_ptr<BlockNode> old_body = main->getBody();
	std::vector<std::shared_ptr<Node> > const &_children = old_body->getChildren();
	main->setBody(block_node);
	for (std::vector<std::shared_ptr<Node>>::const_iterator it = _children.begin(), end = _children.end(); it != end; ++it) {
		block_node->add_child(*it);
		(*it)->setParent(std::dynamic_pointer_cast<Node>(block_node));
	}
	block_node->setParent(std::dynamic_pointer_cast<Node>(main));
	return true;
}

