//
//	ast.cpp
//

#include <llvm/IR/Value.h>
#include "colang.h"


llvm::LLVMContext ir_context;
llvm::Module* ir_module;
std::unique_ptr<llvm::IRBuilder<>> ir_builder;
std::vector<VAR_LIST> ir_varlist; //�ֲ�������Χ


void ir(std::vector<AST*>& ast_list, const char* filename)
{
	ir_module = new llvm::Module(filename, ir_context);
	ir_builder = std::make_unique<llvm::IRBuilder<>>((ir_context));

	//���õ�ǰ����������
	VAR_LIST varlist;
	varlist.zone = "global";
	ir_varlist.push_back(varlist);

	llvm::FunctionType* mainType = llvm::FunctionType::get(ir_builder->getInt32Ty(), false);
	llvm::Function* main = llvm::Function::Create(mainType, llvm::GlobalValue::ExternalLinkage, "main", ir_module);
	llvm::BasicBlock* entryMain = llvm::BasicBlock::Create(ir_context, "entry_main", main);
	ir_builder->SetInsertPoint(entryMain);

	//ir_proc(tokens, true);
	for (auto a : ast_list)
		a->codegen();

	llvm::ConstantInt* ret = ir_builder->getInt32(0);
	ir_builder->CreateRet(ret);
	llvm::verifyFunction(*main);

	//��֤ģ���Ƿ��������
	std::string mstr;
	llvm::raw_string_ostream merr(mstr);
	bool result = llvm::verifyModule(*ir_module, &merr);
	if (result)
	{
		printf("ģ����ڴ���%s", mstr.c_str());
		exit(2);
	}

	ir_module->print(llvm::outs(), nullptr);

	//���ll��ʽ�ļ�
	std::string col = std::string(filename) + "l";
	std::error_code ec;
	llvm::raw_fd_ostream fout(col, ec);
	ir_module->print(fout, nullptr);
	fout.close();
	printf("\n---------- write ll ----------\n");
	printf("save=%x\n", ec.value());

	std::string cob = std::string(filename) + "b";
	llvm::raw_fd_ostream fout_cob(cob, ec);
	llvm::WriteBitcodeToFile(*ir_module, fout_cob);
	fout_cob.close();
	printf("\n---------- write bc ----------\n");
	printf("save=%x\n", ec.value());
}


llvm::Type* ir_type(std::vector<TOKEN>& tokens)
{
	std::string name = tokens[0].Value;
	if (tokens.size() > 1 && tokens[1].Value == "*")
	{
		name += "*";
	}

	llvm::Type* type = NULL;
	if (name == "void") type = llvm::Type::getVoidTy(ir_context);
	if (name == "int1" || name == "bool") type = llvm::Type::getInt1Ty(ir_context);
	if (name == "int8" || name == "byte" || name == "char") type = llvm::Type::getInt8Ty(ir_context);
	if (name == "int8*" || name == "byte*" || name == "char*") type = llvm::Type::getInt8PtrTy(ir_context);
	if (name == "int16" || name == "short") type = llvm::Type::getInt16Ty(ir_context);
	if (name == "int16*" || name == "short*") type = llvm::Type::getInt16PtrTy(ir_context);
	if (name == "int32" || name == "int") type = llvm::Type::getInt32Ty(ir_context);
	if (name == "int32*" || name == "int*") type = llvm::Type::getInt32PtrTy(ir_context);
	if (name == "int64" || name == "long") type = llvm::Type::getInt64Ty(ir_context);
	if (name == "int64*" || name == "long*") type = llvm::Type::getInt64PtrTy(ir_context);
	if (name == "float") type = llvm::Type::getFloatTy(ir_context);
	if (name == "float*") type = llvm::Type::getFloatPtrTy(ir_context);
	if (name == "double") type = llvm::Type::getDoubleTy(ir_context);
	if (name == "double*") type = llvm::Type::getDoublePtrTy(ir_context);

	if (type == NULL)
		ErrorExit("�������Ͷ������", tokens);

	tokens.erase(tokens.begin());
	if (tokens.size() > 0 && tokens[0].Value == "*")
	{
		tokens.erase(tokens.begin());
	}
	return type;
}

//���ұ���
VAR_INFO ir_var(std::string name, std::vector<VAR_LIST> var_list, TOKEN token)
{
	//�������ϲ��ұ�������
	while (!var_list.empty())
	{
		VAR_LIST vlist = var_list.back();
		var_list.pop_back();

		if (vlist.info.find(name) == vlist.info.end())
		{
			std::vector<TOKEN> tmp;
			tmp.push_back(token);
			ErrorExit("ERROR: ����������", tmp);
		}
		VAR_INFO vinfo = vlist.info[name];
		if (vinfo.value)
		{
			return vinfo;
		}
	}
}
//��ȡ����ֵ
llvm::Value* ir_var_load(VAR_INFO& var_info)
{
	return ir_builder->CreateLoad(var_info.type, var_info.value);
}

//����ת��
llvm::Value* ir_type_conver(llvm::Value* value,llvm::Type* to)
{
	llvm::Type* src = value->getType();
	if (src == to)
		return value;
	if (src->isIntegerTy())
		if (to->isIntegerTy(1))
			return ir_builder->getInt1(value != 0);
	
	printf("ERR! type conver!");
	exit(1);
}

std::map<std::string,int> IR_EXPR_PRI =
{
	{"*",30},
	{"/",30},
	{"+",20},
	{"-",20},
	{"=",10}
};


//��++��--���������д���
AST* ast_parse_expr_add1(AST* old, std::vector<TOKEN>& tokens)
{
	if (tokens[0].type == TOKEN_TYPE::opcode && (tokens[0].Value == "++" || tokens[0].Value=="--"))
	{
		TOKEN opa = tokens[0];
		if(tokens[0].Value == "++")
			opa.Value = "+";
		else
			opa.Value = "-";

		TOKEN code1 = tokens[0];
		code1.type = TOKEN_TYPE::number;
		code1.Value = "1";
		AST_value* value1 = new AST_value(code1);

		AST* add = new AST_expr(old, opa, value1);

		TOKEN ope = tokens[0];
		ope.Value = "=";

		AST* ret = new AST_expr(old, ope, add);

		tokens.erase(tokens.begin());

		return ret;
	}
	return old;
}
//��ȡһ�����ʽ
AST* ast_parse_expr1(std::vector<TOKEN>& tokens);
//�������ʽ
AST* ast_parse_expr(std::vector<TOKEN>& tokens, int left_pri = 0, AST* left = NULL)
{
	if (left_pri == 0) //�״ζ�ȡ
	{
		//if (tokens[0].type == TOKEN_TYPE::opcode && tokens[0].Value == "(")
		//{
		//	tokens.erase(tokens.begin());
		//	left = ast_parse_expr(tokens);
		//}
		//else
		//	left = new AST_value(tokens);
		left = ast_parse_expr1(tokens);
	}

	left = ast_parse_expr_add1(left, tokens);

	while (!tokens.empty())
	{
		int right_pri = 0;
		TOKEN op;
		//��ȡ��һ��������������������ȼ�
		if (tokens[0].type == TOKEN_TYPE::opcode)
		{
			if (tokens[0].Value == ")")
			{
				tokens.erase(tokens.begin());
				return left;
			}
			if (tokens[0].Value == ";" || tokens[0].Value == ",")
				return left;
			
			if (IR_EXPR_PRI.find(tokens[0].Value) == IR_EXPR_PRI.end())
				ErrorExit("δʶ��������", tokens);
			right_pri = IR_EXPR_PRI[tokens[0].Value];
			op = tokens[0];
			tokens.erase(tokens.begin());
		}
		if (right_pri < left_pri)
			return left;

		//if (tokens.size() > 2 && tokens[1].type == TOKEN_TYPE::opcode && tokens[1].Value == "++")
		//{
		//	std::vector<TOKEN> tmp;
		//	{
		//		TOKEN t=tokens[1];
		//		t.Value = "(";
		//		tmp.push_back(t);
		//	}
		//	tmp.push_back(tokens[0]);
		//	{
		//		TOKEN t = tokens[1];
		//		t.Value = "=";
		//		tmp.push_back(t);
		//	}
		//	tmp.push_back(tokens[0]);
		//	{
		//		TOKEN t = tokens[1];
		//		t.Value = "+";
		//		tmp.push_back(t);
		//	}
		//	{
		//		TOKEN t = tokens[0];
		//		t.type = TOKEN_TYPE::number;
		//		t.Value = "1";
		//		tmp.push_back(t);
		//	}
		//	{
		//		TOKEN t = tokens[1];
		//		t.Value = ")";
		//		tmp.push_back(t);
		//	}
		//	//
		//	tokens.erase(tokens.begin());
		//	tokens.erase(tokens.begin());
		//	tokens.insert(tokens.begin(), tmp.begin(),tmp.end());
		//}

		AST* right = ast_parse_expr1(tokens);

		int next_pri = 0;
		if (!tokens.empty() && tokens[0].type == TOKEN_TYPE::opcode)
		{
			if (IR_EXPR_PRI.find(tokens[0].Value) != IR_EXPR_PRI.end())
				next_pri = IR_EXPR_PRI[tokens[0].Value];
		}

		if(right_pri<next_pri || op.Value=="=" && right_pri == next_pri)
		{
			right = ast_parse_expr(tokens, right_pri, right);
			if (!right)
				return nullptr;
		}
		left = new AST_expr(left, op, right);


		//AST* right = ast_parse_expr1(tokens);
		//if (right_pri > left_pri)
		//{
		//	right = ast_parse_expr(tokens, right_pri, right);
		//	if (!right)
		//		return nullptr;
		//}
		//left = new AST_expr(left, op, right);
		//left_pri = right_pri;
	}

	return left;
}
//��ȡһ�����ʽ
AST* ast_parse_expr1(std::vector<TOKEN>& tokens)
{
	if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::opcode && tokens[1].Value == "(")
	{
		return new AST_call(tokens);
	}
	if (tokens[0].type == TOKEN_TYPE::opcode && tokens[0].Value == "(")
	{
		tokens.erase(tokens.begin());
		return ast_parse_expr(tokens);
	}
	return new AST_value(tokens);
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_call
//
////////////////////////////////////////////////////////////////////////////////


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
		if (tokens[0].type != TOKEN_TYPE::string && tokens[0].Value == ")") break;
		//args.push_back(new AST_expr(tokens));
		////args.push_back(tokens[0]);
		args.push_back(ast_parse_expr(tokens));
		if (tokens[0].Value == ",")
			tokens.erase(tokens.begin());
		else if (tokens[0].Value == ")")
		{
			tokens.erase(tokens.begin());
			return;
		}
		else if (tokens[0].Value == ";")
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
	std::cout << pre << "#TYPE:call" << std::endl;
	std::cout << pre << " name:";
	token_echo(name,"");
	std::cout << pre << " args:" << std::endl;
	for (auto a : args)
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
		ErrorExit("δ�ҵ���������",name);
	return NULL;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_codeblock
//
////////////////////////////////////////////////////////////////////////////////

AST_codeblock::AST_codeblock(std::vector<TOKEN>& tokens)
{
	if (tokens[0].type == TOKEN_TYPE::opcode && tokens[0].Value == "{")
		tokens.erase(tokens.begin());
	body = ast(tokens);
	//while (!tokens.empty())
	//{
	//	if (tokens[0].type != TOKEN_TYPE::string && tokens[0].Value == "}")
	//		break;
	//	body.push_back(ast1(tokens));
	//}
}
void AST_codeblock::show(std::string pre)
{
	std::cout << pre << "#TYPE:codeblock" << std::endl;
	for (AST* a : body)
	{
		a->show(pre + "      ");
	}
	std::cout << std::endl;
}
llvm::Value* AST_codeblock::codegen()
{
	//���õ�ǰ����������
	VAR_LIST varlist;
	varlist.zone = "codeblock";
	ir_varlist.push_back(varlist);

	//llvm::Function* TheFunction = ir_builder->GetInsertBlock()->getParent();
	//llvm::BasicBlock* bb = llvm::BasicBlock::Create(ir_context,"codeblock",TheFunction);
	//ir_builder->SetInsertPoint(bb);
	llvm::Value* bb = nullptr;

	for (auto a : body)
		a->codegen();
			
	ir_varlist.pop_back();
	return bb;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_expr
//
////////////////////////////////////////////////////////////////////////////////


//AST_expr::AST_expr(std::vector<TOKEN>& tokens)
//{
//	while (true)
//	{
//		AST_BINARYOP bop;
//		if (tokens[0].type == TOKEN_TYPE::opcode)
//		{
//			if (tokens[0].Value == ";" || tokens[0].Value == ",")
//				break;
//			if (tokens[0].Value == ")")
//			{
//				tokens.erase(tokens.begin());
//				break;
//			}
//			bop.op = tokens[0];
//			bop.op_pri = IR_EXPR_PRI[bop.op.Value];
//			tokens.erase(tokens.begin());
//		}
//		if (tokens[0].type == TOKEN_TYPE::opcode && tokens[0].Value == "(")
//		{
//			tokens.erase(tokens.begin());
//			bop.right = new AST_expr(tokens);
//		}
//		else
//			bop.right = new AST_value(tokens);
//		blist.push_back(bop);
//	}
//}
//AST_expr::AST_expr(AST* left, TOKEN op, AST* right)
//{
//	this->left = left;
//	this->op = op;
//	this->right = right;
//	//�����������ȼ�
//	if (IR_EXPR_PRI.find(op.Value) == IR_EXPR_PRI.end())
//		ErrorExit("δʶ��������", op);
//	op_pri = IR_EXPR_PRI[op.Value];
//}
void AST_expr::show(std::string pre)
{
	std::cout<<pre << "#TYPE:expr" << std::endl;
	std::cout << pre << " left:" << std::endl;
	left->show(pre + "      ");
	std::cout << pre << "   op:";
	token_echo(op, "           ");
	if (right)
	{
		std::cout << pre << "right:" << std::endl;
		right->show(pre + "      ");
	}
	std::cout << std::endl;
}
llvm::Value* AST_expr::codegen()
{
	//	////ת��
	//	//ir_value(*current,irinfo);
	//	//ֻ��һ��ֵ���������Ĵ���
	//	if (right == NULL)
	//	{
	//		value=current.right_value;
	//		list.erase(list.begin()+index-1);
	//		continue;
	//	}

	//�жϸ�ֵ����
	if (op.Value == "=")
	{
		VAR_INFO var_info = ir_var(((AST_value*)left)->value.Value, ir_varlist, ((AST_value*)left)->value);
		llvm::Value* ret = right->codegen();
		ir_builder->CreateStore(ret, var_info.value);
		return ret;
	}

	llvm::Value* l = left->codegen();
	llvm::Value* r = right->codegen();
	if (l->getType() != r->getType())
		ErrorExit("���ʽ�������Ͳ�һ��", op);
	if (op.Value == "+")
	{
		return ir_builder->CreateAdd(l, r);
	}
	if (op.Value == "-")
	{
		return ir_builder->CreateSub(l, r);
	}
	if (op.Value == "*")
	{
		return ir_builder->CreateMul(l, r);
	}
	if (op.Value == "/")
	{
		return ir_builder->CreateSDiv(l, r);
	}
	ErrorExit("��֧�ֵ������", op);
}



////////////////////////////////////////////////////////////////////////////////
//
// AST_function
//
////////////////////////////////////////////////////////////////////////////////


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
		//body = ast(tokens);
		ErrorExit("�����岿�ֻ�δʵ��", tokens);
	}
	else
		ErrorExit("���������������", tokens);
}
void AST_function::show(std::string pre)
{
	std::cout << pre << "#TYPE:function" << std::endl;
	std::cout << pre << " rett:";
	token_echo(rett, pre + "      ");
	std::cout << pre << " name:";
	token_echo(name, pre);
	std::cout << pre << " args:";
	token_echo(args, pre + "      ");
	std::cout << std::endl;
}
llvm::Value* AST_function::codegen()
{
	//args
	llvm::Type* frtype = ir_type(rett);
	std::string fname = name.Value;
	std::vector<llvm::Type*> fatype;
	std::vector<std::string> faname;
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
		VAR_LIST varlist;
		varlist.zone = "function";
		ir_varlist.push_back(varlist);

		for (auto& a : body)
			a->codegen();

		ir_varlist.pop_back();
	}
	return function;
}



////////////////////////////////////////////////////////////////////////////////
//
// AST_if
//
////////////////////////////////////////////////////////////////////////////////


AST_if::AST_if(std::vector<TOKEN>& tokens)
{
	//����
	name = tokens[0];
	tokens.erase(tokens.begin());
	//����
	if (tokens[0].Value == "(")
		tokens.erase(tokens.begin());
	else
		ErrorExit("if����������ֽ�������", tokens);
	
	//��������
	expr1 = ast_parse_expr1(tokens);
	//while (!tokens.empty())
	//	if (tokens[0].Value == ")")
	//	{
	//		break;
	//	}
	//	else
	//	{
	//		expr1.push_back(tokens[0]);
	//		tokens.erase(tokens.begin());
	//	}
	//�Ƴ�)
	if (tokens[0].Value == ")")
		tokens.erase(tokens.begin());
	else
		ErrorExit("if����������ִ���", tokens);
	
	//�жϺ����Ƿ���ں�����
	if (tokens[0].Value == ";")
	{
		tokens.erase(tokens.begin());
		return;
	}
	thenbody = ast1(tokens);

	if (tokens[0].Value == ";")
	{
		tokens.erase(tokens.begin());
	}
	if (!tokens.empty() && tokens[0].Value == "else")
	{
		tokens.erase(tokens.begin());
		elsebody = ast1(tokens);
	}

}
void AST_if::show(std::string pre)
{
	std::cout << pre << "#TYPE:if" << std::endl;
	std::cout << pre << " expr1:";
	expr1->show("");
	if (thenbody)
	{
		std::cout << pre << " then:" << std::endl;
		thenbody->show(pre + "      ");
	}
	if (elsebody)
	{
		std::cout << pre << " else:"<<std::endl;
		elsebody->show(pre+"      ");
	}
	std::cout << std::endl;
}
llvm::Value* AST_if::codegen()
{
	llvm::BasicBlock* bb = ir_builder->GetInsertBlock();

	llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* thenbb = llvm::BasicBlock::Create(ir_context, "",func);
	llvm::BasicBlock* elsebb = llvm::BasicBlock::Create(ir_context, "",func);
	llvm::BasicBlock* enddbb = llvm::BasicBlock::Create(ir_context, "",func);

	llvm::Value* expr1v = ir_type_conver(expr1->codegen(), llvm::Type::getInt1Ty(ir_context));
	//expr1v = ir_builder->getInt1(true);
	ir_builder->CreateCondBr(expr1v, thenbb, elsebb);

	ir_builder->SetInsertPoint(thenbb);
	if(thenbody)
		thenbody->codegen();
	ir_builder->CreateBr(enddbb);

	ir_builder->SetInsertPoint(elsebb);
	if(elsebody)
		elsebody->codegen();
	ir_builder->CreateBr(enddbb);

	ir_builder->SetInsertPoint(enddbb);

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_if
//
////////////////////////////////////////////////////////////////////////////////


AST_for::AST_for(std::vector<TOKEN>& tokens)
{
	//����
	name = tokens[0];
	tokens.erase(tokens.begin());
	//����
	if (tokens[0].Value == "(")
		tokens.erase(tokens.begin());
	else
		ErrorExit("for����������ֽ�������", tokens);

	//��������
	expr1 = ast_parse_expr1(tokens);
	//�Ƴ�;
	if (tokens[0].Value == ";")
		tokens.erase(tokens.begin());
	else
		ErrorExit("for���岿��1����", tokens);

	expr2 = ast_parse_expr1(tokens);
	//�Ƴ�;
	if (tokens[0].Value == ";")
		tokens.erase(tokens.begin());
	else
		ErrorExit("for���岿��2����", tokens);

	expr3 = ast_parse_expr1(tokens);
	//�Ƴ�)
	if (tokens[0].Value == ")")
		tokens.erase(tokens.begin());
	else
		ErrorExit("for���岿��3����", tokens);

	//�жϺ����Ƿ���ں�����
	if (tokens[0].Value == ";")
	{
		tokens.erase(tokens.begin());
		return;
	}
	body = ast1(tokens);

	if (tokens[0].Value == ";")
	{
		tokens.erase(tokens.begin());
	}
}
void AST_for::show(std::string pre)
{
	std::cout << pre << "#TYPE:if" << std::endl;
	std::cout << pre << " expr1:";
	expr1->show("");
	std::cout << pre << " expr2:";
	expr2->show("");
	std::cout << pre << " expr3:";
	expr3->show("");
	if (body)
	{
		std::cout << pre << " then:" << std::endl;
		body->show(pre + "      ");
	}
	std::cout << std::endl;
}
llvm::Value* AST_for::codegen()
{
	llvm::BasicBlock* bb = ir_builder->GetInsertBlock();

	llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* bodybb = llvm::BasicBlock::Create(ir_context, "", func);
	llvm::BasicBlock* enddbb = llvm::BasicBlock::Create(ir_context, "", func);

	expr1->codegen();

	ir_builder->SetInsertPoint(bodybb);
	llvm::Value* expr2v=expr2->codegen();
	ir_builder->CreateCondBr(expr2v, bodybb, enddbb);
	if(body)
		body->codegen();
	expr3->codegen();
	ir_builder->SetInsertPoint(enddbb);

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_noncode
//
////////////////////////////////////////////////////////////////////////////////


AST_noncode::AST_noncode(std::vector<TOKEN>& tokens)
{
	value = tokens[0];
	tokens.erase(tokens.begin());
}
void AST_noncode::show(std::string pre)
{
	std::cout << pre << "#TYPE:noncode" << std::endl;
	std::cout << pre << "value:";
	token_echo(value, pre + "      ");
	std::cout << std::endl;
}
llvm::Value* AST_noncode::codegen()
{
	llvm::Function* function = ir_module->getFunction("printf");
	if (function)
	{
		llvm::Value* args = ir_builder->CreateGlobalStringPtr(value.Value);
		ir_builder->CreateCall(function, args);
	}
	else
		ErrorExit("δ�ҵ��Ǵ����������",value);
	return NULL;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_value
//
////////////////////////////////////////////////////////////////////////////////


AST_value::AST_value(std::vector<TOKEN>& tokens)
{
	value = tokens[0];
	tokens.erase(tokens.begin());
}
void AST_value::show(std::string pre)
{
	std::cout<<pre << "#TYPE:value";
	token_echo(value,"");
	std::cout << std::endl;
}
llvm::Value* AST_value::codegen()
{
	if (value.type == TOKEN_TYPE::string)
		return ir_builder->CreateGlobalStringPtr(value.Value);

	if (value.type == TOKEN_TYPE::number)
	{
		int v = atoi(value.Value.c_str());
		return ir_builder->getInt32(v);
	}

	//else if (current.right.value.empty()) //û��ֵ�ڵ�ģ������²�ڵ�
	//	current.right_value = ir_expr(current.right.body["body"], irinfo);
	//else
	//{
	
	//���ұ���
	VAR_INFO vinfo = ir_var(value.Value, ir_varlist, value);
	return ir_var_load(vinfo);

	//if(current.right_value==NULL)

//	std::vector<TOKEN> tmp;
//	tmp.push_back(value);
//			ErrorExit("δʶ��ı��ʽ����", tmp);
	//}
	//return NULL;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_var
//
////////////////////////////////////////////////////////////////////////////////


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
	var_info.type= ir_type(type);
	var_info.value = ir_builder->CreateAlloca(var_info.type);
	ir_varlist[ir_varlist.size()-1].info[name.Value] = var_info;
	return var_info.value;
}


////////////////////////////////////////////////////////////////////////////////
//
// ast
//
////////////////////////////////////////////////////////////////////////////////


//AST����
//	oneblock ֻ����һ��
AST* ast1(std::vector<TOKEN>& tokens)
{
	while (!tokens.empty())
	{
		if (tokens[0].type == TOKEN_TYPE::noncode)//�Ǵ��룬ֱ�����
		{
			return new AST_noncode(tokens);
		}

		//if (tokens[0].type == TOKEN_TYPE::string)
		//{
		//	return ast_parse_expr(tokens);
		//}

		if (tokens[0].type != TOKEN_TYPE::string)
		{
			if (tokens[0].Value == "{")
			{
				return new AST_codeblock(tokens);
			}
			else if (tokens[0].Value == "}")
			{
				//tokens.erase(tokens.begin());
				return NULL;
			}
			else if (tokens[0].Value == ";")
			{
				tokens.erase(tokens.begin());
				return NULL;
			}

			if (tokens[0].Value == "if" && tokens[1].Value == "(")
			{
				return new AST_if(tokens);
			}

			if (tokens[0].Value == "for" && tokens[1].Value == "(") return new AST_for(tokens);

			//Function
			//	Ex: int function_name(int arg1)
			//		 |		|              + args
			//       |      + ��������
			//       +  ����ֵ����
			//�ж����ݣ���ǰΪ�ַ�����һ��token��һ���ַ�������һ����(
			if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::code && tokens[2].type == TOKEN_TYPE::opcode && tokens[2].Value == "(")
			{
				return new AST_function(tokens);
			}

			//��������
			//Ex:	printf("abc");
			if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::opcode && tokens[1].Value == "(")
			{
				return new AST_call(tokens);
			}

			//��������
			//Ex:	int a;
			//		int b=2;
			if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::code && tokens[1].Value != "(")
			{
				return new AST_var(tokens);
			}
		}

		//ast_list.push_back(new AST_expr(tokens));
		return ast_parse_expr(tokens);
	}
}
std::vector<AST*> ast(std::vector<TOKEN>& tokens)
{
	std::vector<AST*> ast_list;
	while (!tokens.empty())
	{
		if (tokens[0].type != TOKEN_TYPE::string && tokens[0].Value == "}")
		{
			tokens.erase(tokens.begin());
			break;
		}
		AST* a = ast1(tokens);
		if (a)
			ast_list.push_back(a);
	}
	return ast_list;
}
void ast_echo(std::vector<AST*> ast_list, std::string pre)
{
	for (AST* a : ast_list)
	{
		a->show(pre);
	}
}

//	THE END