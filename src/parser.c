#include "parser.h"


char *token_type_to_cstring(token_type t)
{
    switch (t)
    {
        case TOKEN_INVALID: return "TOKEN_INVALID";
        case TOKEN_PAREN_OPEN: return "TOKEN_PAREN_OPEN";
        case TOKEN_PAREN_CLOSE: return "TOKEN_PAREN_CLOSE";
        case TOKEN_BRACKET_OPEN: return "TOKEN_BRACKET_OPEN";
        case TOKEN_BRACKET_CLOSE: return "TOKEN_BRACKET_CLOSE";
        case TOKEN_BRACE_OPEN: return "TOKEN_BRACE_OPEN";
        case TOKEN_BRACE_CLOSE: return "TOKEN_BRACE_CLOSE";
        case TOKEN_EQUALS: return "TOKEN_EQUALS";
        case TOKEN_COLON: return "TOKEN_COLON";
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_LITERAL_INT: return "TOKEN_LITERAL_INT";
        case TOKEN_DOUBLE_COLON: return "TOKEN_DOUBLE_COLON";
        case TOKEN_ARROW_RIGHT: return "TOKEN_ARROW_RIGHT";
        case TOKEN_KW_RETURN: return "TOKEN_KW_RETURN";
        case TOKEN_EOF: return "TOKEN_EOF";
        
        default:
            return "<token_type>";
    }
    return 0;
}

b32 is_ascii_space(char c)
{
    return (c == ' ');
}

b32 is_ascii_whitespace(char c)
{
    return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

b32 is_ascii_alpha(char c)
{
    return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}

b32 is_ascii_digit(char c)
{
    return (c >= '0') && (c <= '9');
}

b32 is_valid_identifier_head(char c)
{
    return (c == '_') || is_ascii_alpha(c);
}

b32 is_valid_identifier_body(char c)
{
    return (c == '_') || is_ascii_alpha(c) || is_ascii_digit(c);
}


char lexer_get_char(lexer *l)
{
    char c = 0;
    if (l->index < l->buffer_size)
    {
        c = l->buffer[l->index];
    }
    return c;
}

char lexer_eat_char(lexer *l)
{
    char c = lexer_get_char(l);
    l->index += 1;
    if (c == '\n')
    {
        l->line += 1;
        l->column = 0;
    }
    l->column += 1;
    return c;
}

void consume_while(lexer *l, predicate *p)
{
    char c = lexer_get_char(l);
    while (p(c))
    {
        lexer_eat_char(l);
        c = lexer_get_char(l);
    }
}

void consume_until(lexer *l, predicate *p)
{
    char c = lexer_get_char(l);
    while (!p(c))
    {
        lexer_eat_char(l);
        c = lexer_get_char(l);
    }
}

token lexer_get_token(lexer *l)
{
    if (!l->next_token_valid)
    {
        consume_while(l, is_ascii_whitespace);
       
        token t = {0};

        char c = lexer_get_char(l);
        if (c == 0)
        {
            t.type = TOKEN_EOF;
            t.line = l->line;
            t.column = l->column;
            t.span = l->buffer + l->index;
            t.span_size = 1;
        }
        else if (is_valid_identifier_head(c))
        {
            t.type = TOKEN_IDENTIFIER;
            t.line = l->line;
            t.column = l->column;
            t.span = l->buffer + l->index;

            lexer_eat_char(l);
            c = lexer_get_char(l);
            int span_size = 1;

            while (is_valid_identifier_body(c))
            {
                lexer_eat_char(l);
                c = lexer_get_char(l);
                span_size += 1;
            }

            t.span_size = span_size;
        }
        else if (is_ascii_digit(c))
        {
            t.type = TOKEN_LITERAL_INT;
            t.line = l->line;
            t.column = l->column;
            t.span = l->buffer + l->index;
            t.span_size = 0;

            int integer = 0;

            while (is_ascii_digit(c))
            {
                integer *= 10;
                integer += (c - '0');
                lexer_eat_char(l);
                c = lexer_get_char(l);
                t.span_size += 1;
            }

            t.integer_value = integer;
        }
        else
        {
            t.line = l->line;
            t.column = l->column;
            t.span = l->buffer + l->index;

            if (c == ':')
            {
                lexer_eat_char(l);
                c = lexer_get_char(l);
                if (c == ':')
                {
                    t.type = TOKEN_DOUBLE_COLON;
                    lexer_eat_char(l);
                    t.span_size = 2;
                }
                else
                {
                    t.type = TOKEN_COLON;
                    t.span_size = 1;
                }
            }
            else if (c == '-')
            {
                lexer_eat_char(l);
                c = lexer_get_char(l);
                if (c == '>')
                {
                    t.type = TOKEN_ARROW_RIGHT;
                    lexer_eat_char(l);
                    t.span_size = 2;
                }
                else
                {
                    t.type = TOKEN_MINUS;
                    t.span_size = 1;
                }
            }
            else
            {
                lexer_eat_char(l);
                t.type = (token_type) c;
                t.span_size = 1;
            }
        }

        l->next_token = t;
        l->next_token_valid = true;
    }

    return l->next_token;
}

token lexer_eat_token(lexer *l)
{
    token result = lexer_get_token(l);
    l->next_token_valid = false;
    return result;
}

