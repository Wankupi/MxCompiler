#include "AstBuilder.h"
#include "AST/AST.h"

std::any AstBuilder::visitFile(MxParser::FileContext *ctx) {
	auto node = new AstFileNode{};
	auto children = ctx->children;
	for (auto c: children) {
		auto visRes = visit(c);
		if (!visRes.has_value())
			continue;// ignore TerminalNode

		if (auto Func = std::any_cast<AstFunctionNode *>(&visRes); Func)
			node->children.push_back(*Func);
		else if (auto Class = std::any_cast<AstClassNode *>(&visRes); Class)
			node->children.push_back(*Class);
		else if (auto Var = std::any_cast<AstStmtNode *>(&visRes); Var)
			node->children.push_back(*Var);
		else
			throw std::runtime_error(__FUNCTION__);
	}
	return static_cast<AstNode *>(node);
}

std::any AstBuilder::visitBinaryExpr(MxParser::BinaryExprContext *ctx) {
	ctx->op->getText();
	auto l = visit(ctx->lhs), r = visit(ctx->rhs);
	auto *left = std::any_cast<AstExprNode *>(l), *right = std::any_cast<AstExprNode *>(r);
	auto node = new AstBinaryExprNode{ctx->op->getText(), left, right};
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitDefineClass(MxParser::DefineClassContext *ctx) {
	auto node = new AstClassNode{};
	node->name = ctx->Identifier()->getText();
	for (auto var: ctx->defineVariableStmt()) {
		auto visRes = visit(var);
		if (!visRes.has_value()) throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstStmtNode *>(visRes);
		node->variables.push_back(dynamic_cast<AstVarStmtNode *>(val));
	}
	for (auto con: ctx->constructFunction()) {
		auto visRes = visit(con);
		if (!visRes.has_value()) throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstConstructFuncNode *>(visRes);
		node->constructors.push_back(val);
	}
	for (auto func: ctx->defineFunction()) {
		auto visRes = visit(func);
		if (!visRes.has_value()) throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstFunctionNode *>(visRes);
		node->functions.push_back(val);
	}
	return node;
}

std::any AstBuilder::visitDefineFunction(MxParser::DefineFunctionContext *ctx) {
	auto node = new AstFunctionNode{};
	node->name = ctx->Identifier()->getText();
	node->returnType = std::any_cast<AstTypeNode *>(visit(ctx->typename_()));
	if (auto list = ctx->functionParameterList(); list) {
		auto params = visit(list);
		node->params = std::move(*std::any_cast<std::vector<AstFuncParamNode *>>(&params));
	}
	node->body = std::any_cast<AstStmtNode *>(visit(ctx->block()));
	return node;
}

std::any AstBuilder::visitFunctionParameterList(MxParser::FunctionParameterListContext *ctx) {
	std::vector<AstFuncParamNode *> params;
	for (auto param: ctx->defineVariable()) {
		auto node = new AstFuncParamNode{};
		node->name = param->Identifier()->getText();
		node->type = std::any_cast<AstTypeNode *>(visit(param->typename_()));
		params.push_back(node);
	}
	return params;
}

std::any AstBuilder::visitBlock(MxParser::BlockContext *ctx) {
	auto node = new AstBlockStmtNode{};
	for (auto stmt: ctx->stmt()) {
		auto visRes = visit(stmt);
		if (!visRes.has_value())
			continue;// empty stmt should be ignored
		auto child = std::any_cast<AstStmtNode *>(visRes);
		node->stmts.push_back(child);
	}
	return static_cast<AstStmtNode *>(node);
}

std::any AstBuilder::visitAtomExpr(MxParser::AtomExprContext *ctx) {
	auto node = new AstAtomExprNode{};
	node->name = ctx->getText();
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitDefineVariableStmt(MxParser::DefineVariableStmtContext *ctx) {
	auto node = new AstVarStmtNode{};
	node->type = std::any_cast<AstTypeNode *>(visit(ctx->typename_()));
	auto list = ctx->variableAssign();
	node->vars.reserve(list.size());
	for (auto var: list) {
		auto name = var->Identifier()->getText();
		node->vars.emplace_back(name, nullptr);
		if (auto expr = var->expression(); expr) {
			auto visRes = visit(expr);
			if (!visRes.has_value())
				throw std::runtime_error(__FUNCTION__);
			auto val = std::any_cast<AstExprNode *>(visRes);
			node->vars.back().second = val;
		}
	}
	return static_cast<AstStmtNode *>(node);
}

std::any AstBuilder::visitExprStmt(MxParser::ExprStmtContext *ctx) {
	if (ctx->exprList() == nullptr)
		return {};
	auto node = new AstExprStmtNode{};
	auto exprs = ctx->exprList()->expression();
	node->expr.reserve(exprs.size());
	for (auto expr: exprs) {
		auto visRes = visit(expr);
		if (!visRes.has_value())
			throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstExprNode *>(visRes);
		node->expr.emplace_back(val);
	}
	return static_cast<AstStmtNode *>(node);
}

std::any AstBuilder::visitAssignExpr(MxParser::AssignExprContext *ctx) {
	auto node = new AstAssignExprNode{};
	auto exprs = ctx->expression();
	node->lhs = std::any_cast<AstExprNode *>(visit(exprs[0]));
	node->rhs = std::any_cast<AstExprNode *>(visit(exprs[1]));
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitLiterExpr(MxParser::LiterExprContext *ctx) {
	auto node = new AstLiterExprNode{};
	node->value = ctx->getText();
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitFlowStmt(MxParser::FlowStmtContext *ctx) {
	AstStmtNode *node = nullptr;
	if (ctx->Continue())
		node = new AstContinueStmtNode{};
	else if (ctx->Break())
		node = new AstBreakStmtNode{};
	else {
		auto retNode = new AstReturnStmtNode{};
		if (auto expr = ctx->expression(); expr) {
			auto visRes = visit(expr);
			if (!visRes.has_value()) return {};
			auto val = std::any_cast<AstExprNode *>(visRes);
			retNode->expr = val;
		}
		node = retNode;
	}
	return node;
}

std::any AstBuilder::visitFuncCall(MxParser::FuncCallContext *ctx) {
	auto node = new AstFuncCallExprNode{};
	auto f = visit(ctx->expression());
	node->func = std::any_cast<AstExprNode *>(f);
	if (auto list = ctx->exprList(); list) {
		auto exprs = list->expression();
		node->args.reserve(exprs.size());
		for (auto expr: exprs) {
			auto visRes = visit(expr);
			if (!visRes.has_value())
				throw std::runtime_error(__FUNCTION__);
			auto val = std::any_cast<AstExprNode *>(visRes);
			node->args.emplace_back(val);
		}
	}
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitNewExpr(MxParser::NewExprContext *ctx) {
	auto node = new AstNewExprNode{};
	node->type = std::any_cast<AstTypeNode *>(visit(ctx->typename_()));
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitTypename(MxParser::TypenameContext *ctx) {
	if (!ctx->bad.empty())
		throw std::runtime_error(__FUNCTION__ + std::string(": bad type name: ") + ctx->getText());
	auto node = new AstTypeNode{};
	if (auto id = ctx->Identifier(); id)
		node->name = id->getText();
	else
		node->name = ctx->BasicType()->getText();
	node->dimension = ctx->BracketLeft().size();
	auto list = ctx->good;
	node->arraySize.reserve(list.size());
	for (auto expr: list) {
		auto visRes = visit(expr);
		if (!visRes.has_value()) throw std::runtime_error(__FUNCTION__ + std::string(": bad array size"));
		auto val = std::any_cast<AstExprNode *>(visRes);
		node->arraySize.emplace_back(val);
	}
	return node;
}

std::any AstBuilder::visitArrayAccess(MxParser::ArrayAccessContext *ctx) {
	auto node = new AstArrayAccessExprNode{};
	auto exprs = ctx->expression();
	node->array = std::any_cast<AstExprNode *>(visit(exprs[0]));
	node->index = std::any_cast<AstExprNode *>(visit(exprs[1]));
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitMemberAccess(MxParser::MemberAccessContext *ctx) {
	auto node = new AstMemberAccessExprNode{};
	node->object = std::any_cast<AstExprNode *>(visit(ctx->expression()));
	node->member = ctx->Identifier()->getText();
	return static_cast<AstExprNode *>(node);
}


std::any AstBuilder::visitForStmt(MxParser::ForStmtContext *ctx) {
	auto node = new AstForStmtNode{};
	if (auto init = ctx->exprStmt(); init) {
		auto visRes = visit(init);
		if (!visRes.has_value())
			throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstStmtNode *>(visRes);
		node->init = val;
	}
	if (auto init = ctx->defineVariableStmt(); init) {
		auto visRes = visit(init);
		if (!visRes.has_value())
			throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstStmtNode *>(visRes);
		node->init = val;
	}
	if (auto cond = ctx->condition; cond) {
		auto visRes = visit(cond);
		if (!visRes.has_value())
			throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstExprNode *>(visRes);
		node->cond = val;
	}
	if (auto step = ctx->step; step) {
		auto visRes = visit(step);
		if (!visRes.has_value())
			throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstExprNode *>(visRes);
		node->step = val;
	}
	auto visRes = visit(ctx->suite());
	if (!visRes.has_value())
		throw std::runtime_error(__FUNCTION__);
	node->body = std::move(*std::any_cast<std::vector<AstStmtNode *>>(&visRes));
	return static_cast<AstStmtNode *>(node);
}

std::any AstBuilder::visitSuite(MxParser::SuiteContext *ctx) {
	std::vector<AstStmtNode *> rets;
	std::vector<MxParser::StmtContext *> stmts;
	if (auto block = ctx->block(); block)
		stmts = block->stmt();
	else
		stmts = {ctx->stmt()};
	for (auto stmt: stmts) {
		auto visRes = visit(stmt);
		if (!visRes.has_value())
			continue;// empty stmt should be ignored
		auto child = std::any_cast<AstStmtNode *>(visRes);
		rets.push_back(child);
	}
	return rets;
}

std::any AstBuilder::visitLeftSingleExpr(MxParser::LeftSingleExprContext *ctx) {
	auto node = new AstSingleExprNode{};
	node->op = ctx->op->getText();
	node->right = false;
	node->expr = std::any_cast<AstExprNode *>(visit(ctx->expression()));
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitRightSingleExpr(MxParser::RightSingleExprContext *ctx) {
	auto node = new AstSingleExprNode{};
	node->op = ctx->op->getText();
	node->right = true;
	node->expr = std::any_cast<AstExprNode *>(visit(ctx->expression()));
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitWhileStmt(MxParser::WhileStmtContext *ctx) {
	auto node = new AstWhileStmtNode{};
	auto visRes = visit(ctx->expression());
	if (!visRes.has_value())
		throw std::runtime_error(__FUNCTION__);
	node->cond = std::any_cast<AstExprNode *>(visRes);
	visRes = visit(ctx->suite());
	if (!visRes.has_value())
		throw std::runtime_error(__FUNCTION__);
	node->body = std::move(*std::any_cast<std::vector<AstStmtNode *>>(&visRes));
	return static_cast<AstStmtNode *>(node);
}

std::any AstBuilder::visitConstructFunction(MxParser::ConstructFunctionContext *ctx) {
	auto node = new AstConstructFuncNode{};
	node->name = ctx->Identifier()->getText();
	if (auto list = ctx->functionParameterList(); list) {
		auto params = visit(list);
		node->params = std::move(*std::any_cast<std::vector<AstFuncParamNode *>>(&params));
	}
	node->body = std::any_cast<AstStmtNode *>(visit(ctx->block()));
	return node;
}

std::any AstBuilder::visitIfStmt(MxParser::IfStmtContext *ctx) {
	auto node = new AstIfStmtNode{};
	auto exprs = ctx->expression();
	auto bodies = ctx->suite();
	for (size_t i = 0; i < exprs.size(); ++i) {
		auto visRes = visit(exprs[i]);
		if (!visRes.has_value())
			throw std::runtime_error(__FUNCTION__);
		auto val = std::any_cast<AstExprNode *>(visRes);
		visRes = visit(bodies[i]);
		if (!visRes.has_value())
			throw std::runtime_error(__FUNCTION__);
		node->ifStmts.emplace_back(val, std::move(*std::any_cast<std::vector<AstStmtNode *>>(&visRes)));
	}
	if (exprs.size() < bodies.size()) {
		auto visRes = visit(bodies.back());
		if (!visRes.has_value())
			throw std::runtime_error(__FUNCTION__);
		node->elseStmt = std::move(*std::any_cast<std::vector<AstStmtNode *>>(&visRes));
	}
	return static_cast<AstStmtNode *>(node);
}

std::any AstBuilder::visitTernaryExpr(MxParser::TernaryExprContext *ctx) {
	auto node = new AstTernaryExprNode{};
	auto exprs = ctx->expression();
	node->cond = std::any_cast<AstExprNode *>(visit(exprs[0]));
	node->trueExpr = std::any_cast<AstExprNode *>(visit(exprs[1]));
	node->falseExpr = std::any_cast<AstExprNode *>(visit(exprs[2]));
	return static_cast<AstExprNode *>(node);
}

std::any AstBuilder::visitWrapExpr(MxParser::WrapExprContext *ctx) {
	return visit(ctx->expression());
}
