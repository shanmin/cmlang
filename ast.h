//
//	AST�﷨����
//
#pragma once

#include <vector>
#include "lexer.h"

enum AST_TYPE
{
	echo, //ֱ�����
	function,	//��������
	function_ret,	//��������ֵ����
	function_name,	//��������
	function_args,	//���������б�
	function_body,	//������
	call,		//��������		
	call_args,	//�������ò���
	codeblock,	//��������飬����{}�������Ĵ���
};
struct AST_STRUCT
{
	AST_TYPE type;
	std::vector<AST_STRUCT> child;
	std::vector<TOKEN> tokens;
};
//�﷨��������Ҫ���ִ����Ρ������;
void ast(std::vector<TOKEN>& tokens, std::vector<AST_STRUCT>& ast_list)
{
	while (!tokens.empty())
	{
		if (tokens[0].type == TOKEN_TYPE::noncode)
		{
			AST_STRUCT a;
			a.type = AST_TYPE::echo;
			a.tokens.push_back(tokens[0]);
			ast_list.push_back(a);
			tokens.erase(tokens.begin());
			continue;
		}
		//Function
		//	Ex: int function_name(int arg1)
		//		 |		|              + args
		//       |      + ��������
		//       +  ����ֵ����
		//�ж����ݣ���ǰΪ�ַ�����һ��token��һ���ַ�������һ����(
		if (tokens[0].type == TOKEN_TYPE::block && tokens[1].type == TOKEN_TYPE::block && tokens[2].type == TOKEN_TYPE::paren_open)
		{
			AST_STRUCT func;
			func.type = AST_TYPE::function;
			//����ֵ
			AST_STRUCT ret;
			ret.type = AST_TYPE::function_ret;
			ret.tokens.push_back(tokens[0]);
			func.child.push_back(ret);
			tokens.erase(tokens.begin());
			//����
			AST_STRUCT name;
			name.type = AST_TYPE::function_name;
			name.tokens.push_back(tokens[0]);
			func.child.push_back(name);
			tokens.erase(tokens.begin());
			//������ʼ
			if (tokens[0].type == TOKEN_TYPE::paren_open)
				tokens.erase(tokens.begin());
			else
				ErrorExit("���������������", tokens[0]);
			//�����б�
			while (tokens[0].type != TOKEN_TYPE::paren_close) //ֻҪ����)�ڵ㣬��һֱ��ȡ
			{
				AST_STRUCT arg;
				arg.type = AST_TYPE::function_args;
				while (tokens[0].type != TOKEN_TYPE::paren_close && tokens[0].type != TOKEN_TYPE::comma)
				{
					arg.tokens.push_back(tokens[0]);
					tokens.erase(tokens.begin());
				}
				func.child.push_back(arg);
				//�Ƴ��Ѵ���Ķ���
				if (tokens[0].type == TOKEN_TYPE::comma)
					tokens.erase(tokens.begin());
			}
			//�Ƴ�)
			if (tokens[0].Value == ")")
				tokens.erase(tokens.begin());
			else
				ErrorExit("��������������ִ���", tokens[0]);
			//�жϺ����Ƿ���ں�����
			if (tokens[0].Value == ";")
				tokens.erase(tokens.begin());
			else if (tokens[0].Value == "{")
			{
				AST_STRUCT body;
				body.type = AST_TYPE::function_body;
				ast(tokens, body.child);
				func.child.push_back(body);
			}

			ast_list.push_back(func);
			continue;
		}
		//��������
		//Ex:	printf("abc");
		if (tokens[0].type == TOKEN_TYPE::block && tokens[1].type == TOKEN_TYPE::paren_open)
		{
			AST_STRUCT call;
			call.type = AST_TYPE::call;
			//����
			AST_STRUCT name;
			name.type = AST_TYPE::function_name;
			name.tokens.push_back(tokens[0]);
			call.child.push_back(name);
			tokens.erase(tokens.begin());
			//������ʼ
			if (tokens[0].type == TOKEN_TYPE::paren_open)
				tokens.erase(tokens.begin());
			else
				ErrorExit("�������ý�������", tokens[0]);
			//�����б�
			while (tokens[0].type != TOKEN_TYPE::paren_close) //ֻҪ����)�ڵ㣬��һֱ��ȡ
			{
				AST_STRUCT arg;
				arg.type = AST_TYPE::function_args;
				while (tokens[0].type != TOKEN_TYPE::paren_close && tokens[0].type != TOKEN_TYPE::comma)
				{
					arg.tokens.push_back(tokens[0]);
					tokens.erase(tokens.begin());
				}
				call.child.push_back(arg);
				//�Ƴ��Ѵ���Ķ���
				if (tokens[0].type == TOKEN_TYPE::comma)
					tokens.erase(tokens.begin());
			}
			//�Ƴ�)
			if (tokens[0].Value == ")")
				tokens.erase(tokens.begin());
			else
				ErrorExit("��������������ִ���", tokens[0]);
			//�жϺ����Ƿ���ں�����
			if (tokens[0].Value == ";")
				tokens.erase(tokens.begin());

			ast_list.push_back(call);
			continue;
		}
		ErrorExit("δʶ���ʶ", tokens[0]);
		break;
	}
}

void token_echo(std::vector<TOKEN> tokens, std::string pre)
{
	for (TOKEN token : tokens)
	{
		printf(pre.c_str());
		printf("[r:%3d,c:%3d]%2d : %s\n", token.row_index, token.col_index, token.type, token.Value.c_str());
	}
}
void ast_echo(std::vector<AST_STRUCT>& ast_list, std::string pre)
{
	for (AST_STRUCT a : ast_list)
	{
		printf(pre.c_str());
		printf(" type: %d\n", a.type);
		if (!a.child.empty())
		{
			printf(pre.c_str());
			printf("child:\n");
			ast_echo(a.child, pre + "    ");
		}
		if (!a.tokens.empty())
		{
			token_echo(a.tokens, pre + "    ");
		}
	}
}

//	THE END