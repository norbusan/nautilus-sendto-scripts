
#include "config.h"

#include <locale.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "nautilus-sendto-mimetype.h"

static void
test_name (void)
{
	guint i, num_lines;
	char *contents, **lines;

	char *bug, *result;
	int num_files, num_dirs;
	GPtrArray *types;

	if (g_file_get_contents (TEST_SRCDIR "test-mimetype-data", &contents, NULL, NULL) == FALSE)
		g_error ("Failed to open test-mimetype-data file");

	lines = g_strsplit (contents, "\n", -1);
	num_lines = g_strv_length (lines);
	g_free (contents);

	num_files = -1;
	num_dirs = -1;
	bug = NULL;
	result = NULL;
	types = g_ptr_array_new ();

	for (i = 0; i < num_lines; i++) {
		char **mimetypes;

		if (*lines[i] == '#')
			continue;
		if (*lines[i] == '\0') {
			continue;
		}
		if (bug == NULL) {
			bug = lines[i];
			continue;
		}
		if (strstr (lines[i], "/") != NULL) {
			g_ptr_array_add (types, lines[i]);
			continue;
		}
		if (num_files == -1) {
			num_files = (int) strtod (lines[i], NULL);
			continue;
		}
		if (num_dirs == -1) {
			num_dirs = (int) strtod (lines[i], NULL);
			continue;
		}
		result = lines[i];

		g_ptr_array_add (types, NULL);
		mimetypes = (char **) g_ptr_array_free (types, FALSE);

		g_test_bug (bug);
		g_assert_cmpstr (nst_title_from_mime_types ((const char **) mimetypes, num_files, num_dirs), ==, result);

		num_files = -1;
		num_dirs = -1;
		bug = NULL;
		result = NULL;
		types = g_ptr_array_new ();
	}
}

int main (int argc, char **argv)
{
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	setlocale (LC_ALL, "en_US.UTF-8");

	g_type_init ();
	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

	g_test_add_func ("/mimetype/name", test_name);

	return g_test_run ();
}
