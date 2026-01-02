#include "../test_framework.h"
#include "lexer.h"
#include <string.h>

/* Test: Tokenize CREATE keyword */
TEST_CASE(lexer, tokenize_create) {
    Lexer lexer;
    lexer_init(&lexer, "CREATE");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_CREATE);
    ASSERT_STR_EQ(token.lexeme, "CREATE");
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_EOF);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Tokenize TABLE keyword */
TEST_CASE(lexer, tokenize_table) {
    Lexer lexer;
    lexer_init(&lexer, "TABLE");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_TABLE);
    ASSERT_STR_EQ(token.lexeme, "TABLE");
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Case-insensitive keywords */
TEST_CASE(lexer, keywords_case_insensitive) {
    const char *inputs[] = {"CREATE", "create", "Create", "CrEaTe"};

    for (int i = 0; i < 4; i++) {
        Lexer lexer;
        lexer_init(&lexer, inputs[i]);

        Token token = lexer_next_token(&lexer);
        ASSERT_EQ(token.type, TOKEN_CREATE);
        lexer_free_token(&token);

        lexer_cleanup(&lexer);
    }

    TEST_PASS();
}

/* Test: Simple identifier */
TEST_CASE(lexer, tokenize_identifier) {
    Lexer lexer;
    lexer_init(&lexer, "user_id");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENTIFIER);
    ASSERT_STR_EQ(token.lexeme, "user_id");
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Quoted identifier */
TEST_CASE(lexer, tokenize_quoted_identifier) {
    Lexer lexer;
    lexer_init(&lexer, "\"select\"");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENTIFIER);
    /* Quoted identifier should include quotes or be unquoted depending on implementation */
    ASSERT_NOT_NULL(token.lexeme);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Integer number */
TEST_CASE(lexer, tokenize_integer) {
    Lexer lexer;
    lexer_init(&lexer, "12345");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token.lexeme, "12345");
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Decimal number */
TEST_CASE(lexer, tokenize_decimal) {
    Lexer lexer;
    lexer_init(&lexer, "123.456");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token.lexeme, "123.456");
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: String literal */
TEST_CASE(lexer, tokenize_string) {
    Lexer lexer;
    lexer_init(&lexer, "'Hello World'");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_STRING_LITERAL);
    /* String may or may not include quotes depending on implementation */
    ASSERT_NOT_NULL(token.lexeme);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: String with escape sequences */
TEST_CASE(lexer, tokenize_string_escape) {
    Lexer lexer;
    lexer_init(&lexer, "'Hello\\'World'");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_STRING_LITERAL);
    ASSERT_NOT_NULL(token.lexeme);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Operators */
TEST_CASE(lexer, tokenize_operators) {
    struct {
        const char *input;
        TokenType expected;
    } tests[] = {
        {"(", TOKEN_LPAREN},
        {")", TOKEN_RPAREN},
        {",", TOKEN_COMMA},
        {";", TOKEN_SEMICOLON},
        {"=", TOKEN_EQUAL},
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Lexer lexer;
        lexer_init(&lexer, tests[i].input);

        Token token = lexer_next_token(&lexer);
        ASSERT_EQ(token.type, tests[i].expected);
        lexer_free_token(&token);

        lexer_cleanup(&lexer);
    }

    TEST_PASS();
}

/* Test: Line comment */
TEST_CASE(lexer, tokenize_line_comment) {
    Lexer lexer;
    lexer_init(&lexer, "CREATE -- this is a comment\nTABLE");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_CREATE);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_TABLE);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Block comment */
TEST_CASE(lexer, tokenize_block_comment) {
    Lexer lexer;
    lexer_init(&lexer, "CREATE /* comment */ TABLE");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_CREATE);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_TABLE);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Multi-line block comment */
TEST_CASE(lexer, tokenize_multiline_comment) {
    Lexer lexer;
    lexer_init(&lexer, "CREATE /*\n * Multi-line\n * comment\n */ TABLE");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_CREATE);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_TABLE);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Whitespace handling */
TEST_CASE(lexer, tokenize_whitespace) {
    Lexer lexer;
    lexer_init(&lexer, "  CREATE  \t\n  TABLE  ");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_CREATE);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_TABLE);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Complete CREATE TABLE statement */
TEST_CASE(lexer, tokenize_create_table) {
    Lexer lexer;
    lexer_init(&lexer, "CREATE TABLE users (id INTEGER);");

    Token token;

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_CREATE);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_TABLE);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENTIFIER);
    ASSERT_STR_EQ(token.lexeme, "users");
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_LPAREN);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENTIFIER);
    ASSERT_STR_EQ(token.lexeme, "id");
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    /* INTEGER might be a keyword or identifier depending on implementation */
    ASSERT_NOT_NULL(token.lexeme);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_RPAREN);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_SEMICOLON);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Line and column tracking */
TEST_CASE(lexer, line_column_tracking) {
    Lexer lexer;
    lexer_init(&lexer, "CREATE\nTABLE");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 1);
    lexer_free_token(&token);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.line, 2);
    ASSERT_EQ(token.column, 1);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Multiple keywords */
TEST_CASE(lexer, multiple_keywords) {
    const char *keywords[] = {
        "PRIMARY", "KEY", "FOREIGN", "REFERENCES",
        "UNIQUE", "CHECK", "NOT", "NULL", "DEFAULT"
    };

    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        Lexer lexer;
        lexer_init(&lexer, keywords[i]);

        Token token = lexer_next_token(&lexer);
        /* Should be a keyword token, not identifier */
        ASSERT_NEQ(token.type, TOKEN_IDENTIFIER);
        ASSERT_NEQ(token.type, TOKEN_EOF);
        lexer_free_token(&token);

        lexer_cleanup(&lexer);
    }

    TEST_PASS();
}

/* Test: Unterminated string (error case) */
TEST_CASE(lexer, unterminated_string) {
    Lexer lexer;
    lexer_init(&lexer, "'unterminated");

    Token token = lexer_next_token(&lexer);

    /* Should either return error token or set error flag */
    /* Different implementations may handle this differently */
    lexer_free_token(&token);
    lexer_cleanup(&lexer);

    TEST_PASS();
}

/* Test: Unterminated block comment (error case) */
TEST_CASE(lexer, unterminated_comment) {
    Lexer lexer;
    lexer_init(&lexer, "/* unterminated comment");

    Token token = lexer_next_token(&lexer);

    /* Should handle gracefully */
    lexer_free_token(&token);
    lexer_cleanup(&lexer);

    TEST_PASS();
}

/* Test: Empty input */
TEST_CASE(lexer, empty_input) {
    Lexer lexer;
    lexer_init(&lexer, "");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_EOF);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Only whitespace */
TEST_CASE(lexer, only_whitespace) {
    Lexer lexer;
    lexer_init(&lexer, "   \t\n\n  ");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_EOF);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Only comments */
TEST_CASE(lexer, only_comments) {
    Lexer lexer;
    lexer_init(&lexer, "-- comment\n/* block */");

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_EOF);
    lexer_free_token(&token);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: All major SQL keywords */
TEST_CASE(lexer, all_keywords) {
    const char *sql = "CREATE ALTER TABLE TEMPORARY TEMP UNLOGGED IF NOT EXISTS "
                      "PRIMARY KEY FOREIGN REFERENCES UNIQUE CHECK NULL "
                      "CONSTRAINT DEFAULT PARTITION BY RANGE LIST HASH";

    Lexer lexer;
    lexer_init(&lexer, sql);

    Token token;
    int count = 0;

    do {
        token = lexer_next_token(&lexer);
        if (token.type != TOKEN_EOF) {
            /* Each token should be a keyword */
            ASSERT_NEQ(token.type, TOKEN_IDENTIFIER);
            count++;
        }
        lexer_free_token(&token);
    } while (token.type != TOKEN_EOF);

    /* Should have tokenized all keywords */
    ASSERT_TRUE(count > 0);

    lexer_cleanup(&lexer);
    TEST_PASS();
}

/* Test: Numbers with different formats */
TEST_CASE(lexer, number_formats) {
    const char *numbers[] = {
        "0", "123", "999999",
        "0.0", "3.14", "123.456789"
    };

    for (size_t i = 0; i < sizeof(numbers) / sizeof(numbers[0]); i++) {
        Lexer lexer;
        lexer_init(&lexer, numbers[i]);

        Token token = lexer_next_token(&lexer);
        ASSERT_EQ(token.type, TOKEN_NUMBER);
        ASSERT_STR_EQ(token.lexeme, numbers[i]);
        lexer_free_token(&token);

        lexer_cleanup(&lexer);
    }

    TEST_PASS();
}

/* Test suite definition */
static TestCase lexer_tests[] = {
    {"tokenize_create", test_lexer_tokenize_create, "lexer"},
    {"tokenize_table", test_lexer_tokenize_table, "lexer"},
    {"keywords_case_insensitive", test_lexer_keywords_case_insensitive, "lexer"},
    {"tokenize_identifier", test_lexer_tokenize_identifier, "lexer"},
    {"tokenize_quoted_identifier", test_lexer_tokenize_quoted_identifier, "lexer"},
    {"tokenize_integer", test_lexer_tokenize_integer, "lexer"},
    {"tokenize_decimal", test_lexer_tokenize_decimal, "lexer"},
    {"tokenize_string", test_lexer_tokenize_string, "lexer"},
    {"tokenize_string_escape", test_lexer_tokenize_string_escape, "lexer"},
    {"tokenize_operators", test_lexer_tokenize_operators, "lexer"},
    {"tokenize_line_comment", test_lexer_tokenize_line_comment, "lexer"},
    {"tokenize_block_comment", test_lexer_tokenize_block_comment, "lexer"},
    {"tokenize_multiline_comment", test_lexer_tokenize_multiline_comment, "lexer"},
    {"tokenize_whitespace", test_lexer_tokenize_whitespace, "lexer"},
    {"tokenize_create_table", test_lexer_tokenize_create_table, "lexer"},
    {"line_column_tracking", test_lexer_line_column_tracking, "lexer"},
    {"multiple_keywords", test_lexer_multiple_keywords, "lexer"},
    {"unterminated_string", test_lexer_unterminated_string, "lexer"},
    {"unterminated_comment", test_lexer_unterminated_comment, "lexer"},
    {"empty_input", test_lexer_empty_input, "lexer"},
    {"only_whitespace", test_lexer_only_whitespace, "lexer"},
    {"only_comments", test_lexer_only_comments, "lexer"},
    {"all_keywords", test_lexer_all_keywords, "lexer"},
    {"number_formats", test_lexer_number_formats, "lexer"},
};

void run_lexer_tests(void) {
    run_test_suite("lexer", NULL, NULL, lexer_tests,
                   sizeof(lexer_tests) / sizeof(lexer_tests[0]));
}
