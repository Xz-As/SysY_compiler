#include <sstream>
#include <algorithm>
#include <iostream>
using namespace std;
#include "arm_generator.h"
#include "../common/global.h"
#include "optimizer.h"

char const *last_arm_generate_error_msg = nullptr;

template<class NODE_TYPE>
bool array_gen_arm_assembly(std::vector<std::shared_ptr<NODE_TYPE>> &nodes, std::ostringstream &oss) {
	for (typename std::vector<std::shared_ptr<NODE_TYPE>>::iterator it = nodes.begin(), end = nodes.end(); it != end; ++it) {
		if (!gen_arm_assembly(std::dynamic_pointer_cast<Node>(*it), oss))
			return false;
	}
	return true;
}

int is_valid_immediate_value(int _value, int &new_value) {
	unsigned int value;
	if (_value < 0)
		value = ~_value;
	else
		value = _value;
	for (int i = 0; i < 32; i += 2) {
		if (value < 0x100) {
			new_value = value;
			return i;
		}
		value = value << 2 | value >> 30;
	}
	return -1;
}
inline bool is_valid_immediate_value(int _value) {
	int new_value;
	return -1 < is_valid_immediate_value(_value, new_value);
}

bool registers_assignment[10] = { };
const int registers_assignment_len = 10;

int allocate_register(int specified_register, std::ostringstream &oss) {
	if (registers_assignment[specified_register]){
		registers_assignment[specified_register] = true;
		oss << "\tpush\t{r" << specified_register << "}" << std::endl;
		return specified_register;
	}
	std::cout << "no register left! invalid assignment of r11!";
	last_arm_generate_error_msg = "no valid register";
	return 11;
}

int allocate_register(std::ostringstream &oss) {
	for (int i = registers_assignment_len - 1; i >= 0; --i) {
		if (!registers_assignment[i]) {
			registers_assignment[i] = true;
			return i;
		}
	}
	return allocate_register(9, oss);
}

void deallocate_register(int _register, std::ostringstream &oss) {
	if (registers_assignment[_register]){
		registers_assignment[_register] = false;
		oss << "\tpop\t{r" << _register << "}" << std::endl;
	}
}

int calculate_function_local_variable_size() {
	return -1;
}

long long get_array_total_size(std::vector<std::shared_ptr<Node>> const &dimensions) {
	long long size = 1;
	for (std::vector<std::shared_ptr<Node>>::const_iterator it = dimensions.begin(), end = dimensions.end(); it != end; ++it) {
		if (Dig_Node != (*it)->get_type_()) {
			last_arm_generate_error_msg = "The array dimension is not literal!!";
			return false;
		}
		int dimension = std::dynamic_pointer_cast<LiteralNode>(*it)->getValue();
		if (dimension < 0)
			return -1;
		size *= dimension;
	}
	return size;
}

bool get_dimensions(std::vector<std::shared_ptr<Node>> const &_dimensions, std::vector<int> &dimensions) {
	for (std::vector<std::shared_ptr<Node>>::const_iterator it = _dimensions.begin(), end = _dimensions.end(); it != end; ++it) {
		if (Dig_Node != (*it)->get_type_()) {
			last_arm_generate_error_msg = "The dimension is not digit!!";
			return false;
		}
		dimensions.push_back(std::dynamic_pointer_cast<LiteralNode>(*it)->getValue());
	}
	return true;
}

bool literal_vector2int_vector(std::vector<std::shared_ptr<Node>> const &literal_vector, std::vector<int> &int_vector) {
	for (std::vector<std::shared_ptr<Node>>::const_iterator it = literal_vector.begin(), end = literal_vector.end(); it != end; ++it) {
		if (Dig_Node != (*it)->get_type_()) {
			last_arm_generate_error_msg = "The vector element is not digit!!";
			return false;
		}
		int_vector.push_back(std::dynamic_pointer_cast<LiteralNode>(*it)->getValue());
	}
	return true;
}

bool gen_array_init_value(std::vector<int> const &dimensions, std::shared_ptr<ArrayNode> const &init_value, std::vector<long long> &_init_value, std::vector<int> &init_array_index, int dimensions_index) {
	std::vector<std::shared_ptr<Node>> const &array = init_value->getArray();
	if (dimensions_index >= dimensions.size()) {
		int index = init_array_index.back();
		if (array.size() > index) {
			switch (array[index]->get_type_()) {
			case Dig_Node:
				_init_value.push_back(std::dynamic_pointer_cast<LiteralNode>(array[index])->getValue());
				++init_array_index.back();
				break;
			case Array_Node:
				_init_value.push_back(0);
				break;
			default:
				std::shared_ptr<LiteralNode> literal;
				if (!evaluate_dig_node(array[index], literal)) {
					last_arm_generate_error_msg = "Array element cannot evaluated to literal!";
					return false;
				}
				literal->setParent(std::dynamic_pointer_cast<LiteralNode>(init_value));
				_init_value.push_back(literal->getValue());
				++init_array_index.back();
				break;
			}
		}
		else {
			_init_value.push_back(0);
		}
	}
	else
		for (int i = 0, end = dimensions[dimensions_index]; i < end; ++i) {
			int index = init_array_index.back();
			if (init_array_index.size() - 1 == dimensions_index && array.size() > index && Array_Node == array[index]->get_type_()) {
				init_array_index.push_back(0);
				if (!gen_array_init_value(dimensions, std::dynamic_pointer_cast<ArrayNode>(array[index]), _init_value, init_array_index, dimensions_index + 1))
					return false;
				init_array_index.pop_back();
				++init_array_index.back();
			}
			else {
				if (!gen_array_init_value(dimensions, init_value, _init_value, init_array_index, dimensions_index + 1))
					return false;
			}
		}
	return true;
}

bool gen_array_init_value(std::vector<int> const &dimensions, std::shared_ptr<ArrayNode> const &init_value, std::vector<std::shared_ptr<Node>> &_init_value, std::vector<int> &init_array_index, int dimensions_index) {
	std::vector<std::shared_ptr<Node>> const &array = init_value->getArray();
	if (dimensions_index >= dimensions.size()) {
		int index = init_array_index.back();
		if (array.size() > index) {
			if (Array_Node != array[index]->get_type_()) {
				_init_value.push_back(array[index]);
				++init_array_index.back();
				return true;
			}
		}
		std::shared_ptr<LiteralNode> zero(new LiteralNode(nullptr, Dig_Node, 0));
		_init_value.push_back(std::dynamic_pointer_cast<Node>(zero));
		return true;
	}
	for (int i = 0, end = dimensions[dimensions_index]; i < end; ++i) {
		int index = init_array_index.back();
		if (init_array_index.size() - 1 == dimensions_index && array.size() > index && Array_Node == array[index]->get_type_()) {
			init_array_index.push_back(0);
			if (!gen_array_init_value(dimensions, std::dynamic_pointer_cast<ArrayNode>(array[index]), _init_value, init_array_index, dimensions_index + 1))
				return false;
			init_array_index.pop_back();
			++init_array_index.back();
		} else {
			if (!gen_array_init_value(dimensions, init_value, _init_value, init_array_index, dimensions_index + 1))
				return false;
		}
	}
	return true;
}

bool gen_array_init_value(std::vector<int> const &dimensions, std::shared_ptr<ArrayNode> &init_value, std::vector<std::shared_ptr<Node>> &_init_value) {
	std::vector<int> init_array_index = { 0 };
	return gen_array_init_value(dimensions, init_value, _init_value, init_array_index, 0);
}

bool gen_array_init_value(std::vector<int> const &dimensions, std::shared_ptr<ArrayNode> &init_value, std::vector<long long> &_init_value) {
	std::vector<int> init_array_index = { 0 };
	return gen_array_init_value(dimensions, init_value, _init_value, init_array_index, 0);
}

bool calculate_block_var_offset(std::shared_ptr<BlockNode> &block, int &local_var_total_size) {
	std::vector<std::shared_ptr<Node> > &children = block->getChildren();
	for (std::vector<std::shared_ptr<Node> >::iterator it = children.begin(), end = children.end(); it != end; ++it) {
		Node_Type type = (*it)->get_type_();
		switch (type) {
		case Var_Def_Node: {
			std::shared_ptr<VarDefNode> _node = std::dynamic_pointer_cast<VarDefNode>(*it);
			std::vector<std::shared_ptr<VarTypeNode> > &var_type = _node->getVarType();
			for (std::vector<std::shared_ptr<VarTypeNode> >::iterator it = var_type.begin(), end = var_type.end(); it != end; ++it) {
				std::vector<std::shared_ptr<Node> > &dimensions = (*it)->getDimensions();
				long long size = 0;
				if (dimensions.size())
					size = get_array_total_size(dimensions);
				if (-1 == size) {
					last_arm_generate_error_msg = "Array must specify the size!!";
					return false;
				}
				_node->add_var_offset(-(local_var_total_size += (size + 1) * 4));
			}
		}
			break;
		case If_Else_Node: {
			std::shared_ptr<IfElseNode> _node = std::dynamic_pointer_cast<IfElseNode>(*it);
			std::shared_ptr<BlockNode> block = _node->getIfBody();
			if (!calculate_block_var_offset(block, local_var_total_size))
				return false;
			block = _node->getElseBody();
			if (block)
				if (!calculate_block_var_offset(block, local_var_total_size))
					return false;
		}
			break;
		case While_Node: {
			std::shared_ptr<WhileNode> _node = std::dynamic_pointer_cast<WhileNode>(*it);
			std::shared_ptr<BlockNode> block = _node->getBody();
			if (!calculate_block_var_offset(block, local_var_total_size))
				return false;
		}
			break;
		case Block_Node: {
			std::shared_ptr<BlockNode> block = std::dynamic_pointer_cast<BlockNode>(*it);
			if (!calculate_block_var_offset(block, local_var_total_size))
				return false;
		}
			break;
		default:
			break;
		}
	}
	return true;
}

bool calculate_funtion_var_offset(std::shared_ptr<FunDefNode> &fun_def, int &local_var_total_size) {
	std::vector<std::shared_ptr<VarDefNode> > &parameters = fun_def->getParameters();
	for (std::vector<std::shared_ptr<VarDefNode> >::iterator it = parameters.begin(), end = parameters.end(); it != end; ++it) {
		std::vector<std::shared_ptr<VarTypeNode>> &var_type = (*it)->getVarType();
		if (1 != var_type.size()) {
			last_arm_generate_error_msg = "Incorrect function parameter format!";
			return false;
		}
		(*it)->add_var_offset(-(local_var_total_size += 4));
	}
	std::shared_ptr<BlockNode> body = fun_def->getBody();
	return calculate_block_var_offset(body, local_var_total_size);
}

bool get_target_var_offset(std::shared_ptr<VarTypeNode> const &target_var, int &offset) {
	std::shared_ptr<Node> const &_parent = target_var->getParent();
	if (_parent->get_type_() != Var_Def_Node) {
		last_arm_generate_error_msg = "target_var's parent must be VarDefNode type!";
		return false;
	}
	std::shared_ptr<VarDefNode> const &parent = std::dynamic_pointer_cast<VarDefNode>(_parent);
	std::vector<std::shared_ptr<VarTypeNode> > const &var_type = parent->getVarType();
	int index = 0;
	for (std::vector<std::shared_ptr<VarTypeNode> >::const_iterator it = var_type.begin(), end = var_type.end(); it != end; ++it) {
		if ((*it) == target_var) {
			offset = parent->getVarOffset()[index];
			return true;
		}
		++index;
	}
	last_arm_generate_error_msg = "Cannot find target_var!";
	return false;
}

long long get_array_access_index(std::shared_ptr<VarTypeNode> const &target_var, std::vector<std::shared_ptr<Node>> const &_indexes) {
	std::vector<int> dimensions;
	if (!literal_vector2int_vector(target_var->getDimensions(), dimensions))
		return -1;
	std::vector<int> indexes;
	if (!literal_vector2int_vector(_indexes, indexes))
		return -1;
	int indexes_size = indexes.size();
	int dimensions_size = dimensions.size();
	if (dimensions_size < indexes_size) {
		last_arm_generate_error_msg = "Array index out of boundary!";
		return -1;
	}
	long long index = 0;
	index = indexes[0];
	for (int i = 1, end = dimensions_size; i < end; ++i) {
		index = index * dimensions[i] + indexes[i];
	}
	return index;
}

bool get_array_access_index_expression(std::shared_ptr<VarTypeNode> const &target_var, std::vector<std::shared_ptr<Node>> const &indexes, std::shared_ptr<Node> &expression, bool &partitial_access) {
	std::vector<int> dimensions;
	if (!literal_vector2int_vector(target_var->getDimensions(), dimensions))
		return false;

	int indexes_size = indexes.size();
	int dimensions_size = dimensions.size();
	if (dimensions_size < indexes_size) {
		last_arm_generate_error_msg = "Array index out of boundary!";
		return false;
	}
	partitial_access = indexes_size != dimensions_size;
	std::shared_ptr<Node> index = indexes[0];
	for (int i = 1, end = dimensions_size; i < end; ++i) {
		std::shared_ptr<LiteralNode> dimension(new LiteralNode(nullptr, Dig_Node, dimensions[i]));
		std::shared_ptr<Node> mul = std::dynamic_pointer_cast<Node>(
		std::shared_ptr<OperationNode>(new OperationNode(nullptr, Op_Node, Mul_Op, index, std::dynamic_pointer_cast<Node>(dimension))));
		dimension->setParent(mul);
		index->setParent(mul);
		std::shared_ptr<Node> add = std::dynamic_pointer_cast<Node>(
				std::shared_ptr<OperationNode>(new OperationNode(nullptr, Op_Node, Plus_Op, mul, i < indexes_size ? indexes[i] : std::dynamic_pointer_cast<Node>(std::shared_ptr<LiteralNode>(new LiteralNode(nullptr, Dig_Node, 0))))));
		mul->setParent(add);
		if (i < indexes_size)
			indexes[i]->setParent(add);
		index = add;
	}
	expression = index;
	return true;
}

std::shared_ptr<Node> get_var_type_init_value(std::shared_ptr<VarTypeNode> const &type) {
	std::shared_ptr<VarDefNode> var_def = std::dynamic_pointer_cast<VarDefNode>(type->getParent());
	std::vector<std::shared_ptr<VarTypeNode> >const &var_type = var_def->getVarType();
	std::vector<std::shared_ptr<Node> >const &var_init_value = var_def->getVarInitValue();
	for (int i = 0, end = var_type.size(); i < end; ++i) {
		if (type == var_type[i])
			return var_init_value[i];
	}
	return nullptr;
}

bool gen_arm_assembly(std::shared_ptr<Node> const &node, std::ostringstream &oss) {
	last_arm_generate_error_msg = nullptr;
	Node_Type type = node->get_type_();
	switch (type) {
	case Var_Def_Node: {
		std::shared_ptr<VarDefNode> _node = std::dynamic_pointer_cast<VarDefNode>(node);
		if (_node->isIsGlobal()) {
			std::vector<std::shared_ptr<VarTypeNode> >const &var_type = _node->getVarType();
			std::vector<std::shared_ptr<Node> >const &var_init_value = _node->getVarInitValue();
			oss << "\t.data " << std::endl;
			for (int i = 0, end = var_type.size(); i < end; ++i) {
				std::shared_ptr<VarTypeNode> const &type = var_type[i];
				std::vector<std::shared_ptr<Node>> const &dimensions = type->getDimensions();
				std::shared_ptr<Node> const &_init_value = var_init_value[i];
				std::vector<int> _dimensions;
				if (!get_dimensions(dimensions, _dimensions))
					return false;
				if (dimensions.size())
					if (_init_value) {
						if (Array_Node != _init_value->get_type_()) {
							last_arm_generate_error_msg = "Initialize of array must use digit!";
							return false;
						}
						std::shared_ptr<ArrayNode> init_value = std::dynamic_pointer_cast<ArrayNode>(_init_value);
						std::vector<long long> __init_value;
						if (!gen_array_init_value(_dimensions, init_value, __init_value))
							return false;
						oss << type->get_name_() << ":" << std::endl;
						for (std::vector<long long>::iterator it = __init_value.begin(), end = __init_value.end(); it != end; ++it) {
							oss << "\t.word	" << (*it) << std::endl;
						}
					}
					else {
						long long size = get_array_total_size(dimensions);
						if (-1 == size) {
							last_arm_generate_error_msg = "Array size unknown!";
							return false;
						}
						oss << type->get_name_() << ":" << std::endl;
						oss << "\t.word\t0" << std::endl;
					}
				else {
					int __init_value = 0;
					if (_init_value) {
						if (Dig_Node != _init_value->get_type_()) {
							last_arm_generate_error_msg = "Initialize of int must use digit!";
							return false;
						}
						std::shared_ptr<LiteralNode> init_value = std::dynamic_pointer_cast<LiteralNode>(_init_value);
						__init_value = init_value->getValue();
					}
					oss << type->get_name_() << ":" << std::endl;
					oss << "\t.word	" << __init_value << std::endl;
				}
			}
			oss << "\t.text" << std::endl;
			for (int i = 0, end = var_type.size(); i < end; ++i) {
				std::shared_ptr<VarTypeNode> const &type = var_type[i];
				std::string const &name = type->get_name_();
				oss << "." << name << "_addr:" << std::endl;
				oss << "\t.word	" << name << std::endl;
			}
			oss << std::endl;
		} else {
			std::vector<std::shared_ptr<VarTypeNode> >const &var_type = _node->getVarType();
			std::vector<std::shared_ptr<Node> >const &var_init_value = _node->getVarInitValue();
			for (int i = 0, end = var_type.size(); i < end; ++i) {
				std::shared_ptr<VarTypeNode> const &type = var_type[i];
				std::vector<std::shared_ptr<Node>> const &dimensions = type->getDimensions();
				std::shared_ptr<Node> const &_init_value = var_init_value[i];
				std::vector<int> _dimensions;
				if (!get_dimensions(dimensions, _dimensions))
					return false;
				if (dimensions.size()) {
					int offset;
					if (!get_target_var_offset(type, offset))
						return false;

					int temp_register = allocate_register(oss);
					int immediate_value = offset + 4;

					if (is_valid_immediate_value(immediate_value)) {
						oss << "\tadd\tr" << temp_register << ", fp, #" << immediate_value << std::endl;
					}
					else {
						oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
						oss << "\tadd\tr" << temp_register << ", fp, r" << temp_register << std::endl;
					}
					oss << "\tstr\tr" << temp_register << ", [r" << temp_register << ", #-4]" << std::endl;
					deallocate_register(temp_register, oss);
					if (_init_value) {
						if (Array_Node != _init_value->get_type_()) {
							last_arm_generate_error_msg = "Initialize of array must be digit!";
							return false;
						}
						std::shared_ptr<ArrayNode> init_value = std::dynamic_pointer_cast<ArrayNode>(_init_value);
						std::vector<std::shared_ptr<Node>> __init_value;

						if (!gen_array_init_value(_dimensions, init_value, __init_value))
							return false;

						int index = 1;

						for (std::vector<std::shared_ptr<Node>>::iterator it = __init_value.begin(), end = __init_value.end(); it != end; ++it) {
							if (!gen_arm_assembly(*it, oss))
								return false;

							int immediate_value = offset + 4 * index;

							if (is_valid_immediate_value(immediate_value)) {
								oss << "\tstr\tr10, [fp, #" << immediate_value << "]" << std::endl;
							}
							else {
								int temp_register = allocate_register(oss);
								oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
								oss << "\tstr\tr10, [fp, r" << temp_register << "]" << std::endl;
								deallocate_register(temp_register, oss);
							}
							++index;
						}
					}
				}
				else {
					if (_init_value) {
						if (!gen_arm_assembly(_init_value, oss))
							return false;

						int offset;

						if (!get_target_var_offset(type, offset))
							return false;

						int immediate_value = offset;

						if (is_valid_immediate_value(immediate_value)) {
							oss << "\tstr\tr10, [fp, #" << immediate_value << "]" << std::endl;
						} 
						else {
							int temp_register = allocate_register(oss);
							oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
							oss << "\tstr\tr10, [fp, r" << temp_register << "]" << std::endl;
							deallocate_register(temp_register, oss);
						}
					}
				}
			}
		}
	}
	break;
	case Func_Def_Node: {
		oss << "\t.text" << std::endl;
		std::shared_ptr<FunDefNode> _node = std::dynamic_pointer_cast<FunDefNode>(node);
		int local_var_total_size = 0;
		if (!calculate_funtion_var_offset(_node, local_var_total_size))
			return false;
		long long return_label_id = next_global_id();
		_node->setReturnLabelId(return_label_id);
		_node->setLocalVarTotalSize(local_var_total_size);
		if (_node->get_name_() != "main"){
			oss << _node->get_name_() << ":" << std::endl;
		} else {
			oss << "main:" << std::endl;
		}
		oss << "\tpush\t{fp, lr}" << std::endl;
		oss << "\tadd\tfp, sp, #0" << std::endl;

		int immediate_value = local_var_total_size;
		if (is_valid_immediate_value(immediate_value)) {
			oss << "\tsub\tsp, sp, #" << immediate_value << std::endl;
		} else {
			int temp_register = allocate_register(oss);
			oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
			oss << "\tsub\tsp, sp, r" << temp_register << std::endl;
			deallocate_register(temp_register, oss);
		}
		std::vector<std::shared_ptr<VarDefNode> >const &parameters = _node->getParameters();
		int parameters_size = parameters.size();
		for (int i = 0, end = parameters_size > 4 ? 4 : parameters_size; i < end; ++i) {
			std::shared_ptr<VarDefNode> const &var_def = parameters[i];
			int immediate_value = var_def->getVarOffset()[0];
			if (is_valid_immediate_value(immediate_value)) {
				oss << "\tstr\tr" << i << ", [fp, #" << immediate_value << "]" << std::endl;
			}
                        else {
				int temp_register = allocate_register(oss);
				oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
				oss << "\tstr\tr" << i << ", [fp, r" << temp_register << "]" << std::endl;
				deallocate_register(temp_register, oss);
			}
		}
		for (int i = 0, end = parameters_size - 4; i < end; ++i) {
			std::shared_ptr<VarDefNode> const &var_def = parameters[4 + i];
			int immediate_value = 8 + 4 * i;
			if (is_valid_immediate_value(immediate_value)) {
				oss << "\tldr\tr10, [fp, #" << immediate_value << "]" << std::endl;
			} else {
				int temp_register = allocate_register(oss);
				oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
				oss << "\tldr\tr10, [fp, r" << temp_register << "]" << std::endl;
				deallocate_register(temp_register, oss);
			}
			immediate_value = var_def->getVarOffset()[0];
			if (is_valid_immediate_value(immediate_value)) {
				oss << "\tstr\tr10, [fp, #" << immediate_value << "]" << std::endl;
			}
			else {
				int temp_register = allocate_register(oss);
				oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
				oss << "\tstr\tr10, [fp, r" << temp_register << "]" << std::endl;
				deallocate_register(temp_register, oss);
			}
		}
		std::shared_ptr<BlockNode> body = _node->getBody();
		if (!gen_arm_assembly(std::dynamic_pointer_cast<Node>(body), oss))
			return false;

		oss << ".L" << return_label_id << ":" << std::endl;
		oss << "\tsub\tsp, fp, #0" << std::endl;
		oss << "\tpop\t{fp, pc}" << std::endl;
		oss << "\t.ltorg" << std::endl;
		oss << std::endl;
	}
	break;
	case Var_Node: {
		std::shared_ptr<VarNode> _node = std::dynamic_pointer_cast<VarNode>(node);
		const std::shared_ptr<VarTypeNode> &target_var = _node->getTargetVar();
		if (std::dynamic_pointer_cast<VarDefNode>(target_var->getParent())->isIsGlobal()) {
			oss << "\tldr\tr10, ." << _node->get_name_() << "_addr" << std::endl;
			if (!target_var->getDimensions().size())
				oss << "\tldr\tr10, [r10]" << std::endl;
			else {
				if (!get_var_type_init_value(target_var)) {
					oss << "\tldr\tr10, [r10]" << std::endl;
				}
			}
		}
		else {
			int offset;
			if (!get_target_var_offset(target_var, offset))
				return false;
			int immediate_value = offset;
			if (is_valid_immediate_value(immediate_value)) {
				oss << "\tldr\tr10, [fp, #" << immediate_value << "]" << std::endl;
			} else {
				int temp_register = allocate_register(oss);
				oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
				oss << "\tldr\tr10, [fp, r" << temp_register << "]" << std::endl;
				deallocate_register(temp_register, oss);
			}
		}
	}
	break;
	case Func_Node: {
		std::shared_ptr<FunNode> _node = std::dynamic_pointer_cast<FunNode>(node);
		std::vector<std::shared_ptr<Node> > &parameters = _node->getParameters();
		int parameters_size = parameters.size();
		oss << "\tpush\t{r0-r9}" << std::endl;
		for (int i = 0, end = parameters_size > 4 ? 4 : parameters_size; i < end; ++i) {
			if (!gen_arm_assembly(parameters[i], oss))
				return false;
			oss << "\tmov\tr" << i << ", r10" << std::endl;
		}
		for (int i = parameters_size - 1; i >= 4; --i) {
			if (!gen_arm_assembly(parameters[i], oss))
				return false;
			oss << "\tpush\t{r10}" << std::endl;
		}
		oss << "\tbl\t" << _node->get_name_() << std::endl;
		for (int i = parameters_size - 1; i >= 4; --i) {
			oss << "\tpop\t{r10}" << std::endl;
		}
		oss << "\tmov\tr10, r0" << std::endl;
		oss << "\tpop\t{r0-r9}" << std::endl;
	}
	break;
	case Stmt_Node: {
		std::shared_ptr<StmtNode> _node = std::dynamic_pointer_cast<StmtNode>(node);
		if (!gen_arm_assembly(std::dynamic_pointer_cast<Node>(_node->getStmt()), oss))
			return false;
	}
	break;
	case Insert_Node: {
		std::shared_ptr<ParenthesisNode> _node = std::dynamic_pointer_cast<ParenthesisNode>(node);
		if (!gen_arm_assembly(std::dynamic_pointer_cast<Node>(_node->getNode()), oss))
			return false;
	}
	break;
	case Dig_Node: {
		std::shared_ptr<LiteralNode> const &_node = std::dynamic_pointer_cast<LiteralNode>(node);
		int immediate_value = _node->getValue();
		if (is_valid_immediate_value(immediate_value)) {
			oss << "\tmov\tr10, #" << immediate_value << std::endl;
		} else {
			oss << "\tldr\tr10, =" << immediate_value << std::endl;
		}
	}
	break;
	case Array_Access_Node: {
		std::shared_ptr<ArrayAccessNode> _node = std::dynamic_pointer_cast<ArrayAccessNode>(node);

		std::shared_ptr<Node> const &target = _node->getTarget();
		if (Var_Node != target->get_type_()) {
			last_arm_generate_error_msg = "ArrayAccessNode's target must be variable!!";
			return false;
		}
		std::vector<std::shared_ptr<Node>> const &indexes = _node->getIndexes();
		std::shared_ptr<VarNode> var_node = std::dynamic_pointer_cast<VarNode>(target);
		const std::shared_ptr<VarTypeNode> &target_var = var_node->getTargetVar();
		std::shared_ptr<Node> expression;
		bool partitial_access;
		if (!get_array_access_index_expression(target_var, indexes, expression, partitial_access))
			return false;
		expression->setParent(node);
		if (!gen_arm_assembly(expression, oss))
			return false;
		if (std::dynamic_pointer_cast<VarDefNode>(target_var->getParent())->isIsGlobal()) {
			int temp_register = allocate_register(oss);
			oss << "\tldr\tr" << temp_register << ", ." << var_node->get_name_() << "_addr" << std::endl;
			if (!get_var_type_init_value(target_var)) {
				oss << "\tldr\tr" << temp_register << ", [r" << temp_register << "]" << std::endl;
			}
			oss << "\tadd\tr10, r" << temp_register << ", r10, LSL #2" << std::endl;
			if (!partitial_access)
				oss << "\tldr\tr10, [r10]" << std::endl;
			deallocate_register(temp_register, oss);
		} else {
			int offset;
			if (!get_target_var_offset(target_var, offset))
				return false;
			int temp_register = allocate_register(oss);
			int immediate_value = offset;
			if (is_valid_immediate_value(immediate_value)) {
				oss << "\tldr\tr" << temp_register << ", [fp, #" << immediate_value << "]" << std::endl;
			} else {
				oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
				oss << "\tldr\tr" << temp_register << ", [fp, r" << temp_register << "]" << std::endl;
			}
			oss << "\tadd\tr10, r" << temp_register << ", r10, LSL #2" << std::endl;
			if (!partitial_access)
				oss << "\tldr	r10, [r10]" << std::endl;
			deallocate_register(temp_register, oss);
		}
	}
	break;
	case Op_Node: {
		std::shared_ptr<OperationNode> _node = std::dynamic_pointer_cast<OperationNode>(node);
		std::vector<std::shared_ptr<Node> > &parameters = _node->getParameters();

		int temp_register = allocate_register(oss);
		Op_Type operator_type = _node->getOperatorType();
		switch (operator_type) {
		case Unary_Pos_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
		break;
		case Unary_Neg_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\trsb\tr10, r10, #0" << std::endl;
		break;
		case Not_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tcmp\tr10, #0" << std::endl;
			oss << "\tmoveq\tr10, #1" << std::endl;
			oss << "\tmovne\tr10, #0" << std::endl;
		break;
		case Mul_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tmul\tr10, r" << temp_register << ", r10" << std::endl;
		break;
		case Div_Op:
			{
				oss << "\tpush\t{r0-r3}" << std::endl;
				if (!gen_arm_assembly(parameters[0], oss))
					return false;
				oss << "\tmov\tr0, r10" << std::endl;
				if (!gen_arm_assembly(parameters[1], oss))
					return false;
				oss << "\tmov\tr1, r10" << std::endl;
				oss << "\tbl\t__aeabi_idiv" << std::endl;
				oss << "\tmov\tr10, r0" << std::endl;
				oss << "\tpop\t{r0-r3}" << std::endl;
			}
		break;
		case Mod_Op:
			{
				oss << "\tpush\t{r0-r3}" << std::endl;
				if (!gen_arm_assembly(parameters[0], oss))
					return false;
				oss << "\tmov\tr0, r10" << std::endl;
				if (!gen_arm_assembly(parameters[1], oss))
					return false;
				oss << "\tmov\tr1, r10" << std::endl;
				oss << "\tbl\t__aeabi_idivmod" << std::endl;
				oss << "\tmov\tr10, r1" << std::endl;
				oss << "\tpop\t{r0-r3}" << std::endl;
			}
		break;
		case Plus_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tadd\tr10, r" << temp_register << ", r10" << std::endl;
		break;
		case Minus_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tsub\tr10, r" << temp_register << ", r10" << std::endl;
		break;
		case Greater_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tcmp\tr" << temp_register << ", r10" << std::endl;
			oss << "\tmovgt\tr10, #1" << std::endl;
			oss << "\tmovle\tr10, #0" << std::endl;
		break;
		case Less_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tcmp\tr" << temp_register << ", r10" << std::endl;
			oss << "\tmovlt\tr10, #1" << std::endl;
			oss << "\tmovge\tr10, #0" << std::endl;
		break;
		case GEqual_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tcmp\tr" << temp_register << ", r10" << std::endl;
			oss << "\tmovge\tr10, #1" << std::endl;
			oss << "\tmovlt\tr10, #0" << std::endl;
		break;
		case LEqual_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tcmp\tr" << temp_register << ", r10" << std::endl;
			oss << "\tmovle\tr10, #1" << std::endl;
			oss << "\tmovgt\tr10, #0" << std::endl;
		break;
		case Equal_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tcmp\tr" << temp_register << ", r10" << std::endl;
			oss << "\tmoveq\tr10, #1" << std::endl;
			oss << "\tmovne\tr10, #0" << std::endl;
		break;
		case NEqual_Op:
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tmov\tr" << temp_register << ", r10" << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tcmp\tr" << temp_register << ", r10" << std::endl;
			oss << "\tmovne\tr10, #1" << std::endl;
			oss << "\tmoveq\tr10, #0" << std::endl;
		break;
		case And_Op: {
			int end_zero_label_id = next_global_id();
			int end_label_id = next_global_id();
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tcmp\tr10, #0" << std::endl;
			oss << "\tbeq\t.L" << end_zero_label_id << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tcmp\tr10, #0" << std::endl;
			oss << "\tbeq\t.L" << end_zero_label_id << std::endl;

			oss << "\tmov\tr10, #1" << std::endl;
			oss << "\tb\t.L" << end_label_id << std::endl;
			oss << ".L" << end_zero_label_id << ":" << std::endl;
			oss << "\tmov\tr10, #0" << std::endl;
			oss << ".L" << end_label_id << ":" << std::endl;
		}
		break;
		case Or_Op: {
			int end_one_label_id = next_global_id();
			int end_label_id = next_global_id();
			if (!gen_arm_assembly(parameters[0], oss))
				return false;
			oss << "\tcmp\tr10, #0" << std::endl;
			oss << "\tbne\t.L" << end_one_label_id << std::endl;
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			oss << "\tcmp\tr10, #0" << std::endl;
			oss << "\tbne\t.L" << end_one_label_id << std::endl;
			oss << "\tmov\tr10, #0" << std::endl;
			oss << "\tb\t.L" << end_label_id << std::endl;
			oss << ".L" << end_one_label_id << ":" << std::endl;
			oss << "\tmov\tr10, #1" << std::endl;
			oss << ".L" << end_label_id << ":" << std::endl;
		}
		break;
		case Assign_Op: {
			std::shared_ptr<Node> const &node = parameters[0];
			Node_Type type = node->get_type_();
			if (Var_Node != type && Array_Access_Node != type) {
				last_arm_generate_error_msg = "Assignment operator first parameter must be lvalue!!";
				return false;
			}
			if (!gen_arm_assembly(parameters[1], oss))
				return false;
			if (Var_Node == type) {
				std::shared_ptr<VarNode> _node = std::dynamic_pointer_cast<VarNode>(node);
				const std::shared_ptr<VarTypeNode> &target_var = _node->getTargetVar();
				if (std::dynamic_pointer_cast<VarDefNode>(target_var->getParent())->isIsGlobal()) {
					oss << "\tldr\tr" << temp_register << ", ." << _node->get_name_() << "_addr" << std::endl;
					oss << "\tstr\tr10, [r" << temp_register << "]" << std::endl;
				} else {
					int offset;
					if (!get_target_var_offset(target_var, offset))
						return false;
					int immediate_value = offset;
					if (is_valid_immediate_value(immediate_value)) {
						oss << "\tstr\tr10, [fp, #" << immediate_value << "]" << std::endl;
					} else {
						oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
						oss << "\tstr\tr10, [fp, r" << temp_register << "]" << std::endl;
					}
				}
			} else {
				std::shared_ptr<ArrayAccessNode> _node = std::dynamic_pointer_cast<ArrayAccessNode>(node);
				std::shared_ptr<Node> const &target = _node->getTarget();
				if (Var_Node != target->get_type_()) {
					last_arm_generate_error_msg = "ArrayAccessNode's target must be variable!!";
					return false;
				}
				std::vector<std::shared_ptr<Node>> const &indexes = _node->getIndexes();
				std::shared_ptr<VarNode> var_node = std::dynamic_pointer_cast<VarNode>(target);
				const std::shared_ptr<VarTypeNode> &target_var = var_node->getTargetVar();

				std::shared_ptr<Node> expression;
				bool partitial_access;
				if (!get_array_access_index_expression(target_var, indexes, expression, partitial_access))
					return false;
				if (partitial_access) {
					last_arm_generate_error_msg = "Assignment operation lvalue cannot be an array!!";
					return false;
				}
				expression->setParent(node);
				oss << "\tpush\t{r10}" << std::endl;
				if (!gen_arm_assembly(expression, oss))
					return false;
				if (std::dynamic_pointer_cast<VarDefNode>(target_var->getParent())->isIsGlobal()) {
					oss << "\tldr\tr" << temp_register << ", ." << var_node->get_name_() << "_addr" << std::endl;
					if (!get_var_type_init_value(target_var)) {
						oss << "\tldr\tr" << temp_register << ", [r" << temp_register << "]" << std::endl;
					}
					oss << "\tadd\tr" << temp_register << ", r" << temp_register << ", r10, LSL #2" << std::endl;
					oss << "\tpop\t{r10}" << std::endl;
					oss << "\tstr\tr10, [r" << temp_register << "]" << std::endl;
				}
				else {
					int offset;
					if (!get_target_var_offset(target_var, offset))
						return false;
					int immediate_value = offset;
					if (is_valid_immediate_value(immediate_value)) {
						oss << "\tldr\tr" << temp_register << ", [fp, #" << immediate_value << "]" << std::endl;
					} else {
						oss << "\tldr\tr" << temp_register << ", =" << immediate_value << std::endl;
						oss << "\tldr\tr" << temp_register << ", [fp, r" << temp_register << "]" << std::endl;
					}
					oss << "\tadd\tr" << temp_register << ", r" << temp_register << ", r10, LSL #2" << std::endl;
					oss << "\tpop\t{r10}" << std::endl;
					oss << "\tstr\tr10, [r" << temp_register << "]" << std::endl;
				}
			}
		}
		break;
		default:
			//never reach
			last_arm_generate_error_msg = "Unknown operator!!";
			return false;
		}
		deallocate_register(temp_register, oss);
	}
	break;
	case If_Else_Node: {
		std::shared_ptr<IfElseNode> _node = std::dynamic_pointer_cast<IfElseNode>(node);
		int else_label_id = next_global_id();
		int end_label_id = next_global_id();
		if (!gen_arm_assembly(_node->getCondition(), oss))
			return false;
		oss << "\tcmp\tr10, #0" << std::endl;
		oss << "\tbeq\t.L" << else_label_id << std::endl;
		if (!gen_arm_assembly(_node->getIfBody(), oss))
			return false;
		oss << "\tb\t.L" << end_label_id << std::endl;
		oss << ".L" << else_label_id << ":" << std::endl;
		const std::shared_ptr<BlockNode> &else_body = _node->getElseBody();
		if (else_body)
			if (!gen_arm_assembly(else_body, oss))
				return false;
		oss << ".L" << end_label_id << ":" << std::endl;
	}
	break;
	case While_Node: {
		std::shared_ptr<WhileNode> _node = std::dynamic_pointer_cast<WhileNode>(node);
		int continue_label_id= next_global_id();
		int break_label_id= next_global_id();
		_node->setContinueLabelId(continue_label_id);
		_node->setBreakLabelId(break_label_id);

		oss << ".L" << continue_label_id << ":" << std::endl;
		oss << "@######WHILE LOOP begin\t.L" << continue_label_id << " - .L" << break_label_id << std::endl;
		if (!gen_arm_assembly(_node->getCondition(), oss))
			return false;
		oss << "\tcmp\tr10, #0" << std::endl;
		oss << "\tbeq\t.L" << break_label_id << std::endl;
		if (!gen_arm_assembly(_node->getBody(), oss))
			return false;
		oss << "\tb\t.L" << continue_label_id << std::endl;
		oss << ".L" << break_label_id << ":" << std::endl;
		oss << "@######WHILE LOOP endt\t.L" << continue_label_id << " - .L" << break_label_id << std::endl;
	}
	break;
	case Continue_Node: {
		std::shared_ptr<ContinueNode> _node = std::dynamic_pointer_cast<ContinueNode>(node);
		std::shared_ptr<WhileNode> const &while_node = _node->getTarget();
		oss << "\tb\t.L" << while_node->getContinueLabelId() << std::endl;
	}
	break;
	case Break_Node: {
		std::shared_ptr<BreakNode> _node = std::dynamic_pointer_cast<BreakNode>(node);
		std::shared_ptr<WhileNode> const &while_node = _node->getTarget();
		oss << "	b	.L" << while_node->getBreakLabelId() << std::endl;
	}
	break;
	case Return_Node: {
		std::shared_ptr<ReturnNode> _node = std::dynamic_pointer_cast<ReturnNode>(node);
		std::shared_ptr<FunDefNode> const &fun_def_node = _node->getTarget();
		std::shared_ptr<Node> const &return_value = _node->getReturnValue();
		if (return_value) {
			if (!gen_arm_assembly(return_value, oss))
				return false;
			oss << "\tmov\tr0, r10" << std::endl;
		}
		oss << "\tb\t.L" << fun_def_node->getReturnLabelId() << std::endl;
	}
	break;
	case Block_Node: {
		std::shared_ptr<BlockNode> _node = std::dynamic_pointer_cast<BlockNode>(node);
		if (!array_gen_arm_assembly(_node->getChildren(), oss))
			return false;
	}
	break;
	case Type_Node:
	case Var_Type_Node:
	case Def_Node:
	case Refer_Node:
	case Array_Node:
	case Unknown_Node:
	default:
		last_arm_generate_error_msg = "Not supported node type!!";
		return false;
	}
	return true;
}

bool gen_arm_assembly_code(std::shared_ptr<Node> const &node, std::ostringstream &oss) {

	//oss << "-------------------------" << std::endl;
	//oss << "|                       |" << std::endl;
	//oss << "| SysY Compiler working |" << std::endl;
	//oss << "|                       |" << std::endl;
	//oss << "-------------------------" << std::endl;
	//oss << std::endl;
	oss << "\t.text" << std::endl;
	oss << "\t.global main" << std::endl << std::endl;
	return gen_arm_assembly(node, oss);

}

