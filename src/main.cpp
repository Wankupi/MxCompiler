#include "AST/AST.h"
#include "MxLexer.h"
#include "MxParser.h"
#include "MxVisitor/AstBuilder.h"
#include "MxVisitor/MxParserErrorListener.h"
#include "Scope.h"
#include "antlr4-runtime.h"
#include <iostream>

int main() {
	antlr4::ANTLRInputStream input(std::cin);
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
		return 1;
	auto ast = std::any_cast<AstNode *>(visit_result);
	std::cout << ast << std::endl;
	ast->print();

	return 0;
}
