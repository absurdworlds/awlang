/*
 * Copyright (C) 2015  absurdworlds
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <daedalus/parser/Parser.h>

namespace deadalus {

Declaration* Parser::parseDeclaration()
{
	switch(token.getType()) {
	case kw_var:
		return parseVariableDeclaration();
	case kw_const:
		return parseConstantDeclaration();
	case kw_func:
		return parseFunctionDeclaration();
	case kw_class:
		return parseClassDeclaration();
	case kw_prototype:
		return parsePrototypeDeclaration();
	case kw_instance:
		return parseInstanceDeclaration();
	}
}

Declaration* parseVariableDeclaration()
{
	Token tok = lex->getNextToken();

	// Read variable type: var [type] id
	if (!tok.isIdentifier()) // TODO: build-in types
		return 0;

	tok = lex->getNextToken();

	// Read variable name: var type [id]
	if (!tok.isIdentifier())
		return 0;
	
	// TODO: symbol table lookup
	std::string name = tok.getData();

	// Variable* var = new Variable(/*symbol*/, /*thingy*/)// TODO ;

	return new VariableDeclaration(name);
}

Declaration* parseConstantDeclaration()
{
	Token tok = lex->getNextToken();

	// Read variable type: const [type] id
	if (!(tok.isIdentifier() || isTypeName(tok))
		return 0;

	tok = lex->getNextToken();

	// Read variable name: const type [id]
	if (!tok.isIdentifier())
		return 0;

	// TODO: symbol table lookup
	std::string name = tok.getData();

	if (tok.getType() != tok_equals)
		return 0;

	Expression* initializer = parseExpression();

	// Constant* constant = new Constant(/*symbol*/, /*thingy*/); // TODO 

	return new ConstantDeclaration(name, initializer);
}

Declaration* parseFunctionDeclaration()
{
	Token tok = lex->getNextToken();

	// Return type: func [type] id (args*)
	if (!(tok.isIdentifier() || isTypeName(tok))
		return 0;

	tok = lex->getNextToken();
	std::string ret = tok.getData();

	// Function name: func type [id] (args*)
	if (!tok.isIdentifier())
		return 0;

	// TODO: symbol table lookup
	std::string name = tok.getData();

	if (lex->getNextToken() != tok_lparen)
		return 0;

	tok = lex->getNextToken();

	std::vector<VariableDeclaration*> args;
	while (tok.getType == tok_kw_var) {
		VariableDeclaration* arg = parseVariableDeclaration();
		args.push_back(arg);

		tok = lex->getNextToken();
	}

	if (lex->getNextToken() != tok_rparen)
		return 0;

	tok = lex->getNextToken();

	return new FunctionDeclaration(name, ret, args);
}
} // namespace daedalus
