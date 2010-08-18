
#include "config.h"

#include <locale.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "nautilus-sendto-mimetype.h"

typedef struct {
	const char **mimetypes;
	guint num_files;
	const char *result;
} TitleTests;

static TitleTests titles[] = {
	{
		{ "application/octet-stream", NULL },
		1,
		"Sharing one file",
	},
};

static void
test_name (void)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS (titles); i++) {
		g_assert_cmpstr (nst_title_from_mime_types (titles[i].mimetypes, titles[i].num_files), ==, titles[i].result);
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
