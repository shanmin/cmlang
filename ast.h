//
//	AST�﷨����
//
#pragma once

#include <map>
#include <iostream>


enum AST_TYPE
{
	ast_noncode,
	ast_codeblock,
	ast_function,
	ast_call,
	ast_var,
	ast_expr,
};
std::vector<std::string> AST_TYPE_STRING = {
	"noncode",
	"codeblock",
	"function",
	"call",
	"var",
	"expr",
};

struct AST
{
	AST_TYPE type;
	std::map<std::string, std::vector<TOKEN>> value;
	std::vector<AST> body;
};


//
std::vector<AST> ast_parse_expr(std::vector<TOKEN>& tokens)
{
	std::vector<AST> ast_list;
	while (!tokens.empty())
	{
		if (tokens[0].type == TOKEN_TYPE::string)
		{
			AST a;
			a.type = AST_TYPE::ast_expr;
			a.value["value"].push_back(tokens[0]);
			tokens.erase(tokens.begin());
			ast_list.push_back(a);
			continue;
		}
		else if (tokens[0].type == TOKEN_TYPE::noncode)//�Ǵ��룬ֱ�����
		{
			tokens[0].type = TOKEN_TYPE::string;
			TOKEN token_name;
			token_name.col_index = tokens[0].col_index;
			token_name.filename = tokens[0].filename;
			token_name.row_index = tokens[0].row_index;
			token_name.type = TOKEN_TYPE::code;
			token_name.Value = "printf";

			AST ast_args;
			ast_args.type = AST_TYPE::ast_expr;
			ast_args.value["value"].push_back(tokens[0]);

			AST a;
			a.type = AST_TYPE::ast_call;
			a.value["name"].push_back(token_name);
			a.body.push_back(ast_args);
			ast_list.push_back(a);
			tokens.erase(tokens.begin());
			break;
		}

		//����������,ֱ�ӷ���
		if (tokens[0].Value == "}")
			break;
		else if (tokens[0].Value == ")" || tokens[0].Value==";") // ���ʽ����
		{
			tokens.erase(tokens.begin());
			break;
		}
		if(tokens[0].Value==",")
		{
			tokens.erase(tokens.begin());
			continue;
		}

		//��������
		//Ex:	printf("abc");
		if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::opcode && tokens[1].Value == "(")
		{
			AST a;
			a.type = AST_TYPE::ast_call;
			a.value["name"].push_back(tokens[0]);
			tokens.erase(tokens.begin());

			if (tokens[0].Value == "(")
				tokens.erase(tokens.begin());
			else
				ErrorExit("�������ý�������", tokens);

			a.body = ast_parse_expr(tokens);//��������

			ast_list.push_back(a);
			if (tokens[0].Value == ";") //�ֺŽ�β���˳���ǰ������
			{
				tokens.erase(tokens.begin());
				break;
			}
			continue;
		}


		if (tokens[0].Value == "(")
		{
			tokens.erase(tokens.begin());
			AST a;
			a.type = AST_TYPE::ast_expr;
			a.body = ast_parse_expr(tokens);
			//tokens.erase(tokens.begin());
			ast_list.push_back(a);
			continue;
		}

		//���ʽ
		AST a;
		a.type =AST_TYPE::ast_expr;
		a.value["value"].push_back(tokens[0]);
		tokens.erase(tokens.begin());
		ast_list.push_back(a);
	}
	return ast_list;
}

//�﷨��������Ҫ���ִ����Ρ������;
std::vector<AST> ast(std::vector<TOKEN>& tokens)
{
	std::vector<AST> ast_list;
	while (!tokens.empty())
	{
		//����������,ֱ�ӷ���
		if (tokens[0].type != TOKEN_TYPE::string)
		{
			if (tokens[0].Value == "{") //��������
			{
				tokens.erase(tokens.begin());
				AST a;
				a.type = AST_TYPE::ast_codeblock;
				a.body = ast(tokens);
				ast_list.push_back(a);
				continue;
			}
			else if (tokens[0].Value == "}")
			{
				tokens.erase(tokens.begin());
				break;
			}
			if (tokens[0].Value == ")") // ���ʽ����
			{
				tokens.erase(tokens.begin());
				break;
			}
		}

		//Function
		//	Ex: int function_name(int arg1)
		//		 |		|              + args
		//       |      + ��������
		//       +  ����ֵ����
		//�ж����ݣ���ǰΪ�ַ�����һ��token��һ���ַ�������һ����(
		if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::code && tokens[2].type == TOKEN_TYPE::opcode && tokens[2].Value=="(")
		{
			AST a;
			a.type =AST_TYPE::ast_function;
			//����ֵ
			a.value["rett"].push_back(tokens[0]);
			tokens.erase(tokens.begin());
			if (tokens[0].type != TOKEN_TYPE::string && (tokens[0].Value == "*" || tokens[0].Value == "&"))
			{
				a.value["rett"].push_back(tokens[0]);
				tokens.erase(tokens.begin());
			}
			//������
			a.value["name"].push_back(tokens[0]);
			tokens.erase(tokens.begin());

			if (tokens[0].Value == "(")
				tokens.erase(tokens.begin());
			else
				ErrorExit("��������������ֽ�������", tokens);

			//��������
			while (!tokens.empty())
				if (tokens[0].Value == ")")
				{
					break;
				}
				else
				{
					a.value["args"].push_back(tokens[0]);
					tokens.erase(tokens.begin());
				}
			//�Ƴ�)
			if (tokens[0].Value == ")")
				tokens.erase(tokens.begin());
			else
				ErrorExit("��������������ִ���", tokens);
			//�жϺ����Ƿ���ں�����
			if (tokens[0].Value == ";")
				tokens.erase(tokens.begin());
			else if (tokens[0].Value == "{")
			{
				tokens.erase(tokens.begin());
				a.body = ast(tokens);
			}
			else
				ErrorExit("���������������", tokens);
			ast_list.push_back(a);
			continue;
		}
		
		//��������
		//Ex:	int a;
		//		int b=2;
		if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::code && tokens[1].Value != "(")
		{
			if (tokens[2].Value == ";") //��������
			{
				AST a;
				a.type = AST_TYPE::ast_var;
				a.value["type"].push_back(tokens[0]);
				tokens.erase(tokens.begin());
				a.value["name"].push_back(tokens[0]);
				tokens.erase(tokens.begin());
				tokens.erase(tokens.begin());
				ast_list.push_back(a);
				continue;
			}
			else if (tokens[2].Value == "=")
			{
				AST a;
				a.type = AST_TYPE::ast_var;
				a.value["type"].push_back(tokens[0]);
				tokens.erase(tokens.begin());
				a.value["name"].push_back(tokens[0]);
				ast_list.push_back(a);
				continue;
			}
		}

		//���ʽ����
		AST a;
		a.type = AST_TYPE::ast_expr;
		a.body = ast_parse_expr(tokens);
		ast_list.push_back(a);


		////������ֵ
		//if (tokens[1].type != TOKEN_TYPE::string && tokens[1].Value == "=")
		//{
		//	AST a;
		//	a.type =AST_TYPE::ast_expr;
		//	a.value["name"].push_back(tokens[0]);
		//	tokens.erase(tokens.begin());
		//	tokens.erase(tokens.begin());
		//	while (!tokens.empty() && tokens[0].type != TOKEN_TYPE::string && tokens[0].Value != ";")
		//	{
		//		a.value["value"].push_back(tokens[0]);
		//		tokens.erase(tokens.begin());
		//	}
		//	if(!tokens.empty())
		//		tokens.erase(tokens.begin());
		//	ast_list.push_back(a);
		//	continue;
		//}

		////���ʽ
		//{
		//	AST a;
		//	a.type = ast_expr;
		//	a.value["value"].push_back(tokens[0]);
		//	tokens.erase(tokens.begin());
		//	ast_list.push_back(a);
		//	continue;
		//}

		//ErrorExit("δʶ���ʶ", tokens);
		//break;
	}
	return ast_list;
}

void ast_echo(std::vector<AST> ast_list, std::string pre,bool onlyone=false)
{
	for (AST a : ast_list)
	{
		std::cout << pre << "#TYPE: " << a.type<<"[" << AST_TYPE_STRING[a.type]<<"]" << std::endl;
		for (auto it : a.value)
		{
			std::cout << pre << it.first<<":"<<std::endl;
			for (auto t : it.second)
			{
				printf(pre.c_str());
				printf("     [r:%3d,c:%3d]%2d(%s) : %s\n", t.row_index, t.col_index, t.type,TOKEN_TYPE_STRING[t.type].c_str(), t.Value.c_str());
			}
		}
		if (a.body.empty())
			std::cout << std::endl;
		else
		{
			std::cout << pre << "BODY:"<<std::endl;
			ast_echo(a.body, pre + "     ");
			std::cout << std::endl;
		}
		if (onlyone)
			break;
	}
}

//	THE END