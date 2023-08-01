#include "antlr4-runtime.h"

#include "MxLexer.h"
#include "MxParser.h"
#include "MxVisitor/MxParserErrorListener.h"

#include "AST/AST.h"
#include "MxVisitor/AstBuilder.h"

#include "Semantic/ClassCollector.h"
#include "Semantic/FunctionCollector.h"
#include "Semantic/Scope.h"
#include "Semantic/SemanticChecker.h"

#include "IR/Builder.h"

#include <fstream>
#include <iostream>

AstNode *getAST(std::istream &in);

int main(int argc, char *argv[]) {
	try {
		AST ast(nullptr);
		if (argc > 1) {
			std::ifstream in(argv[1]);
			ast.root = getAST(in);
		}
		else
			ast.root = getAST(std::cin);

		GlobalScope globalScope;

		ClassCollector classCollector(&globalScope);
		classCollector.init_builtin_classes();
		classCollector.visit(ast.root);

		FunctionCollector functionCollector(&globalScope);
		functionCollector.init_builtin_functions();
		functionCollector.visit(ast.root);

		SemanticChecker semanticChecker(&globalScope);
		semanticChecker.visit(ast.root);

		IRBuilder irBuilder;
		irBuilder.visit(ast.root);

		std::ofstream out("test.ll", std::ios::out);
		if (out.fail())
			throw std::runtime_error("Cannot open file test.ll");

		auto ir = irBuilder.getIR();
		ir->print(out);

		delete ir;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	system("llc --march=riscv32 test.ll");
	return 0;
}

AstNode *getAST(std::istream &in) {
	antlr4::ANTLRInputStream input(in);
	MxParserErrorListener errorListener;

	MxLexer lexer(&input);
	lexer.removeErrorListeners();
	lexer.addErrorListener(&errorListener);

	antlr4::CommonTokenStream tokens(&lexer);
	MxParser parser(&tokens);
	parser.removeErrorListeners();
	parser.addErrorListener(&errorListener);

	auto tree = parser.file();

	AstBuilder builder;
	auto visit_result = builder.visit(tree);

	if (!visit_result.has_value())
		throw std::runtime_error("AST build failed");
	return std::any_cast<AstNode *>(visit_result);
}
