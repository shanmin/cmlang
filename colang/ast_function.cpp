////////////////////////////////////////////////////////////////////////////////
//
//	AST_function
//
////////////////////////////////////////////////////////////////////////////////

#include "colang.h"

AST_function::AST_function(std::vector<TOKEN>& tokens)
{
	//����ֵ
	rett.push_back(tokens[0]);
	tokens.erase(tokens.begin());
	if (tokens[0].type != TOKEN_TYPE::string && (tokens[0].Value == "*" || tokens[0].Value == "&"))
	{
		rett.push_back(tokens[0]);
		tokens.erase(tokens.begin());
	}
	//������
	name = tokens[0];
	tokens.erase(tokens.begin());
	//����
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
			args.push_back(tokens[0]);
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
		body = ast(tokens);
		//ErrorExit("�����岿�ֻ�δʵ��", tokens);
	}
	else
		ErrorExit("���������������", tokens);
}


void AST_function::show(std::string pre)
{
	std::cout << pre << "\033[1m#TYPE:function\033[0m" << std::endl;
	std::cout << pre << " rett:";
	token_echo(rett, pre + "      ");
	std::cout << pre << " name:";
	token_echo(name, pre);
	std::cout << pre << " args:";
	token_echo(args, pre + "      ");
	if (!body.empty())
	{
		std::cout << pre << " body:" << std::endl;
		for (auto a : body)
			//token_echo(body, pre + "      ");
			a->show(pre + "    ");
	}
	std::cout << std::endl;
}


llvm::Value* AST_function::codegen()
{
	//args
	llvm::Type* frtype = ir_type(rett);
	std::string fname = name.Value;
	std::vector<llvm::Type*> fatype;
	std::vector<std::string> faname;
	std::vector<bool> faun;
	bool isVarArg = false;
	while (!args.empty())
		if (args[0].Value == "...")
		{
			isVarArg = true;
			break;
		}
		else if (args[0].Value == ",")
		{
			args.erase(args.begin());
		}
		else
		{
			if (args[0].Value.substr(0, 1) == "u")
				faun.push_back(true);
			else
				faun.push_back(false);
			fatype.push_back(ir_type(args));
			if (!args.empty())
			{
				faname.push_back(args[0].Value);
				args.erase(args.begin());
			}
			else
				faname.push_back("");
		}
	llvm::FunctionType* functionType = llvm::FunctionType::get(frtype, fatype, isVarArg);
	llvm::Function* function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, fname, ir_module);
	////arg name
	//unsigned i = 0;
	//for (auto& a : function->args())
	//{
	//	if(faname[i]!="")
	//		a.setName(faname[i]);
	//}
	if (!body.empty())
	{
		//���õ�ǰ����������
		scope::push("function");
		llvm::Function::arg_iterator args = function->arg_begin();
		for (int i = 0; i < fatype.size(); i++)
		{
			VARINFO vi;
			vi.name = faname[i];
			vi.type = fatype[i];
			vi.value = args;// ir_builder->CreateAlloca(function->getArg(i)->getType(), function->getArg(i));
			scope::set(vi);
			args++;
		}
		//��ǰ��ǩ��
		LABEL_LIST lablelist;
		ir_labellist.push_back(lablelist);

		llvm::BasicBlock* old = ir_builder->GetInsertBlock();
		//���������ǩ
		llvm::BasicBlock* entry = llvm::BasicBlock::Create(ir_context, "", function);
		ir_builder->SetInsertPoint(entry);

		////���� RETURN ����ֵ����
		//VAR_INFO return_val;
		//if (!frtype->isVoidTy())
		//{
		//	return_val.type = frtype;
		//	return_val.value = ir_builder->CreateAlloca(function->getReturnType(), NULL, NULL, "__co__RETURN_VAL");
		//	varlist.info["__co__RETURN_VAL"] = return_val;
		//}
		//ir_varlist.push_back(varlist);

		////���� RETURN ��ǩ
		//llvm::BasicBlock* bb_return = llvm::BasicBlock::Create(ir_context, "__co__RETURN", function);
		//ir_builder->SetInsertPoint(bb_return);
		////��ǰ���뷵��ָ��
		//if (frtype->isVoidTy())
		//	ir_builder->CreateRetVoid();
		//else
		//{
		//	llvm::Value* v=ir_var_load(return_val);
		//	ir_builder->CreateRet(v);
		//}

		ir_builder->SetInsertPoint(entry);

		for (auto& a : body)
			a->codegen();

		//�����������ǩ������
		scope::pop();
		ir_labellist.pop_back();
		ir_builder->SetInsertPoint(old);
	}
	return function;
}


//	THE END