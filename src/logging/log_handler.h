#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

// Custom log writer function
// Custom log writer function
GLogWriterOutput structured_log_writer(GLogLevelFlags log_level,
                                       const GLogField *fields,
                                       gsize n_fields,
                                       gpointer user_data) {
    const gchar *level_str = "UNKNOWN";
    const gchar *log_domain = "APP";
    const gchar *message = "(no message)";
    const gchar *file = "unknown";
    const gchar *line = "unknown";    
    
    for (gsize i = 0; i < n_fields; i++) {
        if (g_strcmp0(fields[i].key, "MESSAGE") == 0) {
            message = (const gchar *)fields[i].value;
        } else if (g_strcmp0(fields[i].key, "GLIB_DOMAIN") == 0) {
            log_domain = (const gchar *)fields[i].value;
        } else if (g_strcmp0(fields[i].key, "CODE_FILE") == 0) {
            file = (const gchar *)fields[i].value;
        } else if (g_strcmp0(fields[i].key, "CODE_LINE") == 0) {
            line = (const gchar *)fields[i].value;
        }
    }

    switch (log_level & G_LOG_LEVEL_MASK) {
        case G_LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        case G_LOG_LEVEL_CRITICAL: level_str = "CRITICAL"; break;
        case G_LOG_LEVEL_WARNING: level_str = "WARNING"; break;
        case G_LOG_LEVEL_MESSAGE: level_str = "MESSAGE"; break;
        case G_LOG_LEVEL_INFO: level_str = "INFO"; break;
        case G_LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
    }

    fprintf(stderr, "[%s] [%s] %s:%s: %s\n", level_str, log_domain, file, line, message);
    return G_LOG_WRITER_HANDLED;
}

// int main() {
//     // Set custom log writer function
//     g_log_set_writer_func(structured_log_writer, NULL, NULL);
    
//     g_debug("This is a debug message");
//     g_info("This is an info message");
//     g_message("This is a message");
//     g_warning("This is a warning message");
//     g_error("This is an error message");
    
//     return 0;
// }
