#ifndef SRC_FRONT_END_SEMANTIC_ANALYZER_H_
#define SRC_FRONT_END_SEMANTIC_ANALYZER_H_
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <iostream>
#include "../common/node.h"

extern char const *last_semantic_analyze_error_msg;

bool semantic_analyze_(std::shared_ptr<BlockNode> &root);

#endif /* SRC_FRONT_END_SEMANTIC_ANALYZER_H_ */
