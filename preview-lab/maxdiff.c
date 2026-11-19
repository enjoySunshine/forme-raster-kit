/* maxdiff: print max per-channel absolute difference between two images.
 * Usage: maxdiff a.png b.png
 */
#include <vips/vips.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s a b\n", argv[0]);
		return 1;
	}
	if (VIPS_INIT(argv[0]))
		vips_error_exit(NULL);

	VipsImage *a, *b, *diff, *absdiff;
	double mx;

	if (!(a = vips_image_new_from_file(argv[1], NULL)))
		vips_error_exit(NULL);
	if (!(b = vips_image_new_from_file(argv[2], NULL)))
		vips_error_exit(NULL);

	if (a->Xsize != b->Xsize || a->Ysize != b->Ysize ||
		a->Bands != b->Bands) {
		printf("SIZE-MISMATCH %dx%dx%d vs %dx%dx%d\n",
			a->Xsize, a->Ysize, a->Bands,
			b->Xsize, b->Ysize, b->Bands);
		return 1;
	}

	if (vips_subtract(a, b, &diff, NULL))
		vips_error_exit(NULL);
	if (vips_abs(diff, &absdiff, NULL))
		vips_error_exit(NULL);
	if (vips_max(absdiff, &mx, NULL))
		vips_error_exit(NULL);

	printf("maxdiff=%g\n", mx);
	return 0;
}
