
////////////////////////////////////////////////////////////////////////////////
//
// AST_for
//
////////////////////////////////////////////////////////////////////////////////
#include "colang.h"

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
	if (tokens[0].type == TOKEN_TYPE::code && tokens[1].type == TOKEN_TYPE::code && tokens[1].Value != "(")
	{
		new AST_var(tokens);
	}
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

