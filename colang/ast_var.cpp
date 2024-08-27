////////////////////////////////////////////////////////////////////////////////
//
// AST_var
//
////////////////////////////////////////////////////////////////////////////////
#include "colang.h"


AST_var::AST_var(std::vector<TOKEN>& tokens)
{
	//����
	type.push_back(tokens[0]);
	tokens.erase(tokens.begin());
	if (tokens[0].type != TOKEN_TYPE::string && (tokens[0].Value == "*" || tokens[0].Value == "&"))
	{
		type.push_back(tokens[0]);
		tokens.erase(tokens.begin());
	}
	//����
	name = tokens[0];
	//������������Ե���ǰ��������
	if (tokens[1].Value == ";")
		tokens.erase(tokens.begin());
}

void AST_var::show(std::string pre)
{
	std::cout << pre << "#TYPE:var" << std::endl;
	std::cout << pre << " type:";
	token_echo(type, pre + "      ");
	std::cout << pre << " name:";
	token_echo(name, pre);
	std::cout << std::endl;
}

llvm::Value* AST_var::codegen()
{
	//��������������ռλ
	VAR_INFO var_info;
	var_info.type = ir_type(type);
	var_info.value = ir_builder->CreateAlloca(var_info.type);
	ir_varlist[ir_varlist.size() - 1].info[name.Value] = var_info;
	return var_info.value;
}

