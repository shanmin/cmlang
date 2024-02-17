//
//	IR
//

#include <vector>
#include <llvm/Bitcode/BitcodeWriter.h>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "lexer.h"
#include "ast.h"

void ErrorExit(const char* str, std::vector<AST>& ast_list)
{
	printf("\n---------- Error ----------\n%s\n", str);
	ast_echo(ast_list,"",true);
	exit(1);
}

struct VAR_INFO
{
	llvm::Type* type;
	llvm::AllocaInst* value;
};
struct VAR_LIST
{
	std::string zone; //global,codeblock,function
	std::map<std::string, VAR_INFO> info; //name,type
};

struct IRINFO
{
	llvm::LLVMContext context;
	llvm::Module* module;
	std::unique_ptr<llvm::IRBuilder<>> builder;

	std::vector<VAR_LIST> varlist; //�ֲ�������Χ
};


void ir_proc(std::vector<AST>& ast_list, IRINFO& irinfo,bool ismain);


void ir(std::vector<AST>& tokens,const char* filename)
{
	IRINFO irinfo;
	irinfo.module =new llvm::Module(filename, irinfo.context);
	irinfo.builder = std::make_unique<llvm::IRBuilder<>>((irinfo.context));

	//���õ�ǰ����������
	VAR_LIST varlist;
	varlist.zone = "global";
	irinfo.varlist.push_back(varlist);

	llvm::FunctionType* mainType = llvm::FunctionType::get(irinfo.builder->getInt32Ty(), false);
	llvm::Function* main = llvm::Function::Create(mainType, llvm::GlobalValue::ExternalLinkage, "main", irinfo.module);
	llvm::BasicBlock* entryMain = llvm::BasicBlock::Create(irinfo.context, "entry_main", main);
	irinfo.builder->SetInsertPoint(entryMain);

	ir_proc(tokens, irinfo,true);

	llvm::ConstantInt* ret =irinfo.builder->getInt32(0);
	irinfo.builder->CreateRet(ret);
	llvm::verifyFunction(*main);

	//��֤ģ���Ƿ��������
	std::string mstr;
	llvm::raw_string_ostream merr(mstr);
	bool result=llvm::verifyModule(*irinfo.module,&merr);
	if (result)
	{
		printf("ģ����ڴ���%s",mstr.c_str());
		exit(2);
	}

	irinfo.module->print(llvm::outs(), nullptr);

	//���ll��ʽ�ļ�
	std::string col = std::string(filename) + "l";
	std::error_code ec;
	llvm::raw_fd_ostream fout(col, ec);
	irinfo.module->print(fout, nullptr);
	fout.close();
	printf("\n---------- write ll ----------\n");
	printf("save=%x\n", ec.value());

	std::string cob = std::string(filename) + "b";
	llvm::raw_fd_ostream fout_cob(cob, ec);
	llvm::WriteBitcodeToFile(*irinfo.module, fout_cob);
	fout_cob.close();
	printf("\n---------- write bc ----------\n");
	printf("save=%x\n", ec.value());
}


llvm::Type* irtype(std::vector<TOKEN>& tokens, IRINFO& irinfo)
{
	std::string name = tokens[0].Value;
	if (tokens.size() > 1 && tokens[1].Value == "*")
	{
		name += "*";
	}

	llvm::Type* type = NULL;
	if (name == "void") type = llvm::Type::getVoidTy(irinfo.context);
	if (name == "int8" || name == "byte" || name == "char") type = llvm::Type::getInt8Ty(irinfo.context);
	if (name == "int8*" || name == "byte*" || name == "char*") type = llvm::Type::getInt8PtrTy(irinfo.context);
	if (name == "int16" || name == "short") type = llvm::Type::getInt16Ty(irinfo.context);
	if (name == "int16*" || name == "short*") type = llvm::Type::getInt16PtrTy(irinfo.context);
	if (name == "int32" || name == "int") type = llvm::Type::getInt32Ty(irinfo.context);
	if (name == "int32*" || name == "int*") type = llvm::Type::getInt32PtrTy(irinfo.context);
	if (name == "int64" || name == "long") type = llvm::Type::getInt64Ty(irinfo.context);
	if (name == "int64*" || name == "long*") type = llvm::Type::getInt64PtrTy(irinfo.context);
	if (name == "float") type = llvm::Type::getFloatTy(irinfo.context);
	if (name == "float*") type = llvm::Type::getFloatPtrTy(irinfo.context);
	if (name == "double") type = llvm::Type::getDoubleTy(irinfo.context);
	if (name == "double*") type = llvm::Type::getDoublePtrTy(irinfo.context);
	
	if (type == NULL)
		ErrorExit("�������Ͷ������", tokens);

	tokens.erase(tokens.begin());
	if (tokens.size() > 0 && tokens[0].Value == "*")
	{
		tokens.erase(tokens.begin());
	}
	return type;
}

std::map<std::string,int> IR_EXPR_PRI =
{
	{"*",30},
	{"/",30},
	{"+",20},
	{"-",20},
	{"=",10}
};

llvm::Value* ir_expr(std::vector<AST>& ast_list, IRINFO& irinfo);

struct IR_EXPR
{
	AST op;		//�������ڵ�
	int op_pri; //���������ȼ�
	AST right;
	llvm::Value* right_value=NULL;
};
//���ұ���
VAR_INFO ir_var(std::string name,std::vector<VAR_LIST> var_list,std::vector<TOKEN> tokens)
{
	//�������ϲ��ұ�������
	while (!var_list.empty())
	{
		VAR_LIST vlist = var_list.back();
		var_list.pop_back();

		if (vlist.info.find(name) == vlist.info.end())
			ErrorExit("ERROR: ����������", tokens);
		VAR_INFO vinfo = vlist.info[name];
		if (vinfo.value)
		{
			return vinfo;
		}
	}
}
//��ȡ����ֵ
llvm::Value* ir_var_load(VAR_INFO& var_info,IRINFO& irinfo)
{
	return irinfo.builder->CreateLoad(var_info.type, var_info.value);
}

//���ʽתΪֵ����
void ir_value(IR_EXPR& current, IRINFO& irinfo)
{
	if (current.right_value)
		return;
	if (current.right.type == AST_TYPE::ast_call)
	{
		//��������
		//Ex:	printf("abc");
		std::string fname = current.right.value["name"][0].Value;
		std::vector<llvm::Value*> fargs;
		std::vector<AST> args = current.right.body;
		while (!args.empty())
		{
			std::vector<AST> a1 = { args[0] };
			fargs.push_back(ir_expr(a1, irinfo));
			args.erase(args.begin());
		}
		llvm::Function* function = irinfo.module->getFunction(fname);
		if (function)
		{
			current.right_value = irinfo.builder->CreateCall(function, fargs);
		}
		else
			ErrorExit("δ�ҵ���������", current.right.value["name"]);
	}
	else if (current.right.value.empty()) //û��ֵ�ڵ�ģ������²�ڵ�
		current.right_value = ir_expr(current.right.body, irinfo);
	else if (current.right.value["value"][0].type == TOKEN_TYPE::string)
		current.right_value = irinfo.builder->CreateGlobalStringPtr(current.right.value["value"][0].Value);
	else if (current.right.value["value"][0].type == TOKEN_TYPE::number)
	{
		int v = atoi(current.right.value["value"][0].Value.c_str());
		current.right_value = irinfo.builder->getInt32(v);
	}
	else
	{
		VAR_INFO vinfo = ir_var(current.right.value["value"][0].Value, irinfo.varlist, current.right.value["value"]);
		current.right_value =ir_var_load( vinfo,irinfo);
		if(current.right_value==NULL)
			ErrorExit("δʶ��ı��ʽ����", current.right.value["value"]);
	}
}
//���ʽ����
llvm::Value* ir_expr(std::vector<AST>& ast_list, IRINFO& irinfo)
{
	std::vector<IR_EXPR> expr_list;
	//������ʽ���ȼ�
	IR_EXPR expr;
	expr.op_pri = 0;
	expr.right = ast_list[0];
	expr_list.push_back(expr);
	ast_list.erase(ast_list.begin());
	//����ʽ����
	while (!ast_list.empty())
	{
		expr.op = ast_list[0];
		expr.op_pri= IR_EXPR_PRI[expr.op.value["value"][0].Value];
		ast_list.erase(ast_list.begin());
		expr.right = ast_list[0];
		expr_list.push_back(expr);
		ast_list.erase(ast_list.begin());
	}

	//������ʽ
	llvm::Value* value;
	while (!expr_list.empty())
	{
		//����������ȼ����ʽ
		IR_EXPR* current = &expr_list[0];
		int index = 0;
		for (index = 1; index < expr_list.size(); index++)
		{
			if (expr_list[index].op_pri > current->op_pri ||
				expr_list[index].op_pri == current->op_pri && current->op.value["value"][0].Value == "=")
				current = &expr_list[index];
		}
		//ת��
		ir_value(*current,irinfo);
		//ֻ��һ��ֵ���������Ĵ���
		if (current->op_pri == 0)
		{
			value=current->right_value;
			expr_list.erase(expr_list.begin()+index-1);
			continue;
		}
		
		IR_EXPR* prev = &expr_list[index - 2];
		//�жϸ�ֵ����
		if (current->op.value["value"][0].Value == "=")
		{
			prev->right_value = current->right_value;
			VAR_INFO var_info = irinfo.varlist[irinfo.varlist.size() - 1].info[prev->right.value["value"][0].Value];

			irinfo.builder->CreateStore(prev->right_value, var_info.value);
			value=current->right_value;
			expr_list.erase(expr_list.begin() + index - 1);
			continue;
		}

		ir_value(*prev,irinfo);
		if (current->op.value["value"][0].Value == "+")
		{
			if(prev->right_value->getType()!=current->right_value->getType())
				ErrorExit("���Ͳ�һ��", current->right.value["value"]);
			prev->right_value=irinfo.builder->CreateAdd(prev->right_value, current->right_value);
			expr_list.erase(expr_list.begin() + index - 1);
		}
		else if (current->op.value["value"][0].Value == "-")
		{
			prev->right_value = irinfo.builder->CreateSub(prev->right_value, current->right_value);
			expr_list.erase(expr_list.begin() + index - 1);
		}
		else if (current->op.value["value"][0].Value == "*")
		{
			prev->right_value = irinfo.builder->CreateMul(prev->right_value, current->right_value);
			expr_list.erase(expr_list.begin() + index - 1);
		}
		else if (current->op.value["value"][0].Value == "/")
		{
			prev->right_value = irinfo.builder->CreateSDiv(prev->right_value, current->right_value);
			expr_list.erase(expr_list.begin() + index - 1);
		}
		else
		{
			ErrorExit("δʶ��ı��ʽ����", current->right.value["value"]);
		}
	}
	return value;
}


//�﷨������������IR����
void ir_proc(std::vector<AST>& ast_list,IRINFO& irinfo,bool ismain)
{

	while (!ast_list.empty())
	{
		//�Ǵ��룬ֱ�����
		if (ast_list[0].type==AST_TYPE::ast_noncode)
		{
			llvm::Function* function = irinfo.module->getFunction("printf");
			if (function)
			{
				llvm::Value* args = irinfo.builder->CreateGlobalStringPtr(ast_list[0].value["value"][0].Value);
				irinfo.builder->CreateCall(function, args);
				ast_list.erase(ast_list.begin());
			}
			else
				ErrorExit("δ�ҵ��������壺printf", ast_list[0].value["value"]);
			continue;
		}

		//Function
		//	Ex: int function_name(int arg1)
		//		 |		|              + args
		//       |      + ��������
		//       +  ����ֵ����
		//�ж����ݣ���ǰΪ�ַ�����һ��token��һ���ַ�������һ����(
		if (ast_list[0].type==AST_TYPE::ast_function)
		{
			//args
			llvm::Type* frtype = irtype(ast_list[0].value["rett"], irinfo);
			std::string fname=ast_list[0].value["name"][0].Value;
			std::vector<llvm::Type*> fatype;
			std::vector<std::string> faname;
			bool isVarArg=false;
			std::vector<TOKEN> args = ast_list[0].value["args"];
			while(!args.empty())
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
					fatype.push_back(irtype(args, irinfo));
					if (!args.empty())
					{
						faname.push_back(args[0].Value);
						args.erase(args.begin());
					}
					else
						faname.push_back("");
				}
			llvm::FunctionType* functionType = llvm::FunctionType::get(frtype, fatype, isVarArg);
			llvm::Function* function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage,fname,irinfo.module);
			//arg name
			unsigned i = 0;
			for (auto& a : function->args())
			{
				if(faname[i]!="")
					a.setName(faname[i]);
			}
			if (!ast_list[0].body.empty())
			{
				//���õ�ǰ����������
				VAR_LIST varlist;
				varlist.zone = "function";
				irinfo.varlist.push_back(varlist);

				ir_proc(ast_list[0].body, irinfo, false);

				irinfo.varlist.pop_back();
			}
			ast_list.erase(ast_list.begin());
			continue;
		}

		if (ast_list[0].type ==AST_TYPE::ast_codeblock)
		{
			//���õ�ǰ����������
			VAR_LIST varlist;
			varlist.zone = "function";
			irinfo.varlist.push_back(varlist);

			ir_proc(ast_list[0].body,irinfo,false);
			
			irinfo.varlist.pop_back();

			ast_list.erase(ast_list.begin());
			continue;
		}

		//��������
		//Ex:	int a;
		//		int b=2;
		if (ast_list[0].type==AST_TYPE::ast_var)
		{
			//VAR_LIST var_list = irinfo.varlist.rbegin();
			VAR_INFO var_info;
			var_info.type= irtype(ast_list[0].value["type"], irinfo);
			var_info.value = irinfo.builder->CreateAlloca(var_info.type);
			irinfo.varlist[irinfo.varlist.size()-1].info[ast_list[0].value["name"][0].Value] = var_info;
			ast_list.erase(ast_list.begin());
			continue;
		}

		//������ֵ
		if (ast_list[0].type ==AST_TYPE::ast_expr)
		{
			ir_expr(ast_list[0].body, irinfo);
			////VAR_LIST var_list = irinfo.varlist.back();
			//VAR_INFO var_info = irinfo.varlist[irinfo.varlist.size() - 1].info[ast_list[0].value["name"][0].Value];
			//irinfo.builder->CreateStore(ir_expr(ast_list[0].value["value"],irinfo), var_info.value);
			ast_list.erase(ast_list.begin());
			continue;
		}

		ErrorExit("δʶ���ʶ", ast_list);
		break;
	}
	//return ast_list;
}


//	THE END