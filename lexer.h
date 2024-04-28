//
//	lexer.h
//
#pragma once

#include <string>
#include <vector>

struct SRCINFO
{
	std::string filename;
	char* src;	//Դ��������
};

enum TOKEN_TYPE
{
	noncode,	//�Ǵ���
	code,		//����
	opcode,		//������
	string,		//�ַ���
	number,		//����
};

struct TOKEN
{
	std::string filename;
	int row_index;	//������
	int col_index;	//������
	TOKEN_TYPE type;
	std::string Value;
};

void ErrorExit(const char* str, std::vector<TOKEN>& tokens);
void ErrorExit(const char* str, TOKEN token);

void lexer(std::vector<TOKEN>& tokens, SRCINFO& srcinfo);
bool lexer_prepare(std::vector<TOKEN>& tokens);
void token_echo(TOKEN token, std::string pre);
void token_echo(std::vector<TOKEN> tokens, std::string pre);

//THE END