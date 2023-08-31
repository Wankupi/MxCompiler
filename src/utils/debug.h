#pragma once
#include <iostream>

#warning "debug.h included"

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define RESET "\033[0m"

template<typename... T>
void debug(T... args) {
	((std::cout << args), ...);
	std::cout.flush();
}
