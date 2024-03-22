/*
 * Copyright (C) 2015-2022 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <aw/script/lexer/lexer.h>

#include <aw/algorithm/in.h>

#include "char_type.h"

namespace aw::script {
lexer::lexer(source_buffer* buffer)
	: buf(buffer)
	, cur(buf->begin())
	, end(buf->end())
{
}

lexer::~lexer()
{
	// for export
}

token lexer::next()
{
	token tok;
	lex(tok);
	return tok;
}

auto lexer::save_state() -> save_point
{
	return { buf, cur };
}

void lexer::restore_state(save_point sp)
{
	if (sp.buf != buf) {
		assert(!"Lexer save point is too old");
		return;
	}

	cur = sp.pos;
}

location lexer::make_location(const char* pos) const
{
	const auto offset = pos - buf->begin();

	// TODO: file_id from source_buffer
	return location(offset, buf->file_id());
}

char lexer::prev() const
{
	assert(cur > std::begin(*buf));
	return *(cur - 1);
}

char lexer::peek() const
{
	return (cur < end) ? *(cur + 1) : '\0';
}

bool lexer::match(const char c)
{
	if (cur == end)
		return false;
	auto next = (cur + 1);
	if (c == *next) {
		cur = next;
		return true;
	}
	return false;
}

bool lexer::lex_eof(token& token) const
{
	token = {
		.kind = token_kind::eof,
		.loc = make_location(cur)
	};
	return true;
}

bool lexer::lex_numeric_constant(token& token)
{
	char const* start = cur;

	// TODO: separate integer and real literals?
	// TODO: support short form: .05
	// TODO: "non-greedy" algorithm
	cur = std::find_if_not(cur, end, isalnum);

	if (*cur == '.') {
		cur = std::find_if_not(cur+1, end, isalnum);

		if (in(*cur, '-', '+') && in(prev(), 'e', 'E'))
			cur = std::find_if_not(cur, end, isalnum);
	}

	token = {
		.kind = token_kind::numeric_constant,
		.data = string_view(start, cur),
		.loc  = make_location(start)
	};

	return true;
}

bool lexer::lex_string_literal(token& token)
{
	// TODO: non-multiline strings
	char const* start = cur;

	++cur; // consume '"'
	while (*cur != '"') {
		cur = std::find(cur, end, '"');
		if (cur == end)
			break;
		if (prev() == '\\')
			++cur;
	}

	const bool complete = cur < end;
	if (complete)
		++cur; // consume '"'

	token = {
		.kind = token_kind::string_literal,
		.data = string_view(start, cur),
		.loc  = make_location(start),
		.incomplete = !complete
	};

	return true;
}

bool lexer::lex_identifier(token& token)
{
	// TODO: support unicode
	char const* start = cur;
	cur = std::find_if_not(cur, end, is_name_char);

	std::string_view id(start, cur);

	// TODO: right now I don't lex the keywords -
	// I want to try to make them context-dependent
	// as much as possible
#if 0
	// Check if token is a reserved keyword
	auto kind = kwmap.get(id);

	if (!kind)
		kind = tok_identifier;
#endif

	token = {
		.kind = token_kind::identifier,
		.data = id,
		.loc = make_location(start)
	};

	return true;
}

bool lexer::lex_operator(token& token)
{
	char const* tok_start = cur;

	token.loc = make_location(tok_start);

	switch (*cur) {
	case '^':
		token.kind = match('=') ?
			token_kind::caret_equal:
			token_kind::caret;
		break;
	case '~':
		token.kind = token_kind::tilde;
		break;
	case ',':
		token.kind = token_kind::comma;
		break;
	case '.':
		if (match('.')) {
			token.kind = (match('.')) ?
				token_kind::ellipsis :
				token_kind::dot_dot;
		} else {
			token.kind = token_kind::dot;
		}
		break;
	case ';':
		token.kind = token_kind::semicolon;
		break;
	case ':':
		if (match(':')) {
			token.kind = token_kind::double_colon;
		} else {
			token.kind = token_kind::colon;
		}
		break;
	case '{':
		token.kind = token_kind::l_brace;
		break;
	case '}':
		token.kind = token_kind::r_brace;
		break;
	case '(':
		token.kind = token_kind::l_paren;
		break;
	case ')':
		token.kind = token_kind::r_paren;
		break;
	case '[':
		token.kind = token_kind::l_square;
		break;
	case ']':
		token.kind = token_kind::r_square;
		break;
	case '=':
		if (peek() == '=') {
			token.kind = token_kind::equal_equal;
			++cur;
		} else {
			token.kind = token_kind::equal;
		}
		break;
	case '%':
		if (peek() == '=') {
			token.kind = token_kind::percent_equal;
			++cur;
		} else {
			token.kind = token_kind::percent;
		}
		break;
	case '/':
		if (peek() == '=') {
			token.kind = token_kind::slash_equal;
			++cur;
		} else {
			token.kind = token_kind::slash;
		}
		break;
	case '<':
		if (match('<')) {
			token.kind = match('=') ?
				token_kind::less_less_equal :
				token_kind::less_less;
		} else if (peek() == '=') {
			token.kind = token_kind::less_equal;
			++cur;
		} else {
			token.kind = token_kind::less;
		}
		break;
	case '>':
		if (match('>')) {
			token.kind = match('=') ?
				token_kind::greater_greater_equal :
				token_kind::greater_greater;
		} else if (peek() == '=') {
			token.kind = token_kind::greater_equal;
			++cur;
		} else {
			token.kind = token_kind::greater;
		}
		break;
	case '+':
		if (peek() == '+') {
			token.kind = token_kind::plus_plus;
			++cur;
		} else if (peek() == '=') {
			token.kind = token_kind::plus_equal;
			++cur;
		} else {
			token.kind = token_kind::plus;
		}
		break;
	case '-':
		if (peek() == '-') {
			token.kind = token_kind::minus_minus;
			++cur;
		} else if (peek() == '=') {
			token.kind = token_kind::minus_equal;
			++cur;
		} else {
			token.kind = token_kind::minus;
		}
		break;
	case '*':
		if (peek() == '=') {
			token.kind = token_kind::ast_equal;
			++cur;
		} else {
			token.kind = token_kind::ast;
		}
		break;
	case '!':
		if (peek() == '=') {
			token.kind = token_kind::bang_equal;
			++cur;
		} else {
			token.kind = token_kind::bang;
		}
		break;
	case '&':
		if (match('&')) {
			token.kind = token_kind::amp_amp;
		} else {
			token.kind = (match('=')) ?
				token_kind::amp_equal :
				token_kind::amp;
		}
		break;
	case '|':
		if (match('|')) {
				token.kind = token_kind::pipe_pipe;
		} else {
			token.kind = (match('=')) ?
				token_kind::pipe_equal :
				token_kind::pipe;
		}
		break;
	}

	++cur;
	token.data = string_view(tok_start, cur);
	token.loc  = make_location(tok_start);

	return true;
}


bool lexer::lex_line_comment(token& token)
{
	// TODO: maybe coalesce multiple comments into one?
	const auto tok_start = cur;

	cur = std::find(cur, end, '\n');

	token = {
		.kind = token_kind::line_comment,
		.data = string_view(tok_start, cur),
		.loc = make_location(tok_start)
	};

	return true;
}

bool lexer::lex_block_comment(token& token)
{
	const auto tok_start = cur;

	// Micro-optimization:
	// We already know that first two characters are /*
	// Third character:
	// - If it is '/', then it needs to be skipped or ‘/*/’
	// would be handled as the end of the commend which is incorrent
	// - If it is '*' followed by '/' then it is going to be handled
	// by the `prev()` in the loop below
	// - If it is anything else, then it doesn't matter and
	// can be skipped in any case.
	constexpr ptrdiff_t skip_count = 3;
	cur += std::min(skip_count, end - cur);

	while (true) {
		cur = std::find(cur, end, '/');

		if (cur == end)
			break;

		if (prev() == '*')
			break;
		++cur;
	}

	const bool complete = cur < end;
	if (complete)
		++cur;

	token = {
		.kind = token_kind::block_comment,
		.data = string_view(tok_start, cur),
		.loc  = make_location(tok_start),
		.incomplete = !complete
	};

	return true;
}

bool lexer::lex_comment(token& token)
{
	char p = peek();
	if (p == '*')
		return lex_block_comment(token);

	if (p == '/')
		return lex_line_comment(token);

	// Not a comment - we're done.
	return false;
}

bool lexer::lex_illegal_token(token& token)
{
	char const* begin = cur;
	cur = std::find_if(begin+1, end, is_separator);

	token = {
		.kind = token_kind::illegal,
		.data = string_view(begin, cur),
		.loc = make_location(begin)
	};

	return true;
}

bool lexer::lex(token& tok)
{
	cur = std::find_if_not(cur, end, isspace);
	if (cur == end)
		return lex_eof(tok);

	switch (*cur) {
	case 0:
		return lex_eof(tok);

	/* Illegal token */
	default:
		// TODO: handle unicode
		return lex_illegal_token(tok);

	/* Numeric constants */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return lex_numeric_constant(tok);

	/* String literal */
	case '"':
		return lex_string_literal(tok);

	/* Identifier */
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
	case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
	case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
	case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
	case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
	case 'v': case 'w': case 'x': case 'y': case 'z':
	case '_':
		// TODO: string literals with a prefix, e.g. f""
		return lex_identifier(tok);

	/* Operators */
	case ',': case '.': case ';': case ':': case '<': case '>':
	case '{': case '}': case '[': case ']': case '(': case ')':
	case '%': case '*': case '+': case '-': case '=':
	case '~': case '!':
	case '&': case '|': case '^':
		return lex_operator(tok);

	case '/':
		// Look for comments first
		if (lex_comment(tok))
			return true;
		// This is less optimized than the previous version
		// but I think this looks nicer and more uniform
		return lex_operator(tok);
	}

	return false;
}

} // namespace aw::script
