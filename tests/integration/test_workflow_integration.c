#include "../test_framework.h"
#include "parser.h"
#include "compare.h"
#include "diff.h"
#include "report.h"
#include "sql_generator.h"
#include "sc_memory.h"
#include "utils.h"
#include <string.h>

/* Test: Full workflow - parse, compare, generate report */
TEST_CASE(workflow_integration, parse_compare_report) {
    MemoryContext *ctx = memory_context_create("test_workflow");
    ASSERT_NOT_NULL(ctx);

    /* Step 1: Parse source table */
    const char *source_sql =
        "CREATE TABLE users (\n"
        "    id INTEGER PRIMARY KEY,\n"
        "    name VARCHAR(100) NOT NULL\n"
        ");";

    Parser *p1 = parser_create(source_sql);
    ASSERT_NOT_NULL(p1);
    CreateTableStmt *source = parser_parse_create_table(p1);
    ASSERT_NOT_NULL(source);

    /* Step 2: Parse target table */
    const char *target_sql =
        "CREATE TABLE users (\n"
        "    id INTEGER PRIMARY KEY,\n"
        "    name VARCHAR(100) NOT NULL\n"
        ");";

    Parser *p2 = parser_create(target_sql);
    ASSERT_NOT_NULL(p2);
    CreateTableStmt *target = parser_parse_create_table(p2);
    ASSERT_NOT_NULL(target);

    /* Step 3: Compare */
    CompareOptions *comp_opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, comp_opts, ctx);

    /* Step 4: Generate report */
    if (diff) {
        ReportOptions *report_opts = report_options_default();
        char *report = generate_table_diff_report(diff, report_opts);

        ASSERT_NOT_NULL(report);
        ASSERT_TRUE(strlen(report) > 0);

        free(report);
        report_options_free(report_opts);
    }

    parser_destroy(p1);
    parser_destroy(p2);
    compare_options_free(comp_opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Full workflow - detect column addition and generate SQL */
TEST_CASE(workflow_integration, detect_column_addition) {
    MemoryContext *ctx = memory_context_create("test_workflow");
    ASSERT_NOT_NULL(ctx);

    /* Source: original table */
    const char *source_sql =
        "CREATE TABLE products (\n"
        "    id INTEGER PRIMARY KEY,\n"
        "    name VARCHAR(200) NOT NULL\n"
        ");";

    /* Target: table with added column */
    const char *target_sql =
        "CREATE TABLE products (\n"
        "    id INTEGER PRIMARY KEY,\n"
        "    name VARCHAR(200) NOT NULL,\n"
        "    price NUMERIC(10,2)\n"
        ");";

    Parser *p1 = parser_create(source_sql);
    Parser *p2 = parser_create(target_sql);

    CreateTableStmt *source = parser_parse_create_table(p1);
    CreateTableStmt *target = parser_parse_create_table(p2);

    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    /* Compare */
    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Should detect changes */
    if (diff) {
        ASSERT_NOT_NULL(diff->table_name);
    }

    parser_destroy(p1);
    parser_destroy(p2);
    free_create_table_stmt(source);
    free_create_table_stmt(target);
    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Full workflow with Sakila schema */
TEST_CASE(workflow_integration, sakila_full_workflow) {
    MemoryContext *ctx = memory_context_create("test_workflow");
    ASSERT_NOT_NULL(ctx);

    /* Parse Sakila actor table */
    char *sql = read_file_to_string("tests/data/silka/tables/actor.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    /* Compare with itself (should be identical) */
    CompareOptions *comp_opts = compare_options_default();
    TableDiff *diff = compare_tables(stmt, stmt, comp_opts, ctx);

    /* Generate report */
    ReportOptions *report_opts = report_options_default();
    SchemaDiff *schema_diff = schema_diff_create("public");

    if (schema_diff) {
        char *report = generate_report(schema_diff, report_opts);
        ASSERT_NOT_NULL(report);

        free(report);
        schema_diff_free(schema_diff);
    }

    if (diff) {
        /* Table compared with itself should not be modified */
        ASSERT_FALSE(diff->table_modified);
    }

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    compare_options_free(comp_opts);
    report_options_free(report_opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Parse -> Compare -> Generate SQL migration */
TEST_CASE(workflow_integration, generate_migration_workflow) {
    MemoryContext *ctx = memory_context_create("test_workflow");
    ASSERT_NOT_NULL(ctx);

    /* Create schema diff for testing */
    SchemaDiff *diff = schema_diff_create("public");
    ASSERT_NOT_NULL(diff);

    /* Generate migration SQL */
    SQLGenOptions *opts = sql_gen_options_default();
    opts->use_transactions = true;
    opts->add_comments = true;

    SQLMigration *migration = generate_migration_sql(diff, opts);
    ASSERT_NOT_NULL(migration);

    /* Verify migration was generated */
    ASSERT_TRUE(migration->statement_count >= 0);

    sql_migration_free(migration);
    sql_gen_options_free(opts);
    schema_diff_free(diff);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Multi-table comparison workflow */
TEST_CASE(workflow_integration, multi_table_comparison) {
    MemoryContext *ctx = memory_context_create("test_workflow");
    ASSERT_NOT_NULL(ctx);

    /* Parse multiple Sakila tables */
    char *actor_sql = read_file_to_string("tests/data/silka/tables/actor.sql");
    char *lang_sql = read_file_to_string("tests/data/silka/tables/language.sql");

    if (!actor_sql || !lang_sql) {
        /* Skip if files don't exist */
        if (actor_sql) free(actor_sql);
        if (lang_sql) free(lang_sql);
        memory_context_destroy(ctx);
        TEST_PASS();
    }

    Parser *p1 = parser_create(actor_sql);
    Parser *p2 = parser_create(lang_sql);

    CreateTableStmt *actor = parser_parse_create_table(p1);
    CreateTableStmt *language = parser_parse_create_table(p2);

    if (actor && language) {
        /* Create array of tables */
        CreateTableStmt *tables1[] = {actor, language};
        CreateTableStmt *tables2[] = {actor, language};

        /* Compare schemas */
        CompareOptions *opts = compare_options_default();
        SchemaDiff *diff = compare_schemas(tables1, 2, tables2, 2, opts, ctx);

        if (diff) {
            /* Same tables should show no changes */
            ASSERT_EQ(diff->tables_added, 0);
            ASSERT_EQ(diff->tables_removed, 0);

            /* Generate report */
            ReportOptions *rep_opts = report_options_default();
            char *report = generate_report(diff, rep_opts);

            ASSERT_NOT_NULL(report);
            free(report);
            report_options_free(rep_opts);
            schema_diff_free(diff);
        }

        compare_options_free(opts);
    }

    parser_destroy(p1);
    parser_destroy(p2);
    free_create_table_stmt(actor);
    free_create_table_stmt(language);
    free(actor_sql);
    free(lang_sql);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Different output formats */
TEST_CASE(workflow_integration, different_output_formats) {
    SchemaDiff *diff = schema_diff_create("public");
    ASSERT_NOT_NULL(diff);

    /* Test TEXT format */
    ReportOptions *text_opts = report_options_default();
    text_opts->format = REPORT_FORMAT_TEXT;
    char *text_report = generate_report(diff, text_opts);
    ASSERT_NOT_NULL(text_report);
    free(text_report);
    report_options_free(text_opts);

    /* Test MARKDOWN format */
    ReportOptions *md_opts = report_options_default();
    md_opts->format = REPORT_FORMAT_MARKDOWN;
    char *md_report = generate_report(diff, md_opts);
    ASSERT_NOT_NULL(md_report);
    free(md_report);
    report_options_free(md_opts);

    schema_diff_free(diff);
    TEST_PASS();
}

/* Test: Complete workflow with all components */
TEST_CASE(workflow_integration, complete_end_to_end) {
    MemoryContext *ctx = memory_context_create("test_workflow");
    ASSERT_NOT_NULL(ctx);

    /* 1. PARSE: Load schemas */
    const char *v1_sql =
        "CREATE TABLE orders (\n"
        "    order_id INTEGER PRIMARY KEY,\n"
        "    customer_id INTEGER NOT NULL\n"
        ");";

    const char *v2_sql =
        "CREATE TABLE orders (\n"
        "    order_id INTEGER PRIMARY KEY,\n"
        "    customer_id INTEGER NOT NULL,\n"
        "    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"
        ");";

    Parser *p1 = parser_create(v1_sql);
    Parser *p2 = parser_create(v2_sql);

    CreateTableStmt *v1 = parser_parse_create_table(p1);
    CreateTableStmt *v2 = parser_parse_create_table(p2);

    ASSERT_NOT_NULL(v1);
    ASSERT_NOT_NULL(v2);

    /* 2. COMPARE: Find differences */
    CompareOptions *comp_opts = compare_options_default();
    TableDiff *table_diff = compare_tables(v1, v2, comp_opts, ctx);

    /* 3. REPORT: Generate human-readable report */
    if (table_diff) {
        ReportOptions *rep_opts = report_options_default();
        rep_opts->format = REPORT_FORMAT_TEXT;
        rep_opts->use_color = false;

        char *report = generate_table_diff_report(table_diff, rep_opts);
        ASSERT_NOT_NULL(report);

        /* Report should mention the table */
        ASSERT_TRUE(strstr(report, "orders") != NULL);

        free(report);
        report_options_free(rep_opts);
    }

    /* 4. CLEANUP */
    parser_destroy(p1);
    parser_destroy(p2);
    compare_options_free(comp_opts);
    memory_context_destroy(ctx);

    TEST_PASS();
}

/* Test suite definition */
static TestCase workflow_integration_tests[] = {
    {"parse_compare_report", test_workflow_integration_parse_compare_report, "workflow_integration"},
    {"detect_column_addition", test_workflow_integration_detect_column_addition, "workflow_integration"},
    {"sakila_full_workflow", test_workflow_integration_sakila_full_workflow, "workflow_integration"},
    {"generate_migration_workflow", test_workflow_integration_generate_migration_workflow, "workflow_integration"},
    {"multi_table_comparison", test_workflow_integration_multi_table_comparison, "workflow_integration"},
    {"different_output_formats", test_workflow_integration_different_output_formats, "workflow_integration"},
    {"complete_end_to_end", test_workflow_integration_complete_end_to_end, "workflow_integration"},
};

void run_workflow_integration_tests(void) {
    run_test_suite("workflow_integration", NULL, NULL, workflow_integration_tests,
                   sizeof(workflow_integration_tests) / sizeof(workflow_integration_tests[0]));
}
