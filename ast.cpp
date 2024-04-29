//
//	ast.cpp
//

#include <iostream>
#include <map>
#include "ast.h"
#include <llvm/Bitcode/BitcodeWriter.h>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

llvm::LLVMContext ir_context;
llvm::Module* ir_module;
std::unique_ptr<llvm::IRBuilder<>> ir_builder;


std::vector<VAR_LIST> ir_varlist; //�ֲ�������Χ
std::vector<LABEL_LIST> ir_labellist; //�ֲ���ǩ

std::map<std::string, int> IR_EXPR_PRI =
{
	{"*",6},
	{"/",6},
	{"+",5},
	{"-",5},
	{">",4},
	{"<",4},
	{">=",4},
	{"<=",4},
	{"==",4},
	{"!=",4},
	{"&&",3},
	{"||",2},
	{"=",1}
};


////////////////////////////////////////////////////////////////////////////////
//
// ir
//
////////////////////////////////////////////////////////////////////////////////


void ir(std::vector<AST*>& ast_list, const char* filename)
{
	ir_module = new llvm::Module(filename, ir_context);
	ir_builder = std::make_unique<llvm::IRBuilder<>>((ir_context));

	//���õ�ǰ����������
	VAR_LIST varlist;
	varlist.zone = "global";
	ir_varlist.push_back(varlist);

	LABEL_LIST labellist;
	ir_labellist.push_back(labellist);

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
		ir_module->print(llvm::outs(), nullptr);
		printf("\n---------- ERROR ----------\n");
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


//�������ͣ���token�л�ȡ��Ӧ��ʵ����
llvm::Type* ir_type(std::vector<TOKEN>& tokens)
{
	llvm::Type* type = NULL;
	std::string code = tokens[0].Value;

	if (code == "void") type = llvm::Type::getVoidTy(ir_context);
	else if (code == "char") type = llvm::Type::getInt8Ty(ir_context);
	else if (code == "short") type = llvm::Type::getInt16Ty(ir_context);
	else if (code == "int") type = llvm::Type::getInt32Ty(ir_context);
	else if (code == "long") type = llvm::Type::getInt64Ty(ir_context);
	else if (code == "float") type = llvm::Type::getFloatTy(ir_context);
	else if (code == "double") type = llvm::Type::getDoubleTy(ir_context);
	else ErrorExit("�������Ͷ������", tokens);

	tokens.erase(tokens.begin());
	//�����ָ�룬��ת��ָ������
	if (tokens.size() > 0 && tokens[0].type==TOKEN_TYPE::opcode && tokens[0].Value == "*")
	{
		type = type->getPointerTo();
		code += "*";
		tokens.erase(tokens.begin());
	}

	return type;
}


//����ת��
llvm::Value* ir_type_conver(llvm::Value* value, llvm::Type* to)
{
	llvm::Type* src = value->getType();
	if (src == to)
		return value;
	if (src->isIntegerTy())
		if (to->isIntegerTy(1))
		{
			llvm::Value* i0 = llvm::ConstantInt::get(src, 0);
			return ir_builder->CreateICmpNE(value, i0);
		}

	printf("ERR! type conver!");
	exit(1);
}


//���ұ���
VAR_INFO ir_var(std::string name, std::vector<VAR_LIST> var_list, TOKEN token)
{
	//if (name == "str")
	//	printf("aa");
	// 
	//�������ϲ��ұ�������
	while (!var_list.empty())
	{
		VAR_LIST vlist = var_list.back();
		var_list.pop_back();

		if (vlist.info.find(name) == vlist.info.end())
		{
			continue;
		}
		VAR_INFO vinfo = vlist.info[name];
		if (vinfo.value)
		{
			return vinfo;
		}
		//�����ǰ��function���壬����һ��ת��ȫ�ֱ��� 20240411 shanmin
		if (vlist.zone == "function")
			var_list.resize(1);
	}
	//
	std::vector<TOKEN> tmp;
	tmp.push_back(token);
	ErrorExit("ERROR: ����������", tmp);
}
//��ȡ����ֵ
llvm::Value* ir_var_load(VAR_INFO& var_info)
{
	return ir_builder->CreateLoad(var_info.type, var_info.value);
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
	std::cout << pre << "#TYPE:call" << std::endl;
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
	//llvm::Value* bb = nullptr;

	for (auto a : body)
		a->codegen();

	ir_varlist.pop_back();
	//return bb;
	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_do
//
////////////////////////////////////////////////////////////////////////////////


AST_do::AST_do(std::vector<TOKEN>& tokens)
{
	//����
	name = tokens[0];
	tokens.erase(tokens.begin());

	//�жϺ����Ƿ���ں�����
	body = ast1(tokens);

	if (tokens[0].type != TOKEN_TYPE::string && tokens[0].Value == ";")
		tokens.erase(tokens.begin());

	//����
	if (tokens[0].Value == "while" && tokens.size() > 1 && tokens[1].Value == "(")
	{
		tokens.erase(tokens.begin());
		tokens.erase(tokens.begin());
	}
	else
		ErrorExit("do..while����������ֽ�������", tokens);

	//��������
	expr = ast_parse_expr(tokens);

	if (tokens[0].Value == ";")
	{
		tokens.erase(tokens.begin());
	}
}
void AST_do::show(std::string pre)
{
	std::cout << pre << "#TYPE:do" << std::endl;
	if (body)
	{
		std::cout << pre << " body:" << std::endl;
		body->show(pre + "      ");
	}
	std::cout << pre << " expr:" << std::endl;
	expr->show(pre + "      ");
	std::cout << std::endl;
}
llvm::Value* AST_do::codegen()
{
	// do
	//	 code;
	// while(expr)
	//
	// start:
	//	 code;
	// if(expr)
	//	 goto start;
	// over:

	llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* bbbody = llvm::BasicBlock::Create(ir_context, "", func);
	llvm::BasicBlock* bbover = llvm::BasicBlock::Create(ir_context, "", func);

	ir_builder->CreateBr(bbbody);

	ir_builder->SetInsertPoint(bbbody);
	if (body)
		body->codegen();

	llvm::Value* expr2v = ir_type_conver(expr->codegen(), llvm::Type::getInt1Ty(ir_context));
	ir_builder->CreateCondBr(expr2v, bbbody, bbover);

	ir_builder->SetInsertPoint(bbover);

	return nullptr;
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
	std::cout << pre << "#TYPE:expr" << std::endl;
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
	if (op.Value == "+") return ir_builder->CreateAdd(l, r);
	else if (op.Value == "-") return ir_builder->CreateSub(l, r);
	else if (op.Value == "*") return ir_builder->CreateMul(l, r);
	else if (op.Value == "/") return ir_builder->CreateSDiv(l, r);
	else if (op.Value == ">") return ir_builder->CreateICmpSGT(l, r);
	else if (op.Value == "<") return ir_builder->CreateICmpSLT(l, r);
	else if (op.Value == ">=") return ir_builder->CreateICmpSGE(l, r);
	else if (op.Value == "<=") return ir_builder->CreateICmpSLE(l, r);
	else if (op.Value == "==") return ir_builder->CreateICmpEQ(l, r);
	else if (op.Value == "!=") return ir_builder->CreateICmpNE(l, r);
	else if (op.Value == "&" or op.Value == "&&") return ir_builder->CreateAnd(l, r);
	else if (op.Value == "|" or op.Value == "||") return ir_builder->CreateOr(l, r);

	ErrorExit("��֧�ֵ������", op);
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_for
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
	expr1 = ast_parse_expr(tokens);
	//�Ƴ�;
	if (tokens[0].Value == ";")
		tokens.erase(tokens.begin());
	else
		ErrorExit("for���岿��1����", tokens);

	expr2 = ast_parse_expr(tokens);
	//�Ƴ�;
	if (tokens[0].Value == ";")
		tokens.erase(tokens.begin());
	else
		ErrorExit("for���岿��2����", tokens);

	expr3 = ast_parse_expr(tokens);
	//����������ʽ��ȡʱ�Ѿ��Ƴ�
	////�Ƴ�)
	//if (tokens[0].Value == ")")
	//	tokens.erase(tokens.begin());
	//else
	//	ErrorExit("for���岿��3����", tokens);

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
	std::cout << pre << "#TYPE:for" << std::endl;
	std::cout << pre << " expr1:" << std::endl;
	expr1->show(pre + "      ");
	std::cout << pre << " expr2:" << std::endl;
	expr2->show(pre + "      ");
	std::cout << pre << " expr3:" << std::endl;
	expr3->show(pre + "      ");
	if (body)
	{
		std::cout << pre << " body:" << std::endl;
		body->show(pre + "      ");
	}
	std::cout << std::endl;
}
llvm::Value* AST_for::codegen()
{
	//for (for1; for2; for3)
	//	for4;
	//
	//for1
	//if(for2) for4 else for3
	//for4
	//for3
	//goto for2;

	//llvm::BasicBlock* bb = ir_builder->GetInsertBlock();

	llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* bbexpr = llvm::BasicBlock::Create(ir_context, "", func);
	llvm::BasicBlock* bbbody = llvm::BasicBlock::Create(ir_context, "", func);
	llvm::BasicBlock* bbover = llvm::BasicBlock::Create(ir_context, "", func);

	expr1->codegen();
	ir_builder->CreateBr(bbexpr); //������һ����ת������ǰ���BasicBlock����

	ir_builder->SetInsertPoint(bbexpr);
	llvm::Value* expr2v = ir_type_conver(expr2->codegen(), llvm::Type::getInt1Ty(ir_context));
	ir_builder->CreateCondBr(expr2v, bbbody, bbover);

	ir_builder->SetInsertPoint(bbbody);
	if (body)
		body->codegen();
	expr3->codegen();
	ir_builder->CreateBr(bbexpr);
	//ir_builder->CreateBr(forend);

	ir_builder->SetInsertPoint(bbover);
	//ir_builder->SetInsertPoint(bb);

	return nullptr;
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
		body = ast(tokens);
		//ErrorExit("�����岿�ֻ�δʵ��", tokens);
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
		VAR_LIST varlist;
		varlist.zone = "function";
		for (int i = 0; i < fatype.size(); i++)
		{
			VAR_INFO vi;
			vi.type = fatype[i];
			vi.value = ir_builder->CreateAlloca(function->getArg(i)->getType(), function->getArg(i));
			varlist.info[faname[i]] = vi;
		}
		ir_varlist.push_back(varlist);
		//��ǰ��ǩ��
		LABEL_LIST lablelist;
		ir_labellist.push_back(lablelist);

		for (auto& a : body)
			a->codegen();

		//�����������ǩ������
		ir_varlist.pop_back();
		ir_labellist.pop_back();
	}
	return function;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_goto
//
////////////////////////////////////////////////////////////////////////////////

AST_goto::AST_goto(std::vector<TOKEN>& tokens)
{
	//����ֵ
	tokens.erase(tokens.begin());
	if (tokens[0].type == TOKEN_TYPE::code)
	{
		name = tokens[0];
		tokens.erase(tokens.begin());
	}
	else
		ErrorExit("goto���ֽ�������", tokens);
	
	//if (tokens[0].Value == ";")
	//	tokens.erase(tokens.begin());
	//else
	//	ErrorExit("goto���ֽ���������", tokens);
}
void AST_goto::show(std::string pre)
{
	std::cout << pre << "#TYPE:goto" << std::endl;
	token_echo(name, pre + "");
	std::cout << std::endl;
}
llvm::Value* AST_goto::codegen()
{
	llvm::BasicBlock* bbstart;
	//���goto��ǰ����ǰ���Ѿ����������ǩ��
	LABEL_LIST labellist = ir_labellist.back();
	if (labellist.info.find(name.Value) == labellist.info.end())
	{
		llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
		bbstart = llvm::BasicBlock::Create(ir_context,name.Value, func);
		ir_labellist[ir_labellist.size() - 1].info[name.Value] = bbstart;
	}
	else
	{
		bbstart =labellist.info[name.Value];
	}
	ir_builder->CreateBr(bbstart);

	//goto����Ҫ�³���һ��basic block���������֡�Terminator found in the middle of a basic block!��������Ϣ
	{
		llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
		llvm::BasicBlock* bbstart1 = llvm::BasicBlock::Create(ir_context,"", func);
		ir_builder->SetInsertPoint(bbstart1);
	}
	
	return nullptr;
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

	//std::vector<TOKEN> e = get_tokens(tokens, ")");
	//expr1 = ast_parse_expr(e);

	//��������
	expr1 = ast_parse_expr(tokens);
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
	// 
	////�Ƴ�)
	//if (tokens[0].Value == ")")
	//	tokens.erase(tokens.begin());
	//else
	//	ErrorExit("if����������ִ���", tokens);

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
	std::cout << pre << " expr:" << std::endl;
	expr1->show(pre + "      ");
	if (thenbody)
	{
		std::cout << pre << " then:" << std::endl;
		thenbody->show(pre + "      ");
	}
	if (elsebody)
	{
		std::cout << pre << " else:" << std::endl;
		elsebody->show(pre + "      ");
	}
	std::cout << std::endl;
}
llvm::Value* AST_if::codegen()
{
	//llvm::BasicBlock* bb = ir_builder->GetInsertBlock();

	llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* thenbb = llvm::BasicBlock::Create(ir_context, "", func);
	llvm::BasicBlock* elsebb = llvm::BasicBlock::Create(ir_context, "", func);
	llvm::BasicBlock* enddbb = llvm::BasicBlock::Create(ir_context, "", func);

	llvm::Value* expr1v = ir_type_conver(expr1->codegen(), llvm::Type::getInt1Ty(ir_context));
	ir_builder->CreateCondBr(expr1v, thenbb, elsebb);

	ir_builder->SetInsertPoint(thenbb);
	if (thenbody)
		thenbody->codegen();
	ir_builder->CreateBr(enddbb);

	ir_builder->SetInsertPoint(elsebb);
	if (elsebody)
		elsebody->codegen();
	ir_builder->CreateBr(enddbb);

	ir_builder->SetInsertPoint(enddbb);

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
//
// AST_label
//
////////////////////////////////////////////////////////////////////////////////

AST_label::AST_label(std::vector<TOKEN>& tokens)
{
	//����
	name = tokens[0];
	tokens.erase(tokens.begin());
	//����
	if (tokens[0].Value == ":")
		tokens.erase(tokens.begin());
	else
		ErrorExit("label����������ֽ�������", tokens);
}
void AST_label::show(std::string pre)
{
	std::cout << pre << "#TYPE:label    " << name.Value << std::endl << std::endl << std::endl;
}
llvm::Value* AST_label::codegen()
{
	llvm::BasicBlock* bbstart;
	//���goto��ǰ����ǰ���Ѿ����������ǩ��
	LABEL_LIST labellist = ir_labellist.back();
	if (labellist.info.find(name.Value) == labellist.info.end())
	{
		llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
		bbstart = llvm::BasicBlock::Create(ir_context, name.Value, func);
		ir_labellist[ir_labellist.size() - 1].info[name.Value] = bbstart;
	}
	else
	{
		bbstart = (llvm::BasicBlock*)labellist.info.find(name.Value)._Ptr;
	}
	ir_builder->CreateBr(bbstart);
	ir_builder->SetInsertPoint(bbstart);

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
	token_echo(value, pre + "           ");
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
		ErrorExit("δ�ҵ��Ǵ����������", value);
	return NULL;
}



////////////////////////////////////////////////////////////////////////////////
//
// AST_value
//
////////////////////////////////////////////////////////////////////////////////

//ʮ�������ַ�ת��
char c2x(char c)
{
	switch (c)
	{
	case '0':return 0;
	case '1':return 1;
	case '2':return 2;
	case '3':return 3;
	case '4':return 4;
	case '5':return 5;
	case '6':return 6;
	case '7':return 7;
	case '8':return 8;
	case '9':return 9;
	case 'a':
	case 'A':return 10;
	case 'b':
	case 'B':return 11;
	case 'c':
	case 'C':return 12;
	case 'd':
	case 'D':return 13;
	case 'e':
	case 'E':return 14;
	case 'f':
	case 'F':return 15;
	}
	return -1;
}

AST_value::AST_value(std::vector<TOKEN>& tokens)
{
	value = tokens[0];
	tokens.erase(tokens.begin());
}
void AST_value::show(std::string pre)
{
	std::cout << pre << "#TYPE:value";
	token_echo(value, "");
	//std::cout << std::endl;
}
llvm::Value* AST_value::codegen()
{
	if (value.type == TOKEN_TYPE::string)
		return ir_builder->CreateGlobalStringPtr(value.Value);

	if (value.type == TOKEN_TYPE::number)
	{
		if (value.Value.size() > 1 && (value.Value[1] == 'X' || value.Value[1] == 'x'))
		{
			std::string str = value.Value.substr(2);
			if (str.size() > 16)
			{
				std::vector<TOKEN> tmp;
				tmp.push_back(value);
				ErrorExit("too big", tmp);
			}
			long long data = 0;
			for (int i = 0; i < str.size(); i++)
			{
				long long c = c2x(str[i]);
				if (c == -1)
				{
					std::vector<TOKEN> tmp;
					tmp.push_back(value);
					ErrorExit("error", tmp);
				}
				data |= c << ((str.size() - i - 1) * 4);
			}
			return ir_builder->getInt64(data);
		}
		else if (value.Value.size() > 9)
		{
			long long v = atoll(value.Value.c_str());
			return ir_builder->getInt64(v);
		}
		else
		{
			int v = atoi(value.Value.c_str());
			return ir_builder->getInt32(v);
		}
		//return _value;
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
// AST_while
//
////////////////////////////////////////////////////////////////////////////////

AST_while::AST_while(std::vector<TOKEN>& tokens)
{
	//����
	name = tokens[0];
	tokens.erase(tokens.begin());
	//����
	if (tokens[0].Value == "(")
		tokens.erase(tokens.begin());
	else
		ErrorExit("while����������ֽ�������", tokens);

	//��������
	expr = ast_parse_expr(tokens);

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
void AST_while::show(std::string pre)
{
	std::cout << pre << "#TYPE:while" << std::endl;
	std::cout << pre << " expr:" << std::endl;
	expr->show(pre + "      ");
	if (body)
	{
		std::cout << pre << " body:" << std::endl;
		body->show(pre + "      ");
	}
	std::cout << std::endl;
}
llvm::Value* AST_while::codegen()
{
	//while(expr)
	//	code;
	//
	// start:
	// if(expr)
	//	 code;
	//	 goto start;
	// over:

	llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* bbstart = llvm::BasicBlock::Create(ir_context, "", func);
	llvm::BasicBlock* bbbody = llvm::BasicBlock::Create(ir_context, "", func);
	llvm::BasicBlock* bbover = llvm::BasicBlock::Create(ir_context, "", func);

	ir_builder->CreateBr(bbstart);

	ir_builder->SetInsertPoint(bbstart);
	llvm::Value* expr2v = ir_type_conver(expr->codegen(), llvm::Type::getInt1Ty(ir_context));
	ir_builder->CreateCondBr(expr2v, bbbody, bbover);

	ir_builder->SetInsertPoint(bbbody);
	if (body)
		body->codegen();
	ir_builder->CreateBr(bbstart);

	ir_builder->SetInsertPoint(bbover);

	return nullptr;
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
	var_info.type = ir_type(type);
	var_info.value = ir_builder->CreateAlloca(var_info.type);
	ir_varlist[ir_varlist.size() - 1].info[name.Value] = var_info;
	return var_info.value;
}


////////////////////////////////////////////////////////////////////////////////
//
// ast
//
////////////////////////////////////////////////////////////////////////////////


//����AST
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

			if (tokens[0].Value == "do") return new AST_do(tokens);
			if (tokens[0].Value == "for" && tokens[1].Value == "(") return new AST_for(tokens);
			if (tokens[0].Value == "goto") return new AST_goto(tokens);
			if (tokens[0].Value == "if" && tokens[1].Value == "(") return new AST_if(tokens);
			if (tokens[0].Value == "while" && tokens[1].Value == "(") return new AST_while(tokens);

			if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::opcode && tokens[1].Value == ":")
				return new AST_label(tokens);

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

		////ast_list.push_back(new AST_expr(tokens));
		return ast_parse_expr(tokens);
	}
}

//�������ʽ
//	tokens
//	left_pri=0
//	left=NULL
AST* ast_parse_expr(std::vector<TOKEN>& tokens, int left_pri, AST* left)
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

	left = ast_parse_expr_add1(left, tokens); //��++��--���������д���

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
		else
			return left;
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

		if (right_pri < next_pri || op.Value == "=" && right_pri == next_pri)
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

//��++��--���������д���
//	�����a++��дΪa=a+1
AST* ast_parse_expr_add1(AST* old, std::vector<TOKEN>& tokens)
{
	if (tokens[0].type == TOKEN_TYPE::opcode && (tokens[0].Value == "++" || tokens[0].Value == "--"))
	{
		TOKEN opa = tokens[0];
		if (tokens[0].Value == "++")
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

//��ʾAST��Ϣ
void ast_echo(std::vector<AST*> ast_list, std::string pre)
{
	for (AST* a : ast_list)
	{
		a->show(pre);
	}
}


//	THE END