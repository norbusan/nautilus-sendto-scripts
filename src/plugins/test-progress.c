#include "nautilus-sendto-progress.h"

int main (int argc, char **argv)
{
	GtkWidget *progress;
	GtkWidget *window;

	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	progress = nst_progress_bar_new ();
	nst_progress_bar_set_label (NST_PROGRESS_BAR (progress), "TEST TEST TEST TEST AGAIN");
	gtk_container_add (GTK_CONTAINER (window), progress);

	gtk_widget_show_all (window);
	gtk_main ();

	return 0;
}
