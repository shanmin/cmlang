//
//	Lexer
//
#pragma once

#include <string>
#include <vector>
#include "colang.h"

enum TOKEN_TYPE
{
	noncode,	//�Ǵ���
	preproc,	//Ԥ������룬��#��ͷ��ָ��
	block,	//�ؼ���
	string,		//�ַ���
	number,		//����
	paren_open,		//������(
	paren_close,	//������)
	bracket_open,	//��������[
	bracket_close,	//��������]
	curly_open,		//�������{
	curly_close,	//�Ҵ�����}
	semicolon,	//�ֺ�;
	comma,		//����,
	colon,		//ð��:
	equal,		//�Ⱥ�=
	define,		//����Ԥ����
};

struct TOKEN
{
	TOKEN_TYPE type;
	std::string filename;
	std::string Value;
	int row_index;	//������
	int col_index;	//������
};


void token_echo(std::vector<TOKEN> tokens, std::string pre)
{
	for (TOKEN token : tokens)
	{
		printf(pre.c_str());
		printf("[r:%3d,c:%3d]%2d : %s\n", token.row_index+1, token.col_index, token.type, token.Value.c_str());
	}
}
void token_echo(TOKEN token, std::string pre)
{
		printf(pre.c_str());
		printf("[r:%3d,c:%3d]%2d : %s\n", token.row_index+1, token.col_index, token.type, token.Value.c_str());
}

//�ʷ�������
//	��Դ������Ϊ��С��token��Ԫ
void lexer(std::vector<TOKEN>& tokens, SRCINFO& srcinfo)
{
	char* src = srcinfo.src;
	std::string current;
	//��ȡԴ�������ݽ��н���
	bool iscode = false; //��ʶ�Ƿ���������
	int row_index = 0; //��ǰ����Ĵ����к�
	int col_index = 0; //��ǰ����Ĵ����к�
	int begin_row_index = 0;//��ǰ����ʼ���к�
	int begin_col_index = 0;//��ǰ����ʼ���к�

	while (src[0] != 0)
	{
		if (iscode)
		{
			//�����հ��ַ�
			while (isspace(src[0]))
			{
				src++;
				if (src[0] == '\n')
				{
					row_index++;
					col_index = 0;
				}
				else
					col_index++;
			}

			//�жϴ����������
			if (src[0] == '?' && src[1] == '>')
			{
				iscode = false;
				src += 2;
				col_index+=2;
				continue;
			}

			//��������ע��
			if (src[0] == '/' && src[1] == '/')
			{
				src += 2;
				col_index += 2;
				while (src[0])
				{
					if (src[0] == '?' && src[1] == '>')
					{
						break;
					}
					else if (src[0] == '\n')
					{
						break;
					}
					src++;
					col_index++;
				}
				continue;
			}

			//����ע�Ϳ�
			if (src[0] == '/' && src[1] == '*')
			{
				src += 2;
				col_index += 2;
				while (src[0])
				{
					if (src[0] == '*' && src[1] == '/')
					{
						src += 2;
						col_index += 2;
						break;
					}
					else if (src[0] == '\n')
					{
						src++;
						row_index++;
						col_index = 0;
					}
					else
					{
						src++;
						col_index++;
					}
				}
			}

			//�ж��ַ�����ȡ
			if (src[0] == '"')
			{
				begin_row_index = row_index;
				begin_col_index = col_index;
				src++;
				col_index++;
				while (src[0] != 0)
					if (src[0] == '\\')
					{
						if (src[1] == '"')		current += "\"";
						else if (src[1] == 'a')	current += "\a";
						else if (src[1] == 'b')	current += "\b";
						else if (src[1] == 'f')	current += "\f";
						else if (src[1] == 't')	current += "\t";
						else if (src[1] == 'v')	current += "\v";
						else if (src[1] == 'r')	current += "\r";
						else if (src[1] == 'n')	current += "\n";
						else
						{
							current += src[0];
							current += src[1];
						}
						src += 2;
						col_index += 2;
						continue;
					}
					else if (src[0] == '"') //�ַ�������
					{
						src++;
						col_index++;
						break;
					}
					else
					{
						current += src[0];
						src++;
						if (src[0] == '\n')
						{
							row_index++;
							col_index = 0;
						}
						else
							col_index++;
					}
				TOKEN token;
				token.filename = srcinfo.src;
				token.type = TOKEN_TYPE::string;
				token.Value = current;
				token.row_index = begin_row_index;
				token.col_index = begin_col_index;
				tokens.push_back(token);

				current = "";
				continue;
			}

			//�����
			if (current.empty())
			{
				//˫�����
				if (src[0] == '=' && src[1] == '=') current = "==";
				else if (src[0] == '>' && src[1] == '=') current = ">=";
				else if (src[0] == '=' && src[1] == '>') current = ">=";
				else if (src[0] == '<' && src[1] == '=') current = "<=";
				else if (src[0] == '=' && src[1] == '<') current = "<=";
				else if (src[0] == '!' && src[1] == '=') current = "!=";
				else if (src[0] == '<' && src[1] == '>') current = "!=";
				else if (src[0] == '>' && src[1] == '<') current = "!=";
				else if (src[0] == '+' && src[1] == '=') current = "+=";
				else if (src[0] == '-' && src[1] == '=') current = "-=";
				else if (src[0] == '*' && src[1] == '=') current = "*=";
				else if (src[0] == '/' && src[1] == '=') current = "/=";
				else if (src[0] == '&' && src[1] == '=') current = "&=";
				else if (src[0] == '|' && src[1] == '=') current = "|=";
				else if (src[0] == '<' && src[1] == '<')
				{
					if (src[2] == '=')
						current = "<<=";
					else
						current = "<<";
				}
				else if (src[0] == '>' && src[1] == '>')
				{
					if (src[2] == '=')
						current = ">>=";
					else
						current = ">>";
				}
				if (!current.empty())
				{
					TOKEN token;
					token.filename = srcinfo.src;
					token.type = TOKEN_TYPE::block;
					token.Value = current;
					token.row_index = row_index;
					token.col_index = col_index;
					tokens.push_back(token);

					current = "";
					src += 2;
					col_index += 2;
					continue;
				}

				if (src[0] == '=' || src[0] == '!' ||
					src[0] == '+' || src[0] == '-' || src[0] == '*' || src[0] == '/' ||
					src[0] == '|' || src[0] == '&' || src[0] == '%' || src[0] == '?' ||
					src[0] == '<' || src[0] == '>' ||
					src[0] == '(' || src[0] == ')' ||
					src[0] == '[' || src[0] == ']' ||
					src[0] == '{' || src[0] == '}' ||
					src[0] == ',' || src[0] == ':' || src[0] == ';'
					)
				{
					current = src[0];

					TOKEN token;
					token.filename = srcinfo.src;
					if (src[0] == '(')
						token.type = TOKEN_TYPE::paren_open;
					else if (src[0] == ')')
						token.type = TOKEN_TYPE::paren_close;
					else if (src[0] == '[')
						token.type = TOKEN_TYPE::bracket_open;
					else if (src[0] == ']')
						token.type = TOKEN_TYPE::bracket_close;
					else if (src[0] == '{')
						token.type = TOKEN_TYPE::curly_open;
					else if (src[0] == '}')
						token.type = TOKEN_TYPE::curly_close;
					else if (src[0] == ';')
						token.type = TOKEN_TYPE::semicolon;
					else if (src[0] == ',')
						token.type = TOKEN_TYPE::comma;
					else if (src[0] == ':')
						token.type = TOKEN_TYPE::colon;
					else if (src[0] == '=')
						token.type = TOKEN_TYPE::equal;
					else
						token.type = TOKEN_TYPE::block;
					token.Value = current;
					token.row_index = row_index;
					token.col_index = col_index;
					tokens.push_back(token);

					current = "";
					src++;
					col_index++;
					continue;
				}
			}

			//�ؼ���
			while (src[0])
				if (src[0] == ' ' || src[0] == '=' ||
					src[0] == '+' || src[0] == '-' || src[0] == '*' || src[0] == '/' ||
					src[0] == '|' || src[0] == '&' || src[0] == '%' || src[0] == '?' ||
					src[0] == '<' || src[0] == '>' ||
					src[0] == '(' || src[0] == ')' ||
					src[0] == '[' || src[0] == ']' ||
					src[0] == '{' || src[0] == '}' ||
					src[0] == ',' || src[0] == ':' || src[0] == ';' || src[0] == 0 ||
					src[0] == '\t' || src[0] == '\r' || src[0] == '\n' || src[0] == '\\'
					)
				{
					break;
				}
				else
				{
					if (current.empty())
					{
						begin_row_index = row_index;
						begin_col_index = col_index;
					}
					current += src[0];
					src++;
					col_index++;
				}
			if (!current.empty())
			{
				TOKEN token;
				token.filename = srcinfo.src;
				if (isdigit(current[0]))
					token.type = TOKEN_TYPE::number;
				else if (token.Value.starts_with("#"))
					token.type = TOKEN_TYPE::preproc;
				else
					token.type = TOKEN_TYPE::block;
				token.Value = current;
				token.row_index = begin_row_index;
				token.col_index = begin_col_index;
				tokens.push_back(token);

				current = "";
			}
		}
		else
		{
			while (src[0] != 0)
			{
				//�ж��Ƿ���������
				if (src[0] == '<' && src[1] == '?')
				{
					src += 2;
					col_index+=2;
					break;
				}
				else
				{
					if (current.empty())
					{
						begin_row_index = row_index;
						begin_col_index = col_index;
					}
					current += src[0];
					if (src[0] == '\n')
					{
						row_index++;
						col_index = 0;
					}
					else
						col_index++;
					src++;
				}
			}
			//����Ǵ�����Ϊ�գ�������д���
			if (!current.empty())
			{
				TOKEN token;
				token.filename = srcinfo.src;
				token.type = TOKEN_TYPE::noncode;
				token.Value = current;
				token.row_index = begin_row_index;
				token.col_index = begin_col_index;
				tokens.push_back(token);
			}
			current = "";
			iscode = true;	//���������
		}
	}
}

//	THE END