#ifndef SRC_BACK_END_OPTIMIZER_H_
#define SRC_BACK_END_OPTIMIZER_H_

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "../common/node.h"
#include "../tools/common.h"
class Token;

extern char const* last_optimize_error_msg;

bool evaluate_dig_node(std::shared_ptr<Node> const& node, std::shared_ptr<LiteralNode>& digit);

bool is_volatile_function(char const* const function_name);

bool merge_block_function_call(std::shared_ptr<BlockNode>& block);
bool replace_node_function_call(std::shared_ptr<Node>& node, std::vector<std::string>& function_call_name, std::map<std::string, std::shared_ptr<Node>>& function_call);
template<class NODE_TYPE>
bool array_replace_node_function_call(std::vector<std::shared_ptr<NODE_TYPE>>& nodes, std::vector<std::string>& function_call_name, std::map<std::string, std::shared_ptr<Node>>& function_call);
#endif /* _SRC_BACK_END_OPTIMIZER_H_END_ */
