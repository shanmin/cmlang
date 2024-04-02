//
//	lexer.cpp
//

#include <vector>
#include "lexer.h"

void ErrorExit(const char* str, std::vector<TOKEN>& tokens)
{
	//printf(str);
	if (tokens.size() == 0)
		printf("\n---------- Error ----------\n%s\n", str);
	else
		printf("\n---------- Error ----------\n%s\n\t  row: %d\n\t  col: %d\n\t type: %d\n\ttoken: %s\n",
			str, tokens[0].row_index+1, tokens[0].col_index, tokens[0].type, tokens[0].Value.c_str());
	exit(1);
}
void ErrorExit(const char* str, TOKEN token)
{
	printf("\n---------- Error ----------\n%s\n\t  row: %d\n\t  col: %d\n\t type: %d\n\ttoken: %s\n",
			str, token.row_index+1, token.col_index, token.type, token.Value.c_str());
	exit(1);
}

void token_echo(TOKEN token, std::string pre)
{
	std::vector<std::string> TOKEN_TYPE_STRING =
	{
		"noncode",
		"   code",
		" opcode",
		" string",
		" number",
	};

	if (token.filename == "")
	{
		std::cout << std::endl;
		return;
	}
	printf(pre.c_str());
	printf("[r:%3d,c:%3d](%d:%s) : %s\n", token.row_index + 1, token.col_index, token.type, TOKEN_TYPE_STRING[token.type].c_str(), token.Value.c_str());
}

void token_echo(std::vector<TOKEN> tokens, std::string pre)
{
	bool first = true;
	for (TOKEN token : tokens)
		if (first)
		{
			token_echo(token, "");
			first = false;
		}
		else
			token_echo(token, pre);
}

//�ж��ַ��Ƿ�Ϊ���ַ�������
bool is_opcode1(char c)
{
	return  c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ';';
}

//�ж��ַ��Ƿ�Ϊ�������ַ�
bool is_opcode2(char c)
{
	return  c == '<' || c == '>' || c == '=' || c == ',' || c==':' ||
		c == '+' || c == '-' || c == '*' || c == '/' || c == '|' || c == '&' || c == '!';
}

//�ʷ�������
//	��Դ������Ϊ��С��token��Ԫ
void lexer(std::vector<TOKEN>& tokens, SRCINFO& srcinfo)
{
	
	char* src = srcinfo.src;
	std::string current;
	//��ȡԴ�������ݽ��н���
	bool iscode = false; //��ʶ�Ƿ���������
	int begin_row_index = 0;//��ǰ����ʼ���к�
	int begin_col_index = 0;//��ǰ����ʼ���к�
	int row_index = 0; //��ǰ����Ĵ����к�
	int col_index = 0; //��ǰ����Ĵ����к�

	while (src[0] != 0)
	{
		if (iscode)
		{
			//�����հ��ַ�
			while (isspace(src[0]))
			{
				if (src[0] == '\n')
				{
					row_index++;
					col_index = 0;
				}
				else
					col_index++;
				src++;
			}

			//�жϴ����������
			if (src[0] == '?' && src[1] == '>')
			{
				iscode = false;
				src += 2;
				col_index += 2;
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
				continue;
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
				token.filename = srcinfo.filename;
				token.type = TOKEN_TYPE::string;
				token.Value = current;
				token.row_index = begin_row_index;
				token.col_index = begin_col_index;
				tokens.push_back(token);

				current = "";
				continue;
			}

			//���������
			if (current.empty())
			{
				while (is_opcode1(src[0]))
				{
					current += src[0];
					src++;
					col_index++;
					break;
				}
				if (current.empty())
					while (is_opcode2(src[0]))
					{
						current += src[0];
						src++;
						col_index++;
					}
				//�����
				if (!current.empty())
				{
					TOKEN token;
					token.filename = srcinfo.filename;
					token.row_index = row_index;
					token.col_index = col_index;
					token.type = TOKEN_TYPE::opcode;
					token.Value = current;
					tokens.push_back(token);

					current = "";
					continue;
				}
			}

			//����
			while (src[0])
			{
				if (is_opcode1(src[0]) || is_opcode2(src[0]) ||
					src[0] == ' ' ||
					src[0] == '|' || src[0] == '&' || src[0] == '%' || src[0] == '?' ||
					src[0] == ',' || src[0] == ':' || src[0] == 0 ||
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
			}
			if (!current.empty())
			{
				TOKEN token;
				token.filename = srcinfo.filename;
				token.row_index = begin_row_index;
				token.col_index = begin_col_index;
				if (isdigit(current[0]))
					token.type = TOKEN_TYPE::number;
				else
					token.type = TOKEN_TYPE::code;
				token.Value = current;
				tokens.push_back(token);

				current = "";
			}
		}
		else
		{
			//��ȡ�Ǵ���������
			while (src[0] != 0)
			{
				//�ж��Ƿ���������
				if (src[0] == '<' && src[1] == '?')
				{
					src += 2;
					col_index += 2;
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
				token.filename = srcinfo.filename;
				token.row_index = begin_row_index;
				token.col_index = begin_col_index;
				token.type = TOKEN_TYPE::noncode;
				token.Value = current;
				tokens.push_back(token);
			}
			current = "";
			iscode = true;	//���������
		}
	}//while (src[0] != 0)
}

//	THE END