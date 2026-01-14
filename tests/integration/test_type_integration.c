#include "../test_framework.h"
#include "parser.h"
#include "pg_create_type.h"
#include "utils.h"
#include <string.h>

/* ========== ENUM Type Tests ========== */

/* Test: Basic ENUM with multiple labels */
TEST_CASE(type_integration, parse_enum_basic) {
    char *sql = "CREATE TYPE status AS ENUM ('active', 'inactive', 'pending');";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_name, "status");
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_ENUM);

    /* Verify count */
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 3);
    ASSERT_NOT_NULL(stmt->type_def.enum_def.labels);

    /* Verify each label individually */
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[0], "active");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[1], "inactive");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[2], "pending");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: ENUM with single label */
TEST_CASE(type_integration, parse_enum_single_label) {
    char *sql = "CREATE TYPE singleton AS ENUM ('only');";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_name, "singleton");
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_ENUM);
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 1);
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[0], "only");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: ENUM with many labels */
TEST_CASE(type_integration, parse_enum_many_labels) {
    char *sql = "CREATE TYPE days AS ENUM ('mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun');";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_ENUM);
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 7);

    /* Verify each day */
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[0], "mon");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[1], "tue");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[2], "wed");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[3], "thu");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[4], "fri");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[5], "sat");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[6], "sun");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: ENUM with special characters */
TEST_CASE(type_integration, parse_enum_special_chars) {
    char *sql = "CREATE TYPE weird AS ENUM ('with space', 'with-dash', 'with_underscore');";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 3);
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[0], "with space");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[1], "with-dash");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[2], "with_underscore");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: ENUM with schema qualification */
TEST_CASE(type_integration, parse_enum_schema_qualified) {
    char *sql = "CREATE TYPE myschema.status AS ENUM ('on', 'off');";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_name, "myschema.status");
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_ENUM);
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 2);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: ENUM with case sensitivity */
TEST_CASE(type_integration, parse_enum_case_sensitive) {
    char *sql = "CREATE TYPE mixed AS ENUM ('Lower', 'UPPER', 'MiXeD');";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 3);
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[0], "Lower");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[1], "UPPER");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[2], "MiXeD");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: ENUM from Sakila sample */
TEST_CASE(type_integration, parse_enum_mpaa_rating) {
    char *sql = "CREATE TYPE mpaa_rating AS ENUM ('G', 'PG', 'PG-13', 'R', 'NC-17');";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_name, "mpaa_rating");
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_ENUM);
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 5);
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[0], "G");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[1], "PG");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[2], "PG-13");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[3], "R");
    ASSERT_STR_EQ(stmt->type_def.enum_def.labels[4], "NC-17");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* ========== COMPOSITE Type Tests ========== */

/* Test: Basic COMPOSITE type */
TEST_CASE(type_integration, parse_composite_basic) {
    char *sql = "CREATE TYPE address AS (street text, city text, zip int);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_name, "address");
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_COMPOSITE);

    /* Verify count */
    ASSERT_EQ(stmt->type_def.composite_def.attribute_count, 3);

    /* Verify first attribute */
    CompositeAttribute *attr = stmt->type_def.composite_def.attributes;
    ASSERT_NOT_NULL(attr);
    ASSERT_STR_EQ(attr->attr_name, "street");
    ASSERT_STR_EQ(attr->data_type, "text");
    ASSERT_NULL(attr->collation);

    /* Verify second attribute */
    attr = attr->next;
    ASSERT_NOT_NULL(attr);
    ASSERT_STR_EQ(attr->attr_name, "city");
    ASSERT_STR_EQ(attr->data_type, "text");
    ASSERT_NULL(attr->collation);

    /* Verify third attribute */
    attr = attr->next;
    ASSERT_NOT_NULL(attr);
    ASSERT_STR_EQ(attr->attr_name, "zip");
    ASSERT_STR_EQ(attr->data_type, "int");
    ASSERT_NULL(attr->collation);
    ASSERT_NULL(attr->next);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: COMPOSITE with COLLATE */
TEST_CASE(type_integration, parse_composite_with_collation) {
    char *sql = "CREATE TYPE person AS (name text COLLATE \"C\", age int);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_COMPOSITE);
    ASSERT_EQ(stmt->type_def.composite_def.attribute_count, 2);

    /* Verify first attribute with collation */
    CompositeAttribute *attr = stmt->type_def.composite_def.attributes;
    ASSERT_NOT_NULL(attr);
    ASSERT_STR_EQ(attr->attr_name, "name");
    ASSERT_STR_EQ(attr->data_type, "text");
    ASSERT_NOT_NULL(attr->collation);
    ASSERT_STR_EQ(attr->collation, "C");

    /* Verify second attribute */
    attr = attr->next;
    ASSERT_NOT_NULL(attr);
    ASSERT_STR_EQ(attr->attr_name, "age");
    ASSERT_STR_EQ(attr->data_type, "int");
    ASSERT_NULL(attr->collation);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: COMPOSITE with single attribute */
TEST_CASE(type_integration, parse_composite_single_attr) {
    char *sql = "CREATE TYPE wrapper AS (value bigint);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type_def.composite_def.attribute_count, 1);

    CompositeAttribute *attr = stmt->type_def.composite_def.attributes;
    ASSERT_NOT_NULL(attr);
    ASSERT_STR_EQ(attr->attr_name, "value");
    ASSERT_STR_EQ(attr->data_type, "bigint");
    ASSERT_NULL(attr->next);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: COMPOSITE with many attributes */
TEST_CASE(type_integration, parse_composite_many_attrs) {
    char *sql = "CREATE TYPE complex AS (f1 int, f2 text, f3 bool, f4 numeric, f5 timestamp);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type_def.composite_def.attribute_count, 5);

    /* Verify all attributes */
    CompositeAttribute *attr = stmt->type_def.composite_def.attributes;
    ASSERT_STR_EQ(attr->attr_name, "f1");
    ASSERT_STR_EQ(attr->data_type, "int");

    attr = attr->next;
    ASSERT_STR_EQ(attr->attr_name, "f2");
    ASSERT_STR_EQ(attr->data_type, "text");

    attr = attr->next;
    ASSERT_STR_EQ(attr->attr_name, "f3");
    ASSERT_STR_EQ(attr->data_type, "bool");

    attr = attr->next;
    ASSERT_STR_EQ(attr->attr_name, "f4");
    ASSERT_STR_EQ(attr->data_type, "numeric");

    attr = attr->next;
    ASSERT_STR_EQ(attr->attr_name, "f5");
    ASSERT_STR_EQ(attr->data_type, "timestamp");
    ASSERT_NULL(attr->next);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: COMPOSITE with array types */
TEST_CASE(type_integration, parse_composite_array_types) {
    char *sql = "CREATE TYPE arrays AS (tags text[], scores integer[]);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type_def.composite_def.attribute_count, 2);

    CompositeAttribute *attr = stmt->type_def.composite_def.attributes;
    ASSERT_STR_EQ(attr->attr_name, "tags");
    ASSERT_STR_EQ(attr->data_type, "text[]");

    attr = attr->next;
    ASSERT_STR_EQ(attr->attr_name, "scores");
    ASSERT_STR_EQ(attr->data_type, "integer[]");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: COMPOSITE with precision types */
TEST_CASE(type_integration, parse_composite_precision) {
    char *sql = "CREATE TYPE precise AS (amount numeric(10,2), created timestamp(6));";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type_def.composite_def.attribute_count, 2);

    CompositeAttribute *attr = stmt->type_def.composite_def.attributes;
    ASSERT_STR_EQ(attr->attr_name, "amount");
    ASSERT_STR_EQ(attr->data_type, "numeric(10,2)");

    attr = attr->next;
    ASSERT_STR_EQ(attr->attr_name, "created");
    ASSERT_STR_EQ(attr->data_type, "timestamp(6)");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* ========== RANGE Type Tests ========== */

/* Test: Minimal RANGE with only SUBTYPE */
TEST_CASE(type_integration, parse_range_minimal) {
    char *sql = "CREATE TYPE int_range AS RANGE (SUBTYPE = int4);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_name, "int_range");
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_RANGE);

    /* Verify SUBTYPE */
    ASSERT_NOT_NULL(stmt->type_def.range_def.subtype);
    ASSERT_STR_EQ(stmt->type_def.range_def.subtype, "int4");

    /* Verify optional params are NULL */
    ASSERT_NULL(stmt->type_def.range_def.subtype_opclass);
    ASSERT_NULL(stmt->type_def.range_def.collation);
    ASSERT_NULL(stmt->type_def.range_def.canonical_function);
    ASSERT_NULL(stmt->type_def.range_def.subtype_diff_function);
    ASSERT_NULL(stmt->type_def.range_def.multirange_type_name);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: RANGE with all parameters */
TEST_CASE(type_integration, parse_range_full) {
    char *sql = "CREATE TYPE float_range AS RANGE ("
                "SUBTYPE = float8, "
                "subtype_opclass = float_ops, "
                "canonical = float8_canonical, "
                "subtype_diff = float8_mi"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_RANGE);

    /* Verify all parameters */
    ASSERT_NOT_NULL(stmt->type_def.range_def.subtype);
    ASSERT_STR_EQ(stmt->type_def.range_def.subtype, "float8");

    ASSERT_NOT_NULL(stmt->type_def.range_def.subtype_opclass);
    ASSERT_STR_EQ(stmt->type_def.range_def.subtype_opclass, "float_ops");

    ASSERT_NOT_NULL(stmt->type_def.range_def.canonical_function);
    ASSERT_STR_EQ(stmt->type_def.range_def.canonical_function, "float8_canonical");

    ASSERT_NOT_NULL(stmt->type_def.range_def.subtype_diff_function);
    ASSERT_STR_EQ(stmt->type_def.range_def.subtype_diff_function, "float8_mi");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: RANGE with MULTIRANGE_TYPE_NAME */
TEST_CASE(type_integration, parse_range_multirange) {
    char *sql = "CREATE TYPE date_range AS RANGE ("
                "SUBTYPE = date, "
                "multirange_type_name = date_multirange"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_RANGE);

    ASSERT_STR_EQ(stmt->type_def.range_def.subtype, "date");
    ASSERT_NOT_NULL(stmt->type_def.range_def.multirange_type_name);
    ASSERT_STR_EQ(stmt->type_def.range_def.multirange_type_name, "date_multirange");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: RANGE with timestamp subtype */
TEST_CASE(type_integration, parse_range_timestamp) {
    char *sql = "CREATE TYPE tsrange AS RANGE (SUBTYPE = timestamp);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_def.range_def.subtype, "timestamp");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* ========== BASE Type Tests ========== */

/* Test: Minimal BASE type with INPUT/OUTPUT */
TEST_CASE(type_integration, parse_base_minimal) {
    char *sql = "CREATE TYPE mytype ("
                "input = mytype_in, "
                "output = mytype_out"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_name, "mytype");
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_BASE);

    /* Verify required functions */
    ASSERT_NOT_NULL(stmt->type_def.base_def.input_function);
    ASSERT_STR_EQ(stmt->type_def.base_def.input_function, "mytype_in");

    ASSERT_NOT_NULL(stmt->type_def.base_def.output_function);
    ASSERT_STR_EQ(stmt->type_def.base_def.output_function, "mytype_out");

    /* Verify optional params are not set */
    ASSERT_NULL(stmt->type_def.base_def.receive_function);
    ASSERT_NULL(stmt->type_def.base_def.send_function);
    ASSERT_FALSE(stmt->type_def.base_def.has_internallength);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: BASE type with variable internallength */
TEST_CASE(type_integration, parse_base_variable_length) {
    char *sql = "CREATE TYPE vartype ("
                "input = var_in, "
                "output = var_out, "
                "internallength = variable"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_BASE);

    ASSERT_TRUE(stmt->type_def.base_def.has_internallength);
    ASSERT_TRUE(stmt->type_def.base_def.is_variable_length);
    ASSERT_EQ(stmt->type_def.base_def.internallength, -1);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: BASE type with fixed internallength */
TEST_CASE(type_integration, parse_base_fixed_length) {
    char *sql = "CREATE TYPE fixedtype ("
                "input = fixed_in, "
                "output = fixed_out, "
                "internallength = 8"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_TRUE(stmt->type_def.base_def.has_internallength);
    ASSERT_FALSE(stmt->type_def.base_def.is_variable_length);
    ASSERT_EQ(stmt->type_def.base_def.internallength, 8);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: BASE type with passedbyvalue */
TEST_CASE(type_integration, parse_base_passedbyvalue) {
    char *sql = "CREATE TYPE smalltype ("
                "input = small_in, "
                "output = small_out, "
                "internallength = 4, "
                "passedbyvalue"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_TRUE(stmt->type_def.base_def.has_passedbyvalue);
    ASSERT_TRUE(stmt->type_def.base_def.passedbyvalue);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: BASE type with ALIGNMENT */
TEST_CASE(type_integration, parse_base_alignment) {
    char *sql = "CREATE TYPE aligned ("
                "input = aligned_in, "
                "output = aligned_out, "
                "alignment = int4"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_TRUE(stmt->type_def.base_def.has_alignment);
    ASSERT_EQ(stmt->type_def.base_def.alignment, 'i');

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: BASE type with all alignment types */
TEST_CASE(type_integration, parse_base_all_alignments) {
    const char *alignments[] = {"char", "int2", "int4", "double"};
    const char expected[] = {'c', 's', 'i', 'd'};

    for (int i = 0; i < 4; i++) {
        char sql[256];
        snprintf(sql, sizeof(sql),
                 "CREATE TYPE t%d (input = in_fn, output = out_fn, alignment = %s);",
                 i, alignments[i]);

        Parser *parser = parser_create(sql);
        ASSERT_NOT_NULL(parser);

        CreateTypeStmt *stmt = parser_parse_create_type(parser);
        ASSERT_NOT_NULL(stmt);

        ASSERT_TRUE(stmt->type_def.base_def.has_alignment);
        ASSERT_EQ(stmt->type_def.base_def.alignment, expected[i]);

        parser_destroy(parser);
        free_create_type_stmt(stmt);
    }

    TEST_PASS();
}

/* Test: BASE type with STORAGE */
TEST_CASE(type_integration, parse_base_storage) {
    char *sql = "CREATE TYPE mystored ("
                "input = stored_in, "
                "output = stored_out, "
                "storage = EXTERNAL"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_TRUE(stmt->type_def.base_def.has_storage);
    ASSERT_EQ(stmt->type_def.base_def.storage, 'e');

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: BASE type with ELEMENT */
TEST_CASE(type_integration, parse_base_element) {
    char *sql = "CREATE TYPE array_base ("
                "input = arr_in, "
                "output = arr_out, "
                "element = int4, "
                "delimiter = ','"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_NOT_NULL(stmt->type_def.base_def.element_type);
    ASSERT_STR_EQ(stmt->type_def.base_def.element_type, "int4");

    ASSERT_TRUE(stmt->type_def.base_def.has_delimiter);
    ASSERT_EQ(stmt->type_def.base_def.delimiter, ',');

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: BASE type with CATEGORY and preferred */
TEST_CASE(type_integration, parse_base_category_preferred) {
    char *sql = "CREATE TYPE cattype ("
                "input = cat_in, "
                "output = cat_out, "
                "category = 'N', "
                "preferred = true"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_TRUE(stmt->type_def.base_def.has_category);
    ASSERT_EQ(stmt->type_def.base_def.category, 'N');

    ASSERT_TRUE(stmt->type_def.base_def.has_preferred);
    ASSERT_TRUE(stmt->type_def.base_def.preferred);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: BASE type with all optional functions */
TEST_CASE(type_integration, parse_base_all_functions) {
    char *sql = "CREATE TYPE fulltype ("
                "input = full_in, "
                "output = full_out, "
                "receive = full_recv, "
                "send = full_send, "
                "TYPMOD_IN = full_typmod_in, "
                "TYPMOD_OUT = full_typmod_out, "
                "ANALYZE = full_analyze"
                ");";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->type_def.base_def.input_function, "full_in");
    ASSERT_STR_EQ(stmt->type_def.base_def.output_function, "full_out");
    ASSERT_STR_EQ(stmt->type_def.base_def.receive_function, "full_recv");
    ASSERT_STR_EQ(stmt->type_def.base_def.send_function, "full_send");
    ASSERT_STR_EQ(stmt->type_def.base_def.typmod_in_function, "full_typmod_in");
    ASSERT_STR_EQ(stmt->type_def.base_def.typmod_out_function, "full_typmod_out");
    ASSERT_STR_EQ(stmt->type_def.base_def.analyze_function, "full_analyze");

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* ========== Parser Edge Cases ========== */

/* Test: Whitespace handling */
TEST_CASE(type_integration, parse_whitespace) {
    char *sql = "  CREATE   TYPE   mytype   AS   ENUM  (  'a'  ,  'b'  )  ;  ";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->type_name, "mytype");
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 2);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: Mixed case keywords */
TEST_CASE(type_integration, parse_case_insensitive) {
    char *sql = "CrEaTe TyPe MyType As EnUm ('x');";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->variant, TYPE_VARIANT_ENUM);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test: Comments in definition */
TEST_CASE(type_integration, parse_with_comments) {
    char *sql = "CREATE TYPE test AS ENUM ( /* comment */ 'val' -- inline\n);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 1);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

// /* Test: Without trailing semicolon */
// TEST_CASE(type_integration, parse_no_semicolon) {
//     char *sql = "CREATE TYPE simple AS ENUM ('one')";
//     Parser *parser = parser_create(sql);
//     ASSERT_NOT_NULL(parser);

//     CreateTypeStmt *stmt = parser_parse_create_type(parser);
//     ASSERT_NOT_NULL(stmt);
//     ASSERT_EQ(stmt->type_def.enum_def.label_count, 1);

//     parser_destroy(parser);
//     free_create_type_stmt(stmt);
//     TEST_PASS();
// }

/* Test: Newlines in definition */
TEST_CASE(type_integration, parse_multiline) {
    char *sql = "CREATE TYPE multi\nAS ENUM\n(\n'a',\n'b'\n);";
    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTypeStmt *stmt = parser_parse_create_type(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type_def.enum_def.label_count, 2);

    parser_destroy(parser);
    free_create_type_stmt(stmt);
    TEST_PASS();
}

/* Test suite definition */
static TestCase type_integration_tests[] = {
    /* ENUM tests */
    {"parse_enum_basic", test_type_integration_parse_enum_basic, "type_integration"},
    {"parse_enum_single_label", test_type_integration_parse_enum_single_label, "type_integration"},
    {"parse_enum_many_labels", test_type_integration_parse_enum_many_labels, "type_integration"},
    {"parse_enum_special_chars", test_type_integration_parse_enum_special_chars, "type_integration"},
    {"parse_enum_schema_qualified", test_type_integration_parse_enum_schema_qualified, "type_integration"},
    {"parse_enum_case_sensitive", test_type_integration_parse_enum_case_sensitive, "type_integration"},
    {"parse_enum_mpaa_rating", test_type_integration_parse_enum_mpaa_rating, "type_integration"},
    /* COMPOSITE tests */
    {"parse_composite_basic", test_type_integration_parse_composite_basic, "type_integration"},
    {"parse_composite_with_collation", test_type_integration_parse_composite_with_collation, "type_integration"},
    {"parse_composite_single_attr", test_type_integration_parse_composite_single_attr, "type_integration"},
    {"parse_composite_many_attrs", test_type_integration_parse_composite_many_attrs, "type_integration"},
    {"parse_composite_array_types", test_type_integration_parse_composite_array_types, "type_integration"},
    {"parse_composite_precision", test_type_integration_parse_composite_precision, "type_integration"},
    /* RANGE tests */
    {"parse_range_minimal", test_type_integration_parse_range_minimal, "type_integration"},
    {"parse_range_full", test_type_integration_parse_range_full, "type_integration"},
    {"parse_range_multirange", test_type_integration_parse_range_multirange, "type_integration"},
    {"parse_range_timestamp", test_type_integration_parse_range_timestamp, "type_integration"},
    /* BASE tests */
    {"parse_base_minimal", test_type_integration_parse_base_minimal, "type_integration"},
    {"parse_base_variable_length", test_type_integration_parse_base_variable_length, "type_integration"},
    {"parse_base_fixed_length", test_type_integration_parse_base_fixed_length, "type_integration"},
    {"parse_base_passedbyvalue", test_type_integration_parse_base_passedbyvalue, "type_integration"},
    {"parse_base_alignment", test_type_integration_parse_base_alignment, "type_integration"},
    {"parse_base_all_alignments", test_type_integration_parse_base_all_alignments, "type_integration"},
    {"parse_base_storage", test_type_integration_parse_base_storage, "type_integration"},
    {"parse_base_element", test_type_integration_parse_base_element, "type_integration"},
    {"parse_base_category_preferred", test_type_integration_parse_base_category_preferred, "type_integration"},
    {"parse_base_all_functions", test_type_integration_parse_base_all_functions, "type_integration"},
    /* Parser edge cases */
    {"parse_whitespace", test_type_integration_parse_whitespace, "type_integration"},
    {"parse_case_insensitive", test_type_integration_parse_case_insensitive, "type_integration"},
    {"parse_with_comments", test_type_integration_parse_with_comments, "type_integration"},
    // {"parse_no_semicolon", test_type_integration_parse_no_semicolon, "type_integration"},
    {"parse_multiline", test_type_integration_parse_multiline, "type_integration"},
};

void run_type_integration_tests(void) {
    run_test_suite("type_integration", NULL, NULL, type_integration_tests,
                   sizeof(type_integration_tests) / sizeof(type_integration_tests[0]));
}
