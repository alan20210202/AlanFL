/*
 * This is the test file for AlanFL!
 * Put tests here!
 */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

#include <mpirxx.h>

#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include <memory>

using namespace std;
using namespace alanfl;

void test_vm() {
	vm v;
	wifstream fin;
	fin.open(LR"(D:\C++\AlanFL\Tests\test_phi.txt)");
	auto lex = make_shared<lexer>(fin);
	auto par = make_shared<parser>(lex);
	const auto mod = par->mod();
	
	if (par->has_error()) {
		wcout << "error in compilation, execution aborted" << endl;
		par->dump_error();
	} else
		v.exec(mod);
	fin.close();
}

int main() {
	test_vm();
	system("pause");
	return 0;
}
