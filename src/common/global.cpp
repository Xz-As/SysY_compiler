#include "global.h"

OPERATOR_INFO OPERATOR_INFOS[] = { { Unary_Pos_Op, 16, 1, AS_R2L }, //		/* +  */
		{ Unary_Neg_Op, 16, 1, AS_R2L }, //		/* -  */
		{ Not_Op, 16, 1, AS_R2L }, //		/* !  */
		{ Mul_Op, 15, 2, AS_L2R }, //				/* *  */
		{ Div_Op, 15, 2, AS_L2R }, //				/* /  */
		{ Mod_Op, 15, 2, AS_L2R }, //				/* %  */
		{ Plus_Op, 14, 2, AS_L2R }, //				/* +  */
		{ Minus_Op, 14, 2, AS_L2R }, //				/* -  */
		{ Greater_Op, 13, 2, AS_L2R }, //			/* >  */
		{ Less_Op, 13, 2, AS_L2R }, //				/* <  */
		{ GEqual_Op, 13, 2, AS_L2R }, //		/* >= */
		{ LEqual_Op, 13, 2, AS_L2R }, //			/* <= */
		{ Equal_Op, 12, 2, AS_L2R }, //				/* == */
		{ NEqual_Op, 12, 2, AS_L2R }, //			/* != */
		{ And_Op, 11, 2, AS_L2R }, //		/* && */
		{ Or_Op, 10, 2, AS_L2R }, //		/* || */
		{ Assign_Op, 9, 2, AS_R2L } //			/* =  */
};
const unsigned int OPERATOR_INFOS_LEN = sizeof(OPERATOR_INFOS)
		/ sizeof(OPERATOR_INFO);

std::map<Op_Type, OPERATOR_INFO> INIT_OPERATOR_INFO_MAP() {
	std::map<Op_Type, OPERATOR_INFO> map;
	for (int i = 0; i < OPERATOR_INFOS_LEN; ++i) {
		OPERATOR_INFO const &info = OPERATOR_INFOS[i];
		map[info.operator_type] = info;
	}
	return map;
}
std::map<Op_Type, OPERATOR_INFO> OPERATOR_INFO_MAP = INIT_OPERATOR_INFO_MAP();

long long next_global_id() {
	static long long id = 0;
	return ++id;
}











