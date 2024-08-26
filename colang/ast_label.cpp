////////////////////////////////////////////////////////////////////////////////
//
// AST_label
//
////////////////////////////////////////////////////////////////////////////////
#include "colang.h"

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
