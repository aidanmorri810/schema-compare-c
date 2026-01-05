#include "report.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Initialize default report options */
ReportOptions *report_options_default(void) {
    ReportOptions *opts = calloc(1, sizeof(ReportOptions));
    if (!opts) {
        return NULL;
    }

    opts->format = REPORT_FORMAT_TEXT;
    opts->verbosity = REPORT_VERBOSITY_NORMAL;
    opts->use_color = true;  /* Detect terminal later */
    opts->show_severity_icons = true;
    opts->diff_style = true;
    opts->group_by_severity = false;
    opts->max_width = 80;
    opts->output_file = NULL;

    return opts;
}

void report_options_free(ReportOptions *opts) {
    free(opts);
}

/* Get severity icon */
const char *severity_icon(DiffSeverity severity) {
    switch (severity) {
        case SEVERITY_CRITICAL: return ICON_CRITICAL;
        case SEVERITY_WARNING:  return ICON_WARNING;
        case SEVERITY_INFO:     return ICON_INFO;
        default:                return " ";
    }
}

/* Get color start code */
const char *severity_color_start(DiffSeverity severity) {
    switch (severity) {
        case SEVERITY_CRITICAL: return ANSI_RED;
        case SEVERITY_WARNING:  return ANSI_YELLOW;
        case SEVERITY_INFO:     return ANSI_CYAN;
        default:                return "";
    }
}

/* Get color end code */
const char *severity_color_end(void) {
    return ANSI_RESET;
}

/* Get diff prefix */
const char *diff_prefix(bool is_added, bool is_removed) {
    if (is_added) {
        return "+ ";
    } else if (is_removed) {
        return "- ";
    }
    return "  ";
}

/* Generate summary section */
char *generate_summary(const SchemaDiff *diff, const ReportOptions *opts) {
    if (!diff) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    /* Title */
    if (opts->use_color) {
        sb_append(sb, ANSI_BOLD);
    }
    sb_append(sb, "Schema Comparison Report\n");
    sb_append(sb, "========================\n");
    if (opts->use_color) {
        sb_append(sb, ANSI_RESET);
    }
    sb_append(sb, "\n");

    /* Summary counts */
    sb_append(sb, "Summary:\n");
    sb_append_fmt(sb, "  Tables Added:    %d\n", diff->tables_added);
    sb_append_fmt(sb, "  Tables Removed:  %d\n", diff->tables_removed);
    sb_append_fmt(sb, "  Tables Modified: %d\n", diff->tables_modified);
    sb_append(sb, "\n");

    /* Severity breakdown */
    if (diff->total_diffs > 0) {
        if (opts->use_color) {
            sb_append_fmt(sb, "  %sCritical Issues:%s %d %s\n",
                         ANSI_RED, ANSI_RESET, diff->critical_count,
                         opts->show_severity_icons ? ICON_CRITICAL : "");
            sb_append_fmt(sb, "  %sWarnings:%s        %d %s\n",
                         ANSI_YELLOW, ANSI_RESET, diff->warning_count,
                         opts->show_severity_icons ? ICON_WARNING : "");
            sb_append_fmt(sb, "  %sInfo:%s            %d %s\n",
                         ANSI_CYAN, ANSI_RESET, diff->info_count,
                         opts->show_severity_icons ? ICON_INFO : "");
        } else {
            sb_append_fmt(sb, "  Critical Issues: %d %s\n", diff->critical_count,
                         opts->show_severity_icons ? ICON_CRITICAL : "");
            sb_append_fmt(sb, "  Warnings:        %d %s\n", diff->warning_count,
                         opts->show_severity_icons ? ICON_WARNING : "");
            sb_append_fmt(sb, "  Info:            %d %s\n", diff->info_count,
                         opts->show_severity_icons ? ICON_INFO : "");
        }
    }

    sb_append(sb, "\n");

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate table diff report */
char *generate_table_diff_report(const TableDiff *td, const ReportOptions *opts) {
    if (!td) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    /* Table header */
    if (opts->use_color) {
        sb_append(sb, ANSI_BOLD);
    }
    sb_append_fmt(sb, "Table: %s\n", td->table_name);
    if (opts->use_color) {
        sb_append(sb, ANSI_RESET);
    }

    /* Handle added/removed tables */
    if (td->table_added) {
        if (opts->use_color) {
            sb_append(sb, ANSI_GREEN);
        }
        sb_append(sb, "  + Table ADDED\n");
        if (opts->use_color) {
            sb_append(sb, ANSI_RESET);
        }
        char *result = sb_to_string(sb);
        sb_free(sb);
        return result;
    }

    if (td->table_removed) {
        if (opts->use_color) {
            sb_append(sb, ANSI_RED);
        }
        sb_append(sb, "  - Table REMOVED\n");
        if (opts->use_color) {
            sb_append(sb, ANSI_RESET);
        }
        char *result = sb_to_string(sb);
        sb_free(sb);
        return result;
    }

    /* Show individual diffs */
    for (Diff *d = td->diffs; d; d = d->next) {
        const char *icon = opts->show_severity_icons ? severity_icon(d->severity) : "";
        const char *color_start = opts->use_color ? severity_color_start(d->severity) : "";
        const char *color_end = opts->use_color ? severity_color_end() : "";

        sb_append_fmt(sb, "  %s%s %s", color_start, icon, diff_type_to_string(d->type));

        if (d->element_name) {
            sb_append_fmt(sb, ": %s", d->element_name);
        }

        if (d->old_value && d->new_value) {
            sb_append_fmt(sb, " (%s → %s)", d->old_value, d->new_value);
        } else if (d->old_value) {
            sb_append_fmt(sb, " (%s)", d->old_value);
        } else if (d->new_value) {
            sb_append_fmt(sb, " (%s)", d->new_value);
        }

        sb_append_fmt(sb, "%s\n", color_end);
    }

    sb_append(sb, "\n");

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate full report */
char *generate_report(const SchemaDiff *diff, const ReportOptions *opts) {
    if (!diff || !opts) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    /* Generate summary */
    char *summary = generate_summary(diff, opts);
    if (summary) {
        sb_append(sb, summary);
        free(summary);
    }

    /* Skip details if verbosity is summary-only */
    if (opts->verbosity == REPORT_VERBOSITY_SUMMARY) {
        char *result = sb_to_string(sb);
        sb_free(sb);
        return result;
    }

    /* Generate table-level diffs */
    if (diff->table_diffs) {
        if (opts->use_color) {
            sb_append(sb, ANSI_BOLD);
        }
        sb_append(sb, "Details:\n");
        sb_append(sb, "========\n\n");
        if (opts->use_color) {
            sb_append(sb, ANSI_RESET);
        }

        for (TableDiff *td = diff->table_diffs; td; td = td->next) {
            char *table_report = generate_table_diff_report(td, opts);
            if (table_report) {
                sb_append(sb, table_report);
                free(table_report);
            }
        }
    }

    /* Footer */
    if (diff->total_diffs == 0 && diff->tables_added == 0 && diff->tables_removed == 0) {
        if (opts->use_color) {
            sb_append_fmt(sb, "%s✓ No differences found%s\n", ANSI_GREEN, ANSI_RESET);
        } else {
            sb_append(sb, "✓ No differences found\n");
        }
    }

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Print report to stdout */
void print_report(const SchemaDiff *diff, const ReportOptions *opts) {
    char *report = generate_report(diff, opts);
    if (report) {
        printf("%s", report);
        free(report);
    }
}

/* Write report to file */
bool write_report_to_file(const SchemaDiff *diff, const char *filename,
                          const ReportOptions *opts) {
    if (!filename) {
        return false;
    }

    char *report = generate_report(diff, opts);
    if (!report) {
        return false;
    }

    bool success = write_string_to_file(filename, report);
    free(report);
    return success;
}
