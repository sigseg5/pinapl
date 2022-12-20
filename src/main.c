#include <syscall.h>
#include <allocator.h>
#include <print.h>
#include <string.h>
#include <parser.h>


char const *spaces = "                                           ";

void print_ast(ast_node *node, int depth)
{
    switch (node->type)
    {
        case AST_NODE_EMPTY_LIST:
        break;

        case AST_NODE_BINARY_OPERATOR:
        {
            ast_node_binary_operator *binary_op = &node->binary_operator;
            if (binary_op->lhs) print_ast(binary_op->lhs, depth);
            if (binary_op->rhs) print_ast(binary_op->rhs, depth);
            print_n(binary_op->op.span, binary_op->op.span_size); 
        }
        break;

        case AST_NODE_VARIABLE:
        {
            print_n(node->variable.span, node->variable.span_size);
            print_f("(%d)", node->variable.symbol_id);
        }
        break;

        case AST_NODE_LITERAL_INT:
        {
            print_n(node->integer_literal.span, node->integer_literal.span_size);
        }
        break;
        
        case AST_NODE_VARIABLE_DECLARATION:
        case AST_NODE_CONSTANT_DECLARATION:
        {
            b32 is_constant = (node->type == AST_NODE_CONSTANT_DECLARATION);
            ast_node_variable_declaration *var = &node->variable_declaration;

            print_f("VAR(%d) {", var->symbol_id);
            print_n(var->var_name.span, var->var_name.span_size);
            print(":");
            if (var->var_type.type != TOKEN_INVALID)
            {
                print_n(var->var_type.span, var->var_type.span_size);
            }
            if (var->init)
            {
                print(is_constant ? ":" : "=");
                print_ast(var->init, depth);
            }
            print("}");
        }
        break;

        case AST_NODE_BLOCK:
        {
            print("{\n");
            if (node->block.statement_list)
            {
                print_ast(node->block.statement_list, depth + 1);
            }
            print_n(spaces, depth * 2);
            print("}\n");
        }
        break;

        case AST_NODE_FUNCTION_DEFINITION:
        {
            print("()");
            if (node->function_definition.block)
            {
                print("\n");
                print_n(spaces, (depth + 1) * 2);
                print_ast(node->function_definition.block, depth + 1);
            }
        }
        break;

        case AST_NODE_FUNCTION_CALL:
        {
            print_n(node->function_call.name.span, node->function_call.name.span_size);
            print("()");
        }
        break;

        case AST_NODE_STATEMENT_LIST:
        {
            print_n(spaces, depth * 2);
            print_ast(node->list.node, depth);
            if (node->list.node->type != AST_NODE_BLOCK)
            {
                print(";");
            }
            print("\n");
            if (node->list.next)
            {
                print_ast(node->list.next, depth);
            }
        }
        break;

        case AST_NODE_GLOBAL_DECLARATION_LIST:
        {
            print_ast(node->list.node, depth);
            print("\n");
            if (node->list.next)
            {
                print_ast(node->list.next, depth);
            }
        }
        break;

        default:
        {
            print("<!!!>");
        }
    }
}


void print_tacs(struct pinapl_tac *codes, usize code_count)
{
    usize code_index = 0;
    while (code_index < code_count)
    {
        struct pinapl_tac *code = codes + code_index++;
        switch (code->type)
        {
            case TAC_NOP:
            {
                print("nop\n");
            }
            break;

            case TAC_MOV_REG:
            {
                print_f("mov r%d, r%d\n", code->dst, code->lhs);
            }
            break;

            case TAC_MOV_INT:
            {
                print_f("mov r%d, #%d\n", code->dst, code->lhs);
            }
            break;

            case TAC_ADD:
            {
                print_f("add r%d, r%d, r%d\n", code->dst, code->lhs, code->rhs);
            }
            break;

            case TAC_SUB:
            {
                print_f("sub r%d, r%d, r%d\n", code->dst, code->lhs, code->rhs);
            }
            break;

            case TAC_MUL:
            {
                print_f("mul r%d, r%d, r%d\n", code->dst, code->lhs, code->rhs);
            }
            break;

            case TAC_DIV:
            {
                print_f("div r%d r%d r%d\n", code->dst, code->lhs, code->rhs);
            }
            break;

            default:
            {
                print("<ERROR!!!>\n");
            }
            break;
        }
    }
}


int main(int argc, char **argv, char **env)
{
    if (argc < 2)
    {
        write(1, "Usage: ./pinapl FILEPATH\n", 25);
        return 1;
    }

    usize memory_buffer_size = MEGABYTES(5);
    void *memory_buffer = mmap2(0, memory_buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    struct allocator arenas;
    initialize_memory_arena(&arenas, memory_buffer, memory_buffer_size);

    usize memory_for_print_buffer_size = KILOBYTES(5);
    void *memory_for_print_buffer = ALLOCATE_BUFFER_(&arenas, memory_for_print_buffer_size);

    initialize_print_buffer(memory_for_print_buffer, memory_for_print_buffer_size);

    struct allocator ast_allocator;
    {
        usize memory_for_ast_size = MEGABYTES(1);
        void *memory_for_ast = ALLOCATE_BUFFER_(&arenas, memory_for_ast_size);
        initialize_memory_arena(&ast_allocator, memory_for_ast, memory_for_ast_size);
    }

    struct allocator scope_allocator;
    {
        usize memory_for_scopes_size = MEGABYTES(1);
        void *memory_for_scopes = ALLOCATE_BUFFER_(&arenas, memory_for_scopes_size); 
        initialize_memory_arena(&scope_allocator, memory_for_scopes, memory_for_scopes_size);
    }

    usize memory_for_three_address_codes_size = MEGABYTES(1);
    void *memory_for_three_address_codes = ALLOCATE_BUFFER_(&arenas, memory_for_three_address_codes_size);

    usize memory_for_labels_size = KILOBYTES(5);
    void *memory_for_labels = ALLOCATE_BUFFER_(&arenas, memory_for_labels_size);

    struct allocator err_allocator;
    {
        usize memory_for_err_size = KILOBYTES(5);
        void *memory_for_err = ALLOCATE_BUFFER_(&arenas, memory_for_err_size);
        initialize_memory_arena(&err_allocator, memory_for_err, memory_for_err_size);
    }

    char const *filename = argv[1];

    int fd = open(filename, 0, O_RDONLY);
    if (fd <= 0)
    {
        write(1, "Error < 0\n", 10);
        return 1;
    }

    char buffer[4096] = {0};
    int  buffer_size = read(fd, buffer, ARRAY_COUNT(buffer) - 1);
    close(fd);

    write(1, buffer, buffer_size);
    write(1, "EOF\n", 4);

    struct pinapl_parser parser = pinapl_init_parser(&ast_allocator, &err_allocator, filename, buffer, buffer_size);
    ast_node *ast = pinapl_parse_global_declaration_list(&parser);

    token t = pinapl_get_token(&parser);
    if (ast && t.type == TOKEN_EOF)
    {
        write(1, "Language recognized!\n", 21);
    
        struct pinapl_scope global_scope = {0};

        struct pinapl_rename_stage rename_stage =
        {
            .global_variable_counter = 0,
            .scope_allocator = &scope_allocator,
            .err_allocator = &err_allocator,
        };

        b32 good = pinapl_check_and_rename_variables(&rename_stage, ast, &global_scope);
        if (good)
        {
            write(1, "Check is good\n", 14);
            print_ast(ast, 0);

            struct pinapl_flatten_stage flatten_stage =
            {
                .global_variable_counter = rename_stage.global_variable_counter,

                .codes = memory_for_three_address_codes,
                .codes_size = memory_for_three_address_codes_size / sizeof(struct pinapl_tac),
                .code_count = 0,

                .labels = memory_for_labels,
                .labels_size = memory_for_labels_size,
                .label_count = 0,
            };

            pinapl_flatten_ast(&flatten_stage, ast);

            print_tacs(flatten_stage.codes, flatten_stage.code_count);
        }
        else
        {
            write(1, "Check is bad\n", 13);
        }
    }
    else
    {
        write(1, "Language is not recognized!\n", 28);
        struct string err = pinapl_parser_get_error_string(&parser);
        write(1, err.data, err.size);
    }

    print_f("blah %d foo %d\n", 42, -1);

    print_flush();

    return 0;
}

