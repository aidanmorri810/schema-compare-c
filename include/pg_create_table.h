#ifndef PG_CREATE_TABLE_H
#define PG_CREATE_TABLE_H

#include <stdbool.h>

/* Forward declarations */
typedef struct ColumnConstraint ColumnConstraint;
typedef struct TableConstraint TableConstraint;

/* Enumerations */

typedef enum {
    TABLE_TYPE_NORMAL,
    TABLE_TYPE_TEMPORARY,
    TABLE_TYPE_TEMP,
    TABLE_TYPE_UNLOGGED
} TableType;

typedef enum {
    TEMP_SCOPE_NONE,
    TEMP_SCOPE_GLOBAL,
    TEMP_SCOPE_LOCAL
} TempScope;

typedef enum {
    STORAGE_TYPE_PLAIN,
    STORAGE_TYPE_EXTERNAL,
    STORAGE_TYPE_EXTENDED,
    STORAGE_TYPE_MAIN,
    STORAGE_TYPE_DEFAULT
} StorageType;

typedef enum {
    PARTITION_TYPE_NONE,
    PARTITION_TYPE_RANGE,
    PARTITION_TYPE_LIST,
    PARTITION_TYPE_HASH
} PartitionType;

typedef enum {
    ON_COMMIT_PRESERVE_ROWS,
    ON_COMMIT_DELETE_ROWS,
    ON_COMMIT_DROP
} OnCommitAction;

typedef enum {
    CONSTRAINT_NOT_NULL,
    CONSTRAINT_NULL,
    CONSTRAINT_CHECK,
    CONSTRAINT_DEFAULT,
    CONSTRAINT_GENERATED_ALWAYS,
    CONSTRAINT_GENERATED_IDENTITY,
    CONSTRAINT_UNIQUE,
    CONSTRAINT_PRIMARY_KEY,
    CONSTRAINT_REFERENCES
} ConstraintType;

typedef enum {
    GENERATED_STORED,
    GENERATED_VIRTUAL
} GeneratedStorage;

typedef enum {
    IDENTITY_ALWAYS,
    IDENTITY_BY_DEFAULT
} IdentityType;

typedef enum {
    MATCH_FULL,
    MATCH_PARTIAL,
    MATCH_SIMPLE
} MatchType;

typedef enum {
    REF_ACTION_NO_ACTION,
    REF_ACTION_RESTRICT,
    REF_ACTION_CASCADE,
    REF_ACTION_SET_NULL,
    REF_ACTION_SET_DEFAULT
} ReferentialAction;

typedef enum {
    TABLE_CONSTRAINT_CHECK,
    TABLE_CONSTRAINT_NOT_NULL,
    TABLE_CONSTRAINT_UNIQUE,
    TABLE_CONSTRAINT_PRIMARY_KEY,
    TABLE_CONSTRAINT_EXCLUDE,
    TABLE_CONSTRAINT_FOREIGN_KEY
} TableConstraintType;

typedef enum {
    LIKE_OPT_COMMENTS,
    LIKE_OPT_COMPRESSION,
    LIKE_OPT_CONSTRAINTS,
    LIKE_OPT_DEFAULTS,
    LIKE_OPT_GENERATED,
    LIKE_OPT_IDENTITY,
    LIKE_OPT_INDEXES,
    LIKE_OPT_STATISTICS,
    LIKE_OPT_STORAGE,
    LIKE_OPT_ALL
} LikeOptionType;

typedef enum {
    NULLS_DISTINCT,
    NULLS_NOT_DISTINCT
} NullsDistinct;

typedef enum {
    SORT_ASC,
    SORT_DESC
} SortOrder;

typedef enum {
    NULLS_FIRST,
    NULLS_LAST
} NullsOrder;

/* Basic structures */

typedef struct {
    char *name;
    char *value;
} StorageParameter;

typedef struct {
    int count;
    StorageParameter *parameters;
} StorageParameterList;

typedef struct {
    char *expression;
} Expression;

typedef struct {
    char *name;
} Identifier;

typedef struct {
    int count;
    char **names;
} IdentifierList;

typedef struct {
    char **columns;
    int column_count;
} IncludeClause;

typedef struct {
    IncludeClause *include;
    StorageParameterList *with_options;
    char *tablespace_name;
} IndexParameters;

typedef struct {
    char *opclass;
    StorageParameter *parameters;
    int parameter_count;
} OpclassSpec;

typedef struct {
    char *column_name;
    Expression *expression;
    char *collation;
    OpclassSpec *opclass;
    SortOrder sort_order;
    NullsOrder nulls_order;
    bool has_sort_order;
    bool has_nulls_order;
} ExcludeElement;

typedef struct {
    bool has_increment;
    long increment_by;
    bool has_start;
    long start_with;
    bool has_minvalue;
    bool is_no_minvalue;
    long minvalue;
    bool has_maxvalue;
    bool is_no_maxvalue;
    long maxvalue;
    bool has_cache;
    long cache;
    bool has_cycle;
    bool cycle;
} SequenceOptions;

typedef struct {
    char *column_name;
    Expression *expression;
    char *collation;
    char *opclass;
} PartitionElement;

typedef struct {
    PartitionType type;
    PartitionElement *elements;
    int element_count;
} PartitionByClause;

/* Partition bound specifications */

typedef struct {
    Expression **exprs;
    int expr_count;
} InBound;

typedef struct {
    bool is_minvalue;
    bool is_maxvalue;
    Expression *expr;
} BoundValue;

typedef struct {
    BoundValue *from_values;
    int from_count;
    BoundValue *to_values;
    int to_count;
} RangeBound;

typedef struct {
    long modulus;
    long remainder;
} HashBound;

typedef enum {
    BOUND_TYPE_IN,
    BOUND_TYPE_RANGE,
    BOUND_TYPE_HASH,
    BOUND_TYPE_DEFAULT
} BoundType;

typedef struct {
    BoundType type;
    union {
        InBound in_bound;
        RangeBound range_bound;
        HashBound hash_bound;
    } spec;
} PartitionBoundSpec;

/* Like clause */

typedef struct {
    LikeOptionType option;
    bool including;
} LikeOption;

typedef struct {
    char *source_table;
    LikeOption *options;
    int option_count;
} LikeClause;

/* Column constraint structures */

typedef struct {
    bool no_inherit;
} NotNullConstraint;

typedef struct {
    Expression *expr;
    bool no_inherit;
} CheckConstraint;

typedef struct {
    Expression *expr;
} DefaultConstraint;

typedef struct {
    Expression *expr;
    GeneratedStorage storage;
    bool has_storage;
} GeneratedAlwaysConstraint;

typedef struct {
    IdentityType type;
    SequenceOptions *sequence_opts;
} GeneratedIdentityConstraint;

typedef struct {
    NullsDistinct nulls_distinct;
    bool has_nulls_distinct;
    IndexParameters *index_params;
} UniqueConstraint;

typedef struct {
    IndexParameters *index_params;
} PrimaryKeyConstraint;

typedef struct {
    char *reftable;
    char *refcolumn;
    MatchType match_type;
    bool has_match_type;
    ReferentialAction on_delete;
    bool has_on_delete;
    ReferentialAction on_update;
    bool has_on_update;
} ReferencesConstraint;

struct ColumnConstraint {
    char *constraint_name;
    ConstraintType type;
    union {
        NotNullConstraint not_null;
        CheckConstraint check;
        DefaultConstraint default_val;
        GeneratedAlwaysConstraint generated_always;
        GeneratedIdentityConstraint generated_identity;
        UniqueConstraint unique;
        PrimaryKeyConstraint primary_key;
        ReferencesConstraint references;
    } constraint;
    bool deferrable;
    bool not_deferrable;
    bool initially_deferred;
    bool initially_immediate;
    bool enforced;
    bool not_enforced;
    bool has_deferrable;
    bool has_initially;
    bool has_enforced;
    ColumnConstraint *next;
};

/* Table constraint structures */

typedef struct {
    char *column_name;
    bool no_inherit;
} TableNotNullConstraint;

typedef struct {
    char **columns;
    int column_count;
    char *without_overlaps_column;
    NullsDistinct nulls_distinct;
    bool has_nulls_distinct;
    IndexParameters *index_params;
} TableUniqueConstraint;

typedef struct {
    char **columns;
    int column_count;
    char *without_overlaps_column;
    IndexParameters *index_params;
} TablePrimaryKeyConstraint;

typedef struct {
    char *index_method;
    ExcludeElement *elements;
    int element_count;
    char **operators;
    IndexParameters *index_params;
    Expression *where_predicate;
} ExcludeConstraint;

typedef struct {
    char **columns;
    int column_count;
    char *period_column;
    char *reftable;
    char **refcolumns;
    int refcolumn_count;
    char *ref_period_column;
    MatchType match_type;
    bool has_match_type;
    ReferentialAction on_delete;
    bool has_on_delete;
    ReferentialAction on_update;
    bool has_on_update;
    char **on_delete_columns;
    int on_delete_column_count;
    char **on_update_columns;
    int on_update_column_count;
} ForeignKeyConstraint;

struct TableConstraint {
    char *constraint_name;
    TableConstraintType type;
    union {
        CheckConstraint check;
        TableNotNullConstraint not_null;
        TableUniqueConstraint unique;
        TablePrimaryKeyConstraint primary_key;
        ExcludeConstraint exclude;
        ForeignKeyConstraint foreign_key;
    } constraint;
    bool deferrable;
    bool not_deferrable;
    bool initially_deferred;
    bool initially_immediate;
    bool enforced;
    bool not_enforced;
    bool has_deferrable;
    bool has_initially;
    bool has_enforced;
    TableConstraint *next;
};

/* Column definition */

typedef struct {
    char *column_name;
    char *data_type;
    StorageType storage_type;
    bool has_storage;
    char *compression_method;
    char *collation;
    ColumnConstraint *constraints;
} ColumnDef;

/* Table element (can be column, constraint, or LIKE) */

typedef enum {
    TABLE_ELEM_COLUMN,
    TABLE_ELEM_TABLE_CONSTRAINT,
    TABLE_ELEM_LIKE
} TableElementType;

typedef struct TableElement {
    TableElementType type;
    union {
        ColumnDef column;
        TableConstraint *table_constraint;
        LikeClause like;
    } elem;
    struct TableElement *next;
} TableElement;

/* Typed table column definition */

typedef struct {
    char *column_name;
    bool with_options;
    ColumnConstraint *constraints;
} TypedColumnDef;

/* Typed table element */

typedef enum {
    TYPED_ELEM_COLUMN,
    TYPED_ELEM_TABLE_CONSTRAINT
} TypedTableElementType;

typedef struct TypedTableElement {
    TypedTableElementType type;
    union {
        TypedColumnDef column;
        TableConstraint *table_constraint;
    } elem;
    struct TypedTableElement *next;
} TypedTableElement;

/* Partition table column definition (same as typed) */
typedef TypedColumnDef PartitionColumnDef;
typedef TypedTableElement PartitionTableElement;

/* Main CREATE TABLE structure */

typedef enum {
    CREATE_TABLE_REGULAR,
    CREATE_TABLE_OF_TYPE,
    CREATE_TABLE_PARTITION
} CreateTableVariant;

typedef struct {
    TableElement *elements;
    char **inherits;
    int inherits_count;
} RegularTableDef;

typedef struct {
    char *type_name;
    TypedTableElement *elements;
} OfTypeTableDef;

typedef struct {
    char *parent_table;
    PartitionTableElement *elements;
    PartitionBoundSpec *bound_spec;
    bool is_default;
} PartitionTableDef;

typedef struct {
    TempScope temp_scope;
    TableType table_type;
    bool if_not_exists;
    char *table_name;
    CreateTableVariant variant;
    union {
        RegularTableDef regular;
        OfTypeTableDef of_type;
        PartitionTableDef partition;
    } table_def;
    PartitionByClause *partition_by;
    char *using_method;
    StorageParameterList *with_options;
    bool without_oids;
    OnCommitAction on_commit;
    bool has_on_commit;
    char *tablespace_name;
} CreateTableStmt;

#endif /* PG_CREATE_TABLE_H */
