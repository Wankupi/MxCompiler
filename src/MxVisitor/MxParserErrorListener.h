#pragma once

#include "MxParser.h"

#include "MxException.h"

class MxParserErrorListener : public antlr4::BaseErrorListener {
	void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line, size_t charPositionInLine,
					 const std::string &msg, std::exception_ptr e) override {
		throw antlr_error{"syntaxError: " + msg};
	}
	// do not throw other three errors,
	// they're just warnings.
};
