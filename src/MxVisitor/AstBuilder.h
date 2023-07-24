#pragma once

#include "MxParserBaseVisitor.h"

class AstBuilder : public MxParserBaseVisitor {
public:
	/// @return AstNode*
	std::any visitFile(MxParser::FileContext *ctx) override;
	/// @return AstNode*
	std::any visitDefineClass(MxParser::DefineClassContext *ctx) override;
	/// @return AstFunctionNode*
	std::any visitDefineFunction(MxParser::DefineFunctionContext *ctx) override;
	/// @return AstConstructFuncNode*
	std::any visitConstructFunction(MxParser::ConstructFunctionContext *ctx) override;
	/// @return std::vector<AstFuncParamNode*>
	std::any visitFunctionParameterList(MxParser::FunctionParameterListContext *ctx) override;
	/// @return std::vector<AstStmtNode*>
	std::any visitSuite(MxParser::SuiteContext *ctx) override;
	/// @return AstStmtNode*
	std::any visitBlock(MxParser::BlockContext *ctx) override;
	/// @return AstStmtNode*
	std::any visitExprStmt(MxParser::ExprStmtContext *ctx) override;
	/// @return AstStmtNode*
	std::any visitIfStmt(MxParser::IfStmtContext *ctx) override;
	/// @return AstStmtNode*
	std::any visitWhileStmt(MxParser::WhileStmtContext *ctx) override;
	/// @return AstStmtNode*
	std::any visitForStmt(MxParser::ForStmtContext *ctx) override;
	/// @return AstStmtNode*
	std::any visitFlowStmt(MxParser::FlowStmtContext *ctx) override;
	/// @return AstStmtNode*
	std::any visitDefineVariableStmt(MxParser::DefineVariableStmtContext *ctx) override;
	/// @return AstExprNode*
	std::any visitBinaryExpr(MxParser::BinaryExprContext *ctx) override;
	/// @return AstExprNode*
	std::any visitAtomExpr(MxParser::AtomExprContext *ctx) override;
	/// @return AstExprNode*
	std::any visitTernaryExpr(MxParser::TernaryExprContext *ctx) override;
	/// @return AstExprNode*
	std::any visitAssignExpr(MxParser::AssignExprContext *ctx) override;
	/// @return AstExprNode*
	std::any visitFuncCall(MxParser::FuncCallContext *ctx) override;
	/// @return AstExprNode*
	std::any visitArrayAccess(MxParser::ArrayAccessContext *ctx) override;
	/// @return AstExprNode*
	std::any visitMemberAccess(MxParser::MemberAccessContext *ctx) override;
	/// @return AstExprNode*
	std::any visitNewExpr(MxParser::NewExprContext *ctx) override;
	/// @return AstExprNode*
	std::any visitLiterExpr(MxParser::LiterExprContext *ctx) override;
	/// @return AstTypeNode*
	std::any visitTypename(MxParser::TypenameContext *ctx) override;
	/// @return AstExprNode*
	std::any visitLeftSingleExpr(MxParser::LeftSingleExprContext *ctx) override;
	/// @return AstExprNode*
	std::any visitRightSingleExpr(MxParser::RightSingleExprContext *ctx) override;
};
