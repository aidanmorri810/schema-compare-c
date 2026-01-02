#ifndef REPORT_H
#define REPORT_H

#include "diff.h"
#include <stdbool.h>
#include <stdio.h>

/* Report output format */
typedef enum {
    REPORT_FORMAT_TEXT,      /* Human-readable text */
    REPORT_FORMAT_MARKDOWN,  /* Markdown format */
    REPORT_FORMAT_JSON       /* JSON format (future) */
} ReportFormat;

/* Report verbosity level */
typedef enum {
    REPORT_VERBOSITY_SUMMARY,   /* Only summary counts */
    REPORT_VERBOSITY_NORMAL,    /* Summary + table-level changes */
    REPORT_VERBOSITY_DETAILED,  /* Summary + all details */
    REPORT_VERBOSITY_VERBOSE    /* Everything including unchanged items */
} ReportVerbosity;

/* Report options */
typedef struct {
    ReportFormat format;
    ReportVerbosity verbosity;
    bool use_color;              /* ANSI color codes for terminal */
    bool show_severity_icons;    /* Show ✓, ⚠, ✗ icons */
    bool diff_style;             /* Show - for removed, + for added */
    bool group_by_severity;      /* Group diffs by severity level */
    int max_width;               /* Maximum line width (0 = unlimited) */
    const char *output_file;     /* NULL for stdout */
} ReportOptions;

/* Initialize default report options */
ReportOptions *report_options_default(void);
void report_options_free(ReportOptions *opts);

/* Generate report to string */
char *generate_report(const SchemaDiff *diff, const ReportOptions *opts);

/* Print report to stdout or file */
void print_report(const SchemaDiff *diff, const ReportOptions *opts);
bool write_report_to_file(const SchemaDiff *diff, const char *filename,
                          const ReportOptions *opts);

/* Generate report sections */
char *generate_summary(const SchemaDiff *diff, const ReportOptions *opts);
char *generate_table_diff_report(const TableDiff *diff, const ReportOptions *opts);
char *generate_column_diff_report(const ColumnDiff *diff, const ReportOptions *opts);
char *generate_constraint_diff_report(const ConstraintDiff *diff, const ReportOptions *opts);

/* Formatting utilities */
const char *severity_icon(DiffSeverity severity);
const char *severity_color_start(DiffSeverity severity);
const char *severity_color_end(void);
const char *diff_prefix(bool is_added, bool is_removed);

/* ANSI color codes */
#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_RED        "\033[31m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_GREEN      "\033[32m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_GRAY       "\033[90m"

/* Severity icons */
#define ICON_CRITICAL   "✗"
#define ICON_WARNING    "⚠"
#define ICON_INFO       "✓"

#endif /* REPORT_H */
