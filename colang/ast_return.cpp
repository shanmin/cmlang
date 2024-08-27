////////////////////////////////////////////////////////////////////////////////
//
// AST_return
//
////////////////////////////////////////////////////////////////////////////////
#include "colang.h"

AST_return::AST_return(std::vector<TOKEN>& tokens)
{
	this->token = tokens[0];
	tokens.erase(tokens.begin());
	if (tokens[0].type != TOKEN_TYPE::string && tokens[0].Value != ";")
		value = ast_parse_expr(tokens);
}


void AST_return::show(std::string pre)
{
	std::cout << pre << "#TYPE:return" << std::endl;
	if (value)
	{
		std::cout << pre << "value:";
		value->show(pre + "      ");
	}
	std::cout << std::endl;
}


llvm::Value* AST_return::codegen()
{
	if (value)
		return ir_builder->CreateRet(value->codegen());
	else
		return ir_builder->CreateRetVoid();

	////������룺
	////	int ff()
	////	{
	////		if(1)
	////			return 1;
	////		return 0;
	////	}
	////	�����������ɵķ�ʽ�����ɵ�bc�����£�
	////	define i32 @ff() {
	////		br i1 true, label %then, label %else
	////	then:                                             ; preds = %0
	////		ret i32 1
	////		br label %endd
	////	else:                                             ; preds = %0
	////		br label %endd
	////	endd:                                             ; preds = %else, %then
	////		ret i32 0
	////	}
	////	��δ��������LVMM��֤����
	////		Terminator found in the middle of a basic block!
	////		label %then
	////	��Ϊδ�ҵ��򵥵Ľ����ʽ���������return�����ɴ����Ϊһ������ֻ��һ����������returnʱ����һ�����ر�����Ȼ��goto�����ش��롣���ս�����ƣ�
	////	define i32 @ff() {
	////		br i1 true, label %then, label %else
	////	then:                                             ; preds = %0
	////		%RETURN_VAL=1;
	////		br label %RETURN
	////	else:                                             ; preds = %0
	////		br label %endd
	////	endd:                                             ; preds = %else, %then
	////		RETURN_VAL=0;
	////		br label %RETURN
	////	RETURN:
	////		ret i32 %RETURN_VAL
	////	}
	//llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
	////����RETURN��ǩ
	////llvm::BasicBlock* bb_return=NULL;
	//for (llvm::BasicBlock &bb : *func)
	//	if (bb.getName() == "__co__RETURN")
	//	{
	//		//bb_return = &bb;
	//		if (value) //�ж��Ƿ���Ҫ����ֵ
	//		{
	//			VAR_INFO vinfo = ir_var("__co__RETURN_VAL", ir_varlist, this->token);
	//			//vinfo.value = value->codegen();
	//			ir_builder->CreateStore(value->codegen(), vinfo.value);
	//		}
	//		ir_builder->CreateBr(&bb);
	//		return NULL;
	//	}

	//ErrorExit("ERROR: δ�ҵ� RETURN ��ǩ", this->token);
			

	//llvm::BasicBlock* bbexpr = llvm::BasicBlock::Create(ir_context, "return over", func);
	//ir_builder->SetInsertPoint(bbexpr);

	return NULL;
}

//	THE END