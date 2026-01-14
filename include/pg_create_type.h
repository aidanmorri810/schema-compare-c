#ifndef PG_CREATE_TYPE_H
#define PG_CREATE_TYPE_H

#include <stdbool.h>

/* Type variant enumeration */
typedef enum {
    TYPE_VARIANT_ENUM,
    TYPE_VARIANT_COMPOSITE,
    TYPE_VARIANT_RANGE,
    TYPE_VARIANT_BASE
} TypeVariant;

/* ENUM Type Definition */
typedef struct {
    char **labels;                // Array of enum labels
    int label_count;              // Number of labels
} EnumTypeDef;

/* COMPOSITE Type Definition */
typedef struct CompositeAttribute {
    char *attr_name;
    char *data_type;
    char *collation;              // Optional COLLATE clause
    struct CompositeAttribute *next;
} CompositeAttribute;

typedef struct {
    CompositeAttribute *attributes;  // Linked list of attributes
    int attribute_count;
} CompositeTypeDef;

/* RANGE Type Definition */
typedef struct {
    char *subtype;                // Required: SUBTYPE parameter
    char *subtype_opclass;        // Optional: SUBTYPE_OPCLASS
    char *collation;              // Optional: COLLATION
    char *canonical_function;     // Optional: CANONICAL
    char *subtype_diff_function;  // Optional: SUBTYPE_DIFF
    char *multirange_type_name;   // Optional: MULTIRANGE_TYPE_NAME (PG14+)
} RangeTypeDef;

/* BASE Type Definition */
typedef struct {
    char *input_function;         // Required: INPUT
    char *output_function;        // Required: OUTPUT
    char *receive_function;       // Optional: RECEIVE
    char *send_function;          // Optional: SEND
    char *typmod_in_function;     // Optional: TYPMOD_IN
    char *typmod_out_function;    // Optional: TYPMOD_OUT
    char *analyze_function;       // Optional: ANALYZE

    int internallength;           // Optional: INTERNALLENGTH (-1 for VARIABLE)
    bool is_variable_length;
    bool has_internallength;
    bool passedbyvalue;           // Optional: PASSEDBYVALUE
    bool has_passedbyvalue;
    char alignment;               // Optional: ALIGNMENT (c=char, s=int2, i=int4, d=double)
    bool has_alignment;
    char storage;                 // Optional: STORAGE (p=plain, e=external, x=extended, m=main)
    bool has_storage;
    char *like_type;              // Optional: LIKE
    char category;                // Optional: CATEGORY (single char)
    bool has_category;
    bool preferred;               // Optional: PREFERRED
    bool has_preferred;
    char *default_value;          // Optional: DEFAULT
    char *element_type;           // Optional: ELEMENT
    char delimiter;               // Optional: DELIMITER
    bool has_delimiter;
    char *collatable;             // Optional: COLLATABLE (true/false as string)
} BaseTypeDef;

/* Main CREATE TYPE structure */
typedef struct CreateTypeStmt {
    char *type_name;              // Type name (may be schema-qualified)
    TypeVariant variant;
    bool if_not_exists;           // For future PostgreSQL versions

    union {
        EnumTypeDef enum_def;
        CompositeTypeDef composite_def;
        RangeTypeDef range_def;
        BaseTypeDef base_def;
    } type_def;
} CreateTypeStmt;

#endif /* PG_CREATE_TYPE_H */
