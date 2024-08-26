//
//	lexer	�ʷ�����
//
#pragma once

//Դ�����ļ���Ϣ
struct SRCINFO
{
	std::string filename;
	char* src = NULL;	//Դ��������
};

enum TOKEN_TYPE
{
	noncode,	//�Ǵ���
	code,		//����
	opcode,		//������������+-()�Ȳ�������
	string,		//�ַ���
	number,		//����
};

struct TOKEN
{
	std::string filename;	//�����ļ�
	int row_index;			//������
	int col_index;			//������
	TOKEN_TYPE type;		//����
	std::string Value;		//����
};

SRCINFO loadsrc(const char* filename);
void lexer(std::vector<TOKEN>& tokens, SRCINFO& srcinfo);
bool lexer_prepare(std::vector<TOKEN>& tokens);
void token_echo(TOKEN token, std::string pre);
void token_echo(std::vector<TOKEN> tokens, std::string pre);

//	THE END