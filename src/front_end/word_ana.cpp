#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <string.h>
#include "word_ana.h"
using namespace std;

char const* last_tokenize_error_msg = nullptr;

/*Token functions*/
Token::Token() :
	Token(nullptr, -1, nullptr) {}

Token::Token(char const* start, int length, char const* const name) :
	Token(start, length, name, Unknown_Token) {}

Token::Token(char const* start, int length, char const* const name, Token_Type type) :
	start(start), length(length), name(name), type(type) {}

Token::~Token() {}

int Token::get_len_() const { return this->length; }

void Token::set_len_(int length) { this->length = length; }

const std::string& Token::get_name_() const { return this->name; }

const char* Token::get_first_char_() const { return this->start; }

void Token::set_first_char_(const char* start) { this->start = start; }

Token_Type Token::get_token_type_() const { return this->type; }

void Token::set_token_type_(Token_Type type) { this->type = type; }

void Token::set_name_(const std::string& name) {this->name = name;}


/*
//��ͷ���������洢token��ÿ����ջһ��
typedef struct Token_Chain_on_heap {
	Token v;
	Token_Chain_on_heap* next;
}token_llist, tl, * ptl, * ptr_token_list;


typedef struct Position_Chain {
	int line, scol;
	Position_Chain* next;
}Positions, Pos, * ppos, * ptr_positions;*/


/*
typedef struct f32 {
	int base;
	int thumb;
}f32;*/

/*
union Value {
	long long int_;
	f32 float_;
	char char_;
};*/


//ֻ���Ǽ���������
enum Data_Types {
	bool2, int16, int32, int64, float32, float64, char4,
	pbool2, pint16, pint32, pint64, pfloat32, pfloat64, pchar4,
	cbool2, cint16, cint32, cint64, cfloat32, cfloat64, cchar4,
	NoneType,
	//enums,
	others,
	unknown
};


//ע������
enum Comment_Types {
	no_, single, multi_start, multi_end
};

/*
enum Token_Types {
	id, digit, cha, str
};*/





/*class Token {
private:
	Value value;
	int point;
public:
	char* name;
	int Reg_addr, length_;
	ppos pos;
	Data_Types datatype;
	Token_Types toknetype;
	bool is_exist(vector<Token> tokens);
	void set_value(int v);
	void set_value(f32 v);
	void set_value(char v);
	Value get_value();
	bool is_exist(vector<shared_ptr<Token>> tokens);
};*/


//��datatypeΪint��token����value
/*void Token::set_value(int v) {
	this->value.int_ = v;
}


//��datatypeΪint��token����value
void Token::set_value(f32 v) {
	this->value.float_ = v;
}


//��datatypeΪint��token����value
void Token::set_value(char v) {
	this->value.char_ = v;
}


//�ж�token�Ƿ��Ѿ�����
bool Token::is_exist(vector<shared_ptr<Token>> tokens) {
	vector<shared_ptr<Token>>::iterator token_ = tokens.begin();
	while (token_ != tokens.end()) {
		if (strcmp(this->name, (*token_)->name)) {
			return true;
		}
		token_++;
	}
	return false;
}


Value Token::get_value() {
	return this->value;
}*/


//�ж��Ƿ�Ϊע�ͣ��ж�ע������
Comment_Types get_comment_type(char const* name) {
	if (*name == '/' && *(name+1) == '/') {
		return single;
	}
	if (*name == '/' && *(name + 1) == '*') {
		return multi_start;
	}
	if (*name == '*' && *(name + 1) == '/') {
		return multi_end;
	}
	return no_;
}


//�ж��ַ��Ƿ�Ϊ8������
inline bool is_odig(char d) {
	return (d <= '7' && d >= '0');
}


//�ж��ַ��Ƿ�Ϊ10������
inline bool is_ddig(char d) {
	return (d <= '9' && d >= '0');
}


//�ж��ַ��Ƿ�Ϊ16������
inline bool is_xdig(char d) {
	return (is_ddig(d) || (d <= 'f' && d >= 'a') || (d <= 'F' && d >= 'A'));
}


//�ж��Ƿ�Ϊid���ַ�
bool is_id_letter(char name) {
	if (is_ddig(name) || (name <= 'z' && name >= 'a') || (name <= 'Z' && name >= 'A') || name == '_') {
		return true;
	}
	return false;
}


//�ж��Ƿ�Ϊ����
bool is_dig(char* name) {
	int i = 0;
	if (*name == '-') {
		i++;
	}
	if (*(name + i) == '.') {
		i++;
	}
	return is_ddig(*(name + i));
}


//�ж��Ƿ�Ϊ����
bool is_div(char* name) {
	if (*name == ',' || *name == ';') {
		return true;
	}
	return false;
}


//�ж��ַ��Ƿ����ִ���
bool is_in_str(char c, string s) {
	int i;
	for (i = 0; i < s.length(); i++) {
		if (c == s[i]) {
			return true;
		}
	}
	return false;
}


//�ж��Ƿ����ַ�
int len_char(char* name) {
	if (*name == '\''){
		if (*(name + 1) != '\\' && *(name + 2) == '\'') {
			return 3;
		}		
		if (*(name + 1) == '\\') {
			if (is_in_str(*(name + 2), "abfnrtv\\\'\"\?0") && *(name + 3) == '\'') {
				return 4;
			}
			char* cur = name + 2;
			int len = 2;
			if (*cur == 'x' || *cur == 'X') {
				cur++;
				len++;
				while (is_xdig(*cur++)) {
					len++;
					if (len > 5) {
						return 0;
					}
				}
			}
			while (is_odig(*(cur++))) {
				len++;
				if (len > 5) {
					return 0;
				}
			}
			if (*cur == '\'') {
				return len + 1;
			}
		}
	}
	return 0;
}


//�ж��Ƿ�Ϊ�ִ�
bool is_str(char* name) {
	if (*name != '"') {
		return false;
	}
	int i = 1;
	while (*(name + i) != '"') {
		if (*(name + i++) == '\n' ) {
			return false;
		}
	}
	return true;
}

  /*************/
 /*   �� ��   */
/*************/
//�����ִ�ת��Ϊint, pri_base = 10
int str2int(char* st, int len, int base_) {
	int ans = 0, syg = 1, i = 0;
	if (*st == '-') {
		syg = -1;
		i = 1;
	}
	for (; i < len; i++) {
		ans *= base_;
		if (base_ > 10 && !is_ddig(*(st + i))) {
			if (*(st + i) <= ('a' + base_ - 11) && *(st + i) >= 'a') {
				ans += *(st + i) - 'a' + 10;
			}
			else if (*(st + i) <= ('A' + base_ - 11) && *(st + i) >= 'A') {
				ans += *(st + i) - 'A' + 10;
			}
		}
		else {
			ans += (*(st + i) - '0');
		}
	}
	return ans * syg;
}


//���ת���ַ�
char escape_char(char* ch, int len, int line) {
	if (*ch == '\\') {
		if (len == 2) {
			switch (*(ch + 1)) {
			case '0': return '\0';
			case 'a': return '\a';
			case 'b': return '\b';
			case 'f': return '\f';
			case 'n': return '\n';
			case 'r': return '\r';
			case 't': return '\t';
			case 'v': return '\v';
			case '\\': return '\\';
			case '\'': return '\'';
			case '\"': return '\"';
			case '\?': return '\?';
			default:
				if (is_odig(*(ch + 1))) {
					return '\000' + *(ch + 1) - '0';
				}
			}
		}
		if (len > 2) {
			return '\000' + str2int(ch, len, 8);
		}
	}
}


//word_analysis functions
/*Ӧ�ü��һ��*/
//�ж��Ƿ�Ϊid��ʼ�ַ�
inline bool is_id_start(char name) {
	return ((name >= 'a' && name <= 'z') || (name >= 'A' && name <= 'Z') || name == '_');
}

//�����µ�id token
inline shared_ptr<Token> new_id_token(char const*& name) {
	char const* start = name;
	char const* end = name;
	while (is_id_letter(*++end));
	return shared_ptr<Token>(new Token(start, end - start, std::string(start, end).c_str()));
}

//�ж��Ƿ�Ϊ�µ�2λ����
inline int is_2c_symbol(char const* current) {
	for (int i = 0; i < Len_Multi_Char_Symbol_List; ++i) {
		if (*current == Multi_Char_Symbol_List[i][0] && *(current+1) == Multi_Char_Symbol_List[i][1])
			return i;
	}
	return -1;
}

//�����µ�2λ���� token
inline std::shared_ptr<Token> new_2c_symbol(char const*& name, int num_) {
	char const* start = name;
	if (num_ != -1) {
		int len = strlen(Multi_Char_Symbol_List[num_]);
		return std::shared_ptr<Token>(new Token(start, len, std::string(start, start+len).c_str()));
	}
	return nullptr;
}

//�ж��Ƿ�Ϊ�µ�1λ����
inline int is_1c_symbol(char const* name) {
	for (int i = 0; i < Lne_Single_Char_Symbol_List; i++) {
		if (*name == Single_Char_Symbol_List[i][0])
		return i;
	}
	return -1;
}

//�����µ�1λ���� token
inline std::shared_ptr<Token> new_1c_symbol(char const*& name, int num_) {
	if (num_ != -1) {
		return std::shared_ptr<Token>(new Token(name, 1, std::string(name, name + 1).c_str()));
	}
	return nullptr;
}

//�����µ����� token
inline shared_ptr<Token> new_dig_token(char const* name) {
	char const* start = name;
	char const* end = name;
	bool is_float = false;
	int len = 0;
	Token_Type type = Unknown_Token;
	if (*end == '-') {
		end++;
	}
	if (*end == '0') {
		end++;
		if (*end == 'x' || *end == 'X') {
			while (is_xdig(*(++end))) {
				len++;
			}
			if (*end == '.') {
				is_float = true;
				len++;
				while (is_xdig(*(++end))) {
					len++;
				}
			}
			type = Hex_Dig_Token;
		}
		else {
			while (is_odig(*end++)) {
				len++;
			}
			if (*end == '.') {
				is_float = true;
				len++;
				while (is_odig(*++end)) {
					len++;
				}
			}
			type = Oct_Dig_Token;
		}
	}
	else {
		while (is_ddig(*end++)) {
			len++;
		}
		if (*end == '.') {
			is_float = true;
			len++;
			while (is_ddig(*++end)) {
				len++;
			}
		}
		type = Dec_Dig_Token;
	}
	end--;
	return shared_ptr<Token>(new Token(start, end - start, std::string(start, end).c_str(), type));
}

//�ж��ַ��Ƿ����ַ����ڣ�Ŀǰδʹ�ã�
inline bool is_in_seq(char const* const* const seq, int len, char const* const str) {
	int i;
	for (i = 0; i < len; i++) {
		if (!strcmp(str, seq[i])) {
			return true;
		}
	}
	return false;
}

//ȷ��token����
inline bool set_token_type(std::vector<std::shared_ptr<Token>>& tokens) {
	std::vector<std::shared_ptr<Token>>::iterator start = tokens.begin(), end = tokens.end();
	for (; start != end; start++) {
		char const* const name = (*start)->get_name_().c_str();

		if (is_in_seq(Op_List, Len_Op_List, name)) {
			(*start)->set_token_type_(Op_Token);
			continue;
		}
		if (is_in_seq(Other_Symbol_List, Len_Other_Symbol_List, name)) {
			(*start)->set_token_type_(Symbol_Token);
			continue;
		}
		if (isdigit(*name)) {
			//�ڼ������ֵ�ʱ�����жϹ������
			continue;
		}

		if (!strcmp("const", name)) {
			(*start)->set_token_type_(Const_Token);
		}
		else if (!strcmp("int", name)) {
			(*start)->set_token_type_(Int_Token);
		}
		else if (!strcmp("void", name)) {
			(*start)->set_token_type_(Void_Token);
		}
		else if (!strcmp("if", name)) {
			(*start)->set_token_type_(If_Token);
		}
		else if (!strcmp("else", name)) {
			(*start)->set_token_type_(Else_Token);
		}
		else if (!strcmp("while", name)) {
			(*start)->set_token_type_(While_Token);
		}
		else if (!strcmp("continue", name)) {
			(*start)->set_token_type_(Continue_Token);
		}
		else if (!strcmp("break", name)) {
			(*start)->set_token_type_(Break_Token);
		}
		else if (!strcmp("return", name)) {
			(*start)->set_token_type_(Return_Token);
		}
		else {
			(*start)->set_token_type_(Id_Token);
		}
	}
	return true;
}

//token�б������Զ������������������ܣ�
bool lexical_analysis(std::vector< std::shared_ptr< Token > > &tokens, char const* const strs) {
	int line = 1;
	char const* current = strs;
	while (*current!= '\0') {
		//��������
		if (*current == '#') {
			while (*++current != '\n');
			line++;
			current++;
		}
		//�ҵ��µ�һ��
		if (*(current) == '\n') {
			line++;
			current++;
			continue;
		}
		//�������ַ�
		if (isspace(*current)) {
			current++;
			continue;
		}
		//������������
		Comment_Types ct = get_comment_type(current);
		if (ct == single) {
			current += 2;
			while ('\n' != *(current++));
			continue;
		}
		//������������
		if (ct == multi_start) {
			current += 2;
			while (get_comment_type(++current) != multi_end);
			current += 2;
			continue;
		}
		//id
		if (is_id_start(*current)) {
			shared_ptr<Token> tok = new_id_token(current);
			if (tok == nullptr) {
				std::cout << "id Ptr Error!" << std::endl;
				last_tokenize_error_msg = "id Ptr Error!";
			}
			tokens.push_back(tok);
			if (tok == nullptr) {
				std::cout << "Pushback Error!" << std::endl;
				last_tokenize_error_msg = "Pushback Error!";
			}
			current += tok->get_len_();
			continue;
		}
		//dig
		if (isdigit(*current)) {
			shared_ptr<Token> tok = new_dig_token(current);
			if (tok == nullptr) {
				std::cout << "dig Ptr Error!" << std::endl;
				last_tokenize_error_msg = "dig Ptr Error!";
			}
			tokens.push_back(tok);
			current += tok->get_len_();
			continue;
		}
		//2csymbol
		int num_ = is_2c_symbol(current);
		if (num_ != -1) {
			tokens.push_back(new_2c_symbol(current, num_));
			current += 2;
			continue;
		}
		//1csymbol
		num_ = is_1c_symbol(current);
		if (num_ != -1) {
			tokens.push_back(new_1c_symbol(current, num_));
			current++;
			continue;
		}
		std::cout << *current << "Unknown token in line " << line << "! Please check your code." << std::endl;
		last_tokenize_error_msg = "Unexpected token!";
		return false;
	}
	//std::cout << "all tokens done" << std::endl;
	return set_token_type(tokens);
}

//���token�б�����
bool check_token_type(std::vector< std::shared_ptr< Token > > const& tokens, std::string& str) {
	std::ostringstream out_str;
	std::vector<std::shared_ptr<Token>>::const_iterator start = tokens.begin(), end = tokens.end();
	for (; start != end; start++) {
		char const* type;
		switch ((*start)->get_token_type_()) {
		case Unknown_Token:
			type = "Unknown";
			break;
		case Op_Token:
			type = "Operator";
			break;
		case Symbol_Token:
			type = "Symbol";
			break;
		case Dec_Dig_Token:
			type = "Ddig";
			break;
		case Hex_Dig_Token:
			type = "Xdig";
			break;
		case Oct_Dig_Token:
			type = "Odig";
			break;
		case Const_Token:
			type = "Const";
			break;
		case Int_Token:
			type = "Int";
			break;
		case Void_Token:
			type = "Void";
			break;
		case If_Token:
			type = "If";
			break;
		case Else_Token:
			type = "ELse";
			break;
		case While_Token:
			type = "While";
			break;
		case Continue_Token:
			type = "Continue";
			break;
		case Break_Token:
			type = "Break";
			break;
		case Return_Token:
			type = "Return";
			break;
		case Id_Token:
			type = "Id";
			break;
		default:
			type = "Error_Token";
		}
		out_str << "Token name:" << (*start)->get_name_() << "\nToken type:" << type << "\n\n";
	}
	str = out_str.str();
	return true;
}

//���token�Ƿ��Ѿ�����
bool is_exist_token(std::shared_ptr< Token > const& token1, std::shared_ptr< Token > const& token2) {
	return token1->get_token_type_() == Id_Token && token2->get_token_type_() == Id_Token && token1->get_name_() == token2->get_name_();
}
