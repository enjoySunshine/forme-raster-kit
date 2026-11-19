/* Benchmark harness: run one pipeline variant on the scan, print wall ms.
 *
 * Usage: bench <mode> <in.jpg> <out.png> <width> <height>
 *
 * Modes:
 *   linear-full     jpegload -> colourspace(scRGB) -> resize -> colourspace(sRGB) -> pngsave
 *   linear-thumb    vips_thumbnail with linear=TRUE
 *   plain-full      jpegload -> resize -> pngsave
 *   plain-thumb     vips_thumbnail (no linear)
 */
#include <vips/vips.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

static long
peak_rss_mb(void)
{
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	return ru.ru_maxrss / 1024;
}

static double
now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

int
main(int argc, char **argv)
{
	if (argc != 6) {
		fprintf(stderr, "usage: %s mode in out width height\n", argv[0]);
		return 1;
	}

	const char *mode = argv[1];
	const char *infile = argv[2];
	const char *outfile = argv[3];
	int width = atoi(argv[4]);
	int height = atoi(argv[5]);

	if (VIPS_INIT(argv[0]))
		vips_error_exit(NULL);

	double t0 = now_ms();

	VipsImage *in;
	VipsImage *out;
	VipsImage *t1;
	VipsImage *t2;

	if (strcmp(mode, "linear-full") == 0) {
		if (!(in = vips_image_new_from_file(infile, NULL)))
			vips_error_exit(NULL);
		if (vips_colourspace(in, &t1, VIPS_INTERPRETATION_scRGB, NULL))
			vips_error_exit(NULL);
		g_object_unref(in);
		double hscale = (double) width / t1->Xsize;
		double vscale = (double) height / t1->Ysize;
		if (vips_resize(t1, &t2, hscale, "vscale", vscale, NULL))
			vips_error_exit(NULL);
		g_object_unref(t1);
		if (vips_colourspace(t2, &out, VIPS_INTERPRETATION_sRGB, NULL))
			vips_error_exit(NULL);
		g_object_unref(t2);
		if (vips_image_write_to_file(out, outfile, NULL))
			vips_error_exit(NULL);
		g_object_unref(out);
	}
	else if (strcmp(mode, "linear-thumb") == 0) {
		if (vips_thumbnail(infile, &out, width,
				"height", height,
				"linear", TRUE,
				NULL))
			vips_error_exit(NULL);
		if (vips_image_write_to_file(out, outfile, NULL))
			vips_error_exit(NULL);
		g_object_unref(out);
	}
	else if (strcmp(mode, "plain-full") == 0) {
		if (!(in = vips_image_new_from_file(infile, NULL)))
			vips_error_exit(NULL);
		double hscale = (double) width / in->Xsize;
		double vscale = (double) height / in->Ysize;
		if (vips_resize(in, &out, hscale, "vscale", vscale, NULL))
			vips_error_exit(NULL);
		g_object_unref(in);
		if (vips_image_write_to_file(out, outfile, NULL))
			vips_error_exit(NULL);
		g_object_unref(out);
	}
	else if (strcmp(mode, "plain-thumb") == 0) {
		if (vips_thumbnail(infile, &out, width,
				"height", height,
				NULL))
			vips_error_exit(NULL);
		if (vips_image_write_to_file(out, outfile, NULL))
			vips_error_exit(NULL);
		g_object_unref(out);
	}
	else {
		fprintf(stderr, "unknown mode %s\n", mode);
		return 1;
	}

	double t1ms = now_ms();
	printf("mode=%s wall=%.0f ms peakrss=%ld MB\n", mode, t1ms - t0, peak_rss_mb());

	vips_shutdown();
	return 0;
}
