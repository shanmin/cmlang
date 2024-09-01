//
//	ast.cpp
//

#include "colang.h"

std::unique_ptr<llvm::IRBuilder<>> ir_builder;
llvm::Module* ir_module;
llvm::LLVMContext ir_context;



//std::vector<VAR_LIST> ir_varlist; //�ֲ�������Χ
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
//	oneblock ֻ����һ�Σ�ֱ����}����;����βΪֹ��
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
			if (tokens[0].Value == "return") return new AST_return(tokens);

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



////////////////////////////////////////////////////////////////////////////////
//
// ir
//
////////////////////////////////////////////////////////////////////////////////


void ir(std::vector<AST*>& ast_list, const char* filename)
{
	ir_module = new llvm::Module(filename, ir_context);
	ir_builder = std::make_unique<llvm::IRBuilder<>>((ir_context));

	//���õ�ǰȫ�ֱ���������
	scope::push("global");

	LABEL_LIST labellist;
	ir_labellist.push_back(labellist);

	llvm::FunctionType* mainType = llvm::FunctionType::get(ir_builder->getInt32Ty(), false);
	llvm::Function* main = llvm::Function::Create(mainType, llvm::GlobalValue::ExternalLinkage, "main", ir_module);
	llvm::BasicBlock* entryMain = llvm::BasicBlock::Create(ir_context, "entry_main", main);
	ir_builder->SetInsertPoint(entryMain);

	//ir_proc(tokens, true);
	for (auto a : ast_list)
		a->codegen();

	std::string mstr;

	llvm::ConstantInt* ret = ir_builder->getInt32(0);
	ir_builder->CreateRet(ret);
	//bool result1 = llvm::verifyFunction(*main);
	//if (result1)
	//{
	//	ir_module->print(llvm::outs(), nullptr);
	//	printf("\n---------- ERROR ----------\n");
	//	printf("ģ����ڴ���%s", mstr.c_str());
	//	printf("\n---- vf = %d\n", result1);
	//	exit(3);
	//}

	//��֤ģ���Ƿ��������
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
	if (tokens.size() > 0 && tokens[0].type == TOKEN_TYPE::opcode && tokens[0].Value == "*")
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




////���ұ���
////	name	��������
////	var_list	�����б�
//VARINFO ir_var(std::string name, std::vector<VAR_LIST> var_list, TOKEN token)
//{
//	//if (name == "str")
//	//	printf("aa");
//	// 
//	//�������ϲ��ұ�������
//	while (!var_list.empty())
//	{
//		VAR_LIST vlist = var_list.back();
//		var_list.pop_back();
//
//		if (vlist.info.find(name) == vlist.info.end())
//		{
//			continue;
//		}
//		VAR_INFO vinfo = vlist.info[name];
//		if (vinfo.value)
//		{
//			return vinfo;
//		}
//		//�����ǰ��function���壬����һ��ת��ȫ�ֱ��� 20240411 shanmin
//		if (vlist.zone == "function")
//			var_list.resize(1);
//	}
//	//
//	std::vector<TOKEN> tmp;
//	tmp.push_back(token);
//	ErrorExit("ERROR: ����������", tmp);
//}
//��ȡ����ֵ
llvm::Value* ir_var_load(VARINFO& var_info)
{
	return ir_builder->CreateLoad(var_info.type, var_info.value);
}



//	THE END