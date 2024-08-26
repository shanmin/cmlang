////////////////////////////////////////////////////////////////////////////////
//
// AST_call
//
////////////////////////////////////////////////////////////////////////////////

#include "colang.h"

AST_call::AST_call(std::vector<TOKEN>& tokens)
{
	//����
	name = tokens[0];
	tokens.erase(tokens.begin());

	//����
	if (tokens[0].Value == "(")
		tokens.erase(tokens.begin());
	else
		ErrorExit("�������ý�������", tokens);
	//��������
	while (!tokens.empty())
	{
		//��ȡ���������������˳�ѭ��
		if (tokens[0].type != TOKEN_TYPE::string && tokens[0].Value == ")")
			break;
		//��ȡ���ʽ
		args.push_back(ast_parse_expr(tokens));
		if (tokens[0].Value == ",")
			tokens.erase(tokens.begin());
		else if (tokens[0].type != TOKEN_TYPE::string && tokens[0].Value == ")")
		{
			tokens.erase(tokens.begin());
			return;
		}
		else if (tokens[0].type != TOKEN_TYPE::string && tokens[0].Value == ";")
			return;
		else
			ErrorExit("�������ò�����������(1)", tokens);
	}
	if (tokens[0].Value == ")")
		tokens.erase(tokens.begin());
	else
		ErrorExit("�������ò�����������", tokens);
}


void AST_call::show(std::string pre)
{
	std::cout << pre << "\033[1m#TYPE:call\033[0m" << std::endl;
	std::cout << pre << " name:";
	token_echo(name, "           ");
	std::cout << pre << " args:";
	bool first = true;
	for (auto a : args)
		if (first)
		{
			a->show("");
			first = false;
		}
		else
			a->show(pre + "      ");
	std::cout << std::endl;
}


llvm::Value* AST_call::codegen()
{
	//��������
	//Ex:	printf("abc");
	std::vector<llvm::Value*> fargs;
	for (auto a : args)
		fargs.push_back(a->codegen());
	llvm::Function* function = ir_module->getFunction(name.Value);
	if (function)
	{
		return ir_builder->CreateCall(function, fargs);
	}
	else
		ErrorExit("δ�ҵ���������", name);
	return NULL;
}

//	THE END