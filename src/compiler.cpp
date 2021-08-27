#include <stdbool.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <unistd.h>

#include "tools/common.h"
#include "front_end/word_ana.h"
#include "common/node.h"
#include "front_end/parser.h"
#include "front_end/semantic_analyzer.h"
#include "back_end/arm_generator.h"
#include "back_end/optimizer.h"
using namespace std;

int main(int argc, char *argv[]) {

	char const* sysy_file_path;
	char const* output_file_path;
	
	if (argc < 2) {
		std::cerr << "Argument count error!" << std::endl;
		return 1;
	}

	char option;
	while ((option = getopt(argc, argv, ":So:")) != -1) { 
		switch (option) {
		case 'S':
			break;
		case 'o':
			output_file_path = optarg;
			break;
		case ':':
			std::cerr << "Option needs a value." << std::endl;
			return 2;
			break;
		default:
			std::cerr << "Unknown option: " << (char)optopt << std::endl;
			return 3;
		}
	}
	for (; optind < argc; optind++) {
		sysy_file_path = argv[optind];
	}
	
	std::string sy_file;
	if (!read_file2string(sysy_file_path, sy_file))
		return 4;
	ReplaceStringInPlace(sy_file, "putf(", "printf(");
	ReplaceStringInPlace(sy_file, "_SYSY_N", "1024");

	expand_timing_function(sy_file, "starttime", "_sysy_starttime");
	expand_timing_function(sy_file, "stoptime", "_sysy_stoptime");


	std::vector<std::shared_ptr<Token>> tokens;
	if (!lexical_analysis(tokens, sy_file.c_str())) {
		std::cerr << last_tokenize_error_msg << std::endl;
		return 5;
	}
	std::string str;
	if (!check_token_type(tokens, str))
		return 6;
	//std::cout << std::endl << "--------------------------------------Tokens--------------------------------------" << std::endl << str << "--------------------------------------Tokens End--------------------------------------" << std::endl;
	std::shared_ptr<BlockNode> root = parse_(tokens);
	if (!root) {
		std::cerr << last_parse_error_msg << std::endl;
		return 7;
	}

	if (!semantic_analyze_(root)) {
		std::cerr << last_semantic_analyze_error_msg << std::endl;
		return 9;
	}
	std::ostringstream oss;
	if (!node2json_dump_(std::dynamic_pointer_cast<Node>(root), oss))
		return 8;
	//std::cout << std::endl << "--------------------------------------Nodes--------------------------------------" << std::endl << oss.str() << "--------------------------------------Nodes End--------------------------------------" << std::endl;

	oss.str("");
	if (!gen_arm_assembly_code(std::dynamic_pointer_cast<Node>(root), oss)) {
		std::cerr << last_arm_generate_error_msg << std::endl;
		return 9;
	}
	//std::cout << std::endl << oss.str() << std::endl;

	if (!write_string2file(output_file_path, oss.str()))
		return 10;
	std::cout << "output file is " << output_file_path << std::endl << std::endl << "Version: 0.1.1" << std::endl << "Update date: 2021-6-26" << std::endl << "Auther: MYX" << std::endl;

	return 0;
}
