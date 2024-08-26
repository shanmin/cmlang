
////////////////////////////////////////////////////////////////////////////////
//
// AST_goto
//
////////////////////////////////////////////////////////////////////////////////
#include "colang.h"

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
		bbstart = llvm::BasicBlock::Create(ir_context, name.Value, func);
		ir_labellist[ir_labellist.size() - 1].info[name.Value] = bbstart;
	}
	else
	{
		bbstart = labellist.info[name.Value];
	}
	ir_builder->CreateBr(bbstart);

	//goto����Ҫ�³���һ��basic block���������֡�Terminator found in the middle of a basic block!��������Ϣ
	{
		llvm::Function* func = ir_builder->GetInsertBlock()->getParent();
		llvm::BasicBlock* bbstart1 = llvm::BasicBlock::Create(ir_context, "", func);
		ir_builder->SetInsertPoint(bbstart1);
	}

	return nullptr;
}
