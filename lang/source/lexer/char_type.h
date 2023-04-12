/*
 * Copyright (C) 2016 hedede <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_char_type_h
#define aw_script_char_type_h
#include <functional>
#include <aw/types/types.h>
namespace aw::script {
/*!
 * Character categories
 */
enum char_category {
	name_begin      = 0x01,
	numeric         = 0x02,
	punctuation     = 0x04,
	space           = 0x08,
	reserved1       = 0x10,
	reserved2       = 0x20,
	unicode_head    = 0x40,
	unicode_tail    = 0x80,
	name            = name_begin | numeric,
	token_begin     = name_begin | numeric | punctuation,
	token_separator = space | punctuation,
};

/*!
 * Mapping of characters to categories
 */
static u8 char_types[] = {
//0  SOH  STX  ETX  EOT  ENQ  ACK  BEL  BS    \t   \n   \v   \f   \r   SO   SI
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,0x08,0x00,0x00,
//DLE DC1  DC2  DC2  DC4  NAK  SYN  ETB  CAN  EM   SUB  ESC  FS   GS   RS   US
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//     !    "    #    $    %    &    '    (    )    *    +    ,    -    .    /
0x08,0x04,0x04,0x04,0x04,0x04,0x04,0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,
//0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?
0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x00,0x04,0x04,0x04,0x04,0x00,
//@    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
//P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x04,0x04,0x04,0x04,0x01,
//`    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
//p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~   DEL
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x04,0x04,0x04,0x04,0x00,
//Unicode
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x00,0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x60,
0x40,0x40,0x40,0x40,0x60,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,
};

// clearly spearates two tokens
bool is_separator(char c)
{
	return char_types[(unsigned char)c] & token_separator;
}

bool is_name_char(char c)
{
	return char_types[(unsigned char)c] & char_category::name;
}

bool is_name_begin(char c)
{
	return char_types[(unsigned char)c] & char_category::name_begin;
}

bool is_token_begin(char c)
{
	return char_types[(unsigned char)c] & char_category::token_begin;
}
} // namespace aw::script
#endif//aw_script_char_type_h
