/*!
 * This file contains list of all hrscript tokens.
 * In is used to generate enumerations, case-switches
 * and other things.
 */

#ifndef TOKEN
#define TOKEN(x)
#endif

#ifndef PUNCT
#define PUNCT(x, y)
#endif

#ifndef KEYWORD
#define KEYWORD(x)
#endif

TOKEN(none)
TOKEN(illegal)
TOKEN(eof)
TOKEN(identifier)
TOKEN(number)
TOKEN(string_literal)
TOKEN(line_comment)
TOKEN(block_comment)

PUNCT(comma,                 ",")
PUNCT(dot,                   ".")
PUNCT(dot_dot,               "..")
PUNCT(ellipsis,              "...")
PUNCT(semicolon,             ";")
PUNCT(colon,                 ":")
PUNCT(double_colon,          "::")
PUNCT(l_brace,               "{")
PUNCT(r_brace,               "}")
PUNCT(l_square,              "[")
PUNCT(r_square,              "]")
PUNCT(l_paren,               "(")
PUNCT(r_paren,               ")")
PUNCT(amp,                   "&")
PUNCT(amp_equal,             "&=")
PUNCT(amp_amp,               "&&")
PUNCT(pipe,                  "|")
PUNCT(pipe_equal,            "|=")
PUNCT(pipe_pipe,             "||")
PUNCT(ast,                   "*")
PUNCT(ast_equal,             "*=")
PUNCT(plus,                  "+")
PUNCT(plus_equal,            "+=")
PUNCT(plus_plus,             "++")
PUNCT(minus,                 "-")
PUNCT(minus_equal,           "-=")
PUNCT(minus_minus,           "--")
PUNCT(slash,                 "/")
PUNCT(slash_equal,           "/=")
PUNCT(percent,               "%")
PUNCT(percent_equal,         "%=")
PUNCT(less,                  "<")
PUNCT(less_equal,            "<=")
PUNCT(less_less,             "<<")
PUNCT(less_less_equal,       "<<=")
PUNCT(greater,               ">")
PUNCT(greater_equal,         ">=")
PUNCT(greater_greater,       ">>")
PUNCT(greater_greater_equal, ">>=")
PUNCT(bang,                  "!")
PUNCT(bang_equal,            "!=")
PUNCT(tilde,                 "~")
PUNCT(caret,                 "^")
PUNCT(caret_equal,           "^=")
PUNCT(equal,                 "=")
PUNCT(equal_equal,           "==")

// Keywords
TOKEN(keyword)
KEYWORD(const)
KEYWORD(var)
KEYWORD(func)
KEYWORD(if)
KEYWORD(else)
KEYWORD(class)
KEYWORD(return)
KEYWORD(export)
KEYWORD(import)
KEYWORD(extern)
KEYWORD(while)
KEYWORD(do)
KEYWORD(break)
KEYWORD(continue)
KEYWORD(void)
KEYWORD(float)
KEYWORD(int)
KEYWORD(string)

#undef TOKEN
#undef PUNCT
#undef KEYWORD
