//
//	lexer	�ʷ�����
//
#include "colang.h"

//����Դ�����ļ�
SRCINFO loadsrc(const char* filename)
{
	SRCINFO srcinfo;
	//��ȡԴ�ļ�����
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		printf("ERROR: input file open fail.");
		return srcinfo;
	}
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	srcinfo.filename = filename;
	srcinfo.src = (char*)malloc(size + 1);
	if (srcinfo.src == NULL)
	{
		printf("ERROR: out of memory");
		return srcinfo;
	}
	fseek(fp, 0, SEEK_SET);
	fread(srcinfo.src, 1, size, fp);
	srcinfo.src[size] = 0;
	fclose(fp);

	return srcinfo;
}


//�ж��ַ��Ƿ�Ϊ���ַ�������
bool is_opcode1(char c)
{
	return  c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ',' || c == ';';
}


//�ж��ַ��Ƿ�Ϊ���ַ�������
bool is_opcode2(char c)
{
	return  c == '<' || c == '>' || c == '=' || c == ':' ||
		c == '+' || c == '-' || c == '*' || c == '/' || c == '|' || c == '&' || c == '!';
}


//�ʷ�������
//	��Դ������Ϊ��С��token��Ԫ
void lexer(std::vector<TOKEN>& tokens, SRCINFO& srcinfo)
{
	//����ļ�δ�������ݣ���ֱ�ӷ��� 20240814 shanmin
	if (srcinfo.src == NULL)
		return;

	char* src = srcinfo.src;

	bool iscode = false; //��ʶ�Ƿ���������
	std::string current;	//��ǰ���ڴ����ı�ʶ
	int begin_row_index = 0;//��ǰ������ʼ���к�
	int begin_col_index = 0;//��ǰ������ʼ���к�
	int current_row_index = 0; //��ǰ�����Ĵ����к�
	int current_col_index = 0; //��ǰ�����Ĵ����к�

	//��ȡԴ�������ݽ��н���
	while (src[0] != 0)
	{
		if (iscode)	//����������
		{
			//�����հ��ַ�������жϰ������ո�\t��\n��\v��\f��\r�ַ�
			while (isspace(src[0]))
			{
				if (src[0] == '\n')
				{
					current_row_index++;
					current_col_index = 0;
				}
				else
					current_col_index++;
				src++;
			}

			//�жϴ����������
			if (src[0] == '?' && src[1] == '>')
			{
				iscode = false;
				src += 2;
				current_col_index += 2;
				continue;
			}

			//��������ע��
			if (src[0] == '/' && src[1] == '/')
			{
				src += 2;
				current_col_index += 2;
				while (src[0])
				{
					if (src[0] == '?' && src[1] == '>') //���ע�ͺ����Ǵ��������������Ϊ�������
					{
						break;
					}
					else if (src[0] == '\n') //�˳���ǰע�ͣ��������ǰ��Ŀհ��ַ�����������д���
					{
						break;
					}
					src++;
					current_col_index++;
				}
				continue;
			}

			//����ע�Ϳ�
			if (src[0] == '/' && src[1] == '*')
			{
				src += 2;
				current_col_index += 2;
				while (src[0])
				{
					if (src[0] == '*' && src[1] == '/') //ע�Ϳ����
					{
						src += 2;
						current_col_index += 2;
						break;
					}
					else if (src[0] == '\n')
					{
						src++;
						current_row_index++;
						current_col_index = 0;
					}
					else
					{
						src++;
						current_col_index++;
					}
				}
				continue;
			}

			//�ж��ַ�����ȡ
			if (src[0] == '"')
			{
				begin_row_index = current_row_index;
				begin_col_index = current_col_index;
				src++;
				current_col_index++;
				while (src[0] != 0)
					if (src[0] == '"') //�ַ�������
					{
						src++;
						current_col_index++;
						break;
					}
					else if (src[0] == '\\')
					{
						if (src[1] == '"')		current += "\"";
						else if (src[1] == '\\')current += "\\";
						else if (src[1] == 'b')	current += "\b";	//backspace
						else if (src[1] == 'f')	current += "\f";	//formfeed
						else if (src[1] == 'n')	current += "\n";	//linefeed
						else if (src[1] == 'r')	current += "\r";	//carriage return
						else if (src[1] == 't')	current += "\t";	//horizontal tab
						else if (src[1] == 'v')	current += "\v";	//vertical tab
						else
						{
							current += src[0];
							current += src[1];
						}
						src += 2;
						current_col_index += 2;
						continue;
					}
					else
					{
						current += src[0];
						src++;
						if (src[0] == '\n')
						{
							current_row_index++;
							current_col_index = 0;
						}
						else
							current_col_index++;
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
					current_col_index++;
					break;
				}
				if (current.empty())
					while (is_opcode2(src[0]))
					{
						current += src[0];
						src++;
						current_col_index++;
					}
				//�����
				if (!current.empty())
				{
					TOKEN token;
					token.filename = srcinfo.filename;
					token.row_index = current_row_index;
					token.col_index = current_col_index;
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
					src[0] == ' ' || src[0] == '%' || src[0] == '?' ||
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
						begin_row_index = current_row_index;
						begin_col_index = current_col_index;
					}
					current += src[0];
					src++;
					current_col_index++;
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
		else //�Ǵ���������
		{
			//��ȡ�Ǵ���������
			while (src[0] != 0)
			{
				//�ж��Ƿ���������
				if (src[0] == '<' && src[1] == '?')
				{
					src += 2;
					current_col_index += 2;
					break;
				}
				else
				{
					if (current.empty()) //�����ǰ��ʶΪ�գ����¼��ʶ��ʼλ��
					{
						begin_row_index = current_row_index;
						begin_col_index = current_col_index;
					}
					current += src[0];	//����һ����Ч�ַ�
					if (src[0] == '\n')
					{
						current_row_index++;
						current_col_index = 0;
					}
					else
						current_col_index++;
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



//�ʷ�����Ԥ����
//	��Ҫ����#inclde����
bool lexer_prepare(std::vector<TOKEN>& tokens)
{
	for (int i = 0; i < tokens.size() - 1; i++)
	{
		if (tokens[i].type == TOKEN_TYPE::code && tokens[i].Value == "#include" && tokens[i + 1].type == TOKEN_TYPE::string)
		{
			//��ȡinclude�ļ�
			std::vector<TOKEN> include_tokens;
			SRCINFO srcinfo = loadsrc(tokens[i + 1].Value.c_str());
			if (srcinfo.filename.empty())
			{
				ErrorExit("#include�ļ����ش���", tokens[i + 1]);
			}
			lexer(include_tokens, srcinfo);
			//����TOKENs
			std::vector<TOKEN> new_tokens;
			for (int j = 0; j < i; j++)
				new_tokens.push_back(tokens[j]);
			for (int j = 0; j < include_tokens.size(); j++)
				new_tokens.push_back(include_tokens[j]);
			for (int j = i + 2; j < tokens.size(); j++)
				new_tokens.push_back(tokens[j]);
			tokens = new_tokens;
			return true;
		}
	}
	return false;
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
	std::string showstr;
	for (char c : token.Value)
	{
		//�������ַ�����ת��������ʾ
		switch (c)
		{
		case '\r':showstr += "\\r"; break;
		case '\n':showstr += "\\n"; break;
		case '\t':showstr += "\\t"; break;
		default:showstr += c; break;
		}
	}
	//printf("\033[1m[%s,r:%3d,c:%3d](%d:%s) :\033[0m %s\n", token.filename.c_str(), token.row_index + 1, token.col_index, token.type, TOKEN_TYPE_STRING[token.type].c_str(), showstr.c_str());
	printf("\033[2m[%s,r:%3d,c:%3d](%d:%s) \033[32m:\033[0m %s\n", token.filename.c_str(), token.row_index + 1, token.col_index, token.type, TOKEN_TYPE_STRING[token.type].c_str(), showstr.c_str());
}

//���ָ��TOKEN��Ϣ
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

//	THE END