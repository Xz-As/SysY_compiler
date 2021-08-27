#ifndef SRC_COMMON_GLOBAL_H_
#define SRC_COMMON_GLOBAL_H_

#include <map>

#include "type.h"

enum SY_ASSOCIATIVITY {
	AS_L2R, AS_R2L
};
typedef struct OPERATOR_INFO {
	Op_Type operator_type;
	int precedence;
	int parameter_number;
	SY_ASSOCIATIVITY associativity;
} OPERATOR_INFO;

extern std::map<Op_Type, OPERATOR_INFO> OPERATOR_INFO_MAP;

long long next_global_id();

#endif /* SRC_COMMON_GLOBAL_H_ */
