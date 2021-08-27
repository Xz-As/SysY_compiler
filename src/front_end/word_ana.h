#ifndef SRC_FRONT_END_TOKENIZE_H_
#define SRC_FRONT_END_TOKENIZE_H_

#include <string>
#include <vector>
#include <memory>
#include "../common/type.h"

extern char const* last_tokenize_error_msg;

class Token {
private:
	char const* start;
	int length;
	std::string name;
	Token_Type type;

public:
	Token();
	Token(char const* start, int length, char const* const name);
	Token(char const* start, int length, char const* const name, Token_Type type);
	~Token();

	int get_len_() const;
	void set_len_(int length);
	const std::string& get_name_() const;
	void set_name_(const std::string& name);
	const char* get_first_char_() const;
	void set_first_char_(const char* start);
	Token_Type get_token_type_() const;
	void set_token_type_(Token_Type type);
};
extern bool is_exist_token(std::shared_ptr< Token > const& token1, std::shared_ptr< Token > const& token2);
extern bool lexical_analysis(std::vector< std::shared_ptr< Token > >& tokens, char const* const strs);
extern bool check_token_type(std::vector< std::shared_ptr< Token > > const& tokens, std::string& str);

#endif /* SRC_FRONT_END_TOKENIZE_H_ */
