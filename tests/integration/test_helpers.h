#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "pg_create_table.h"
#include <stdbool.h>
#include <string.h>

/**
 * Find a column by name in the table elements linked list.
 * Returns NULL if not found.
 */
__attribute__((unused))
static ColumnDef* find_column(TableElement *elements, const char *column_name) {
    TableElement *current = elements;
    while (current != NULL) {
        if (current->type == TABLE_ELEM_COLUMN) {
            if (strcmp(current->elem.column.column_name, column_name) == 0) {
                return &current->elem.column;
            }
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Find a column constraint of a specific type in a column's constraint list.
 * Returns NULL if not found.
 */
__attribute__((unused))
static ColumnConstraint* find_column_constraint(ColumnConstraint *constraints, ConstraintType type) {
    ColumnConstraint *current = constraints;
    while (current != NULL) {
        if (current->type == type) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Find a table constraint by name in the table elements linked list.
 * Returns NULL if not found.
 */
__attribute__((unused))
static TableConstraint* find_table_constraint(TableElement *elements, const char *constraint_name) {
    TableElement *current = elements;
    while (current != NULL) {
        if (current->type == TABLE_ELEM_TABLE_CONSTRAINT) {
            TableConstraint *tc = current->elem.table_constraint;
            if (tc->constraint_name && strcmp(tc->constraint_name, constraint_name) == 0) {
                return tc;
            }
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Count total number of columns in the table.
 */
__attribute__((unused))
static int count_columns(TableElement *elements) {
    int count = 0;
    TableElement *current = elements;
    while (current != NULL) {
        if (current->type == TABLE_ELEM_COLUMN) {
            count++;
        }
        current = current->next;
    }
    return count;
}

/**
 * Check if a column has a specific constraint type.
 */
__attribute__((unused))
static bool column_has_constraint(ColumnDef *col, ConstraintType type) {
    return find_column_constraint(col->constraints, type) != NULL;
}

#endif /* TEST_HELPERS_H */
