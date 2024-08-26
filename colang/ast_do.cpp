////////////////////////////////////////////////////////////////////////////////
//
// AST_do
//
////////////////////////////////////////////////////////////////////////////////
#include "colang.h"

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