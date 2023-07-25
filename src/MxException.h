#pragma once

#include <stdexcept>
#include <string>

class antlr_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

class semantic_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};
