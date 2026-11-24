/*
 * Build the lightbox preview page and the contact sheet for the hall scan.
 *
 * Both linear images are made by the same vips_thumbnail() linear-light
 * shrink: same loader rules, same scRGB working space, same resizer. The
 * only difference is the target size, so the two sheets cannot drift onto
 * different colour pipelines. Neither sheet is a plain resize of the other.
 *
 * The two catalogue images are made without @linear. They match the existing
 * print-archive sheets, so they are deliberately kept on the ordinary
 * sRGB path.
 *
 * The page is written last, after the PNGs have been closed, and carries the
 * elapsed time of the 720 x 480 linear run. The <img> points at the file that
 * run wrote, so the page, the file and the millisecond figure always agree.
 *
 *   lightbox-preview INPUT OUTDIR
 *
 * writes:
 *   OUTDIR/preview-720x480.png    linear light, 720 x 480
 *   OUTDIR/contact-360x240.png    linear light, 360 x 240
 *   OUTDIR/catalog-720x480.png    sRGB, 720 x 480
 *   OUTDIR/catalog-360x240.png    sRGB, 360 x 240
 *   OUTDIR/index.html
 */

#include <vips/vips.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREVIEW_WIDTH 720
#define PREVIEW_HEIGHT 480
#define CONTACT_WIDTH 360
#define CONTACT_HEIGHT 240

/* Write @in to @path, then release it. Returns 0 on success.
 */
static int
save_and_free(VipsImage *in, const char *path)
{
	int result = vips_image_write_to_file(in, path, NULL);

	g_object_unref(in);

	return result;
}

/* Make a linear-light sheet straight from the scan and write it to @path.
 */
static int
write_linear(const char *input, const char *path, int width, int height)
{
	VipsImage *im;

	if (vips_thumbnail(input, &im, width,
			"height", height,
			"linear", TRUE,
			NULL))
		return -1;

	return save_and_free(im, path);
}

/* Make an ordinary sRGB sheet and write it to @path.
 */
static int
write_plain(const char *input, const char *path, int width, int height)
{
	VipsImage *im;

	if (vips_thumbnail(input, &im, width,
			"height", height,
			NULL))
		return -1;

	return save_and_free(im, path);
}

static int
write_page(const char *path, double ms)
{
	FILE *fp = fopen(path, "w");
	if (!fp)
		return -1;

	/* The PNG is already on disk when we get here, so a cache-busting
	 * query keeps a browser from pairing a freshly-written page with a
	 * stale copy of the image.
	 */
	fprintf(fp,
		"<!doctype html>\n"
		"<html lang=\"en\">\n"
		"<head>\n"
		"<meta charset=\"utf-8\">\n"
		"<meta http-equiv=\"cache-control\" content=\"no-cache\">\n"
		"<title>Hall scan &mdash; lightbox preview</title>\n"
		"<style>\n"
		"  body { background: #1d1d1f; color: #f2f2f2;\n"
		"         font: 14px/1.5 system-ui, -apple-system, sans-serif;\n"
		"         margin: 0; padding: 24px; }\n"
		"  h1 { font-size: 15px; font-weight: 600; margin: 0 0 14px; }\n"
		"  figure { margin: 0; }\n"
		"  img { display: block; width: %dpx; height: %dpx;\n"
		"        background: #000; box-shadow: 0 2px 12px #0008; }\n"
		"  figcaption { margin-top: 12px; color: #a1a1a6; }\n"
		"  output { color: #fff; font-variant-numeric: tabular-nums; }\n"
		"</style>\n"
		"</head>\n"
		"<body>\n"
		"<h1>Hall scan &mdash; %d &times; %d, linear light</h1>\n"
		"<figure>\n"
		"<img src=\"preview-%dx%d.png?v=%.0f\" width=\"%d\" height=\"%d\"\n"
		"     alt=\"Linear-light preview of the hall scan\">\n"
		"<figcaption>Linear shrink: <output>%.0f ms</output></figcaption>\n"
		"</figure>\n"
		"</body>\n"
		"</html>\n",
		PREVIEW_WIDTH, PREVIEW_HEIGHT, PREVIEW_WIDTH, PREVIEW_HEIGHT,
		PREVIEW_WIDTH, PREVIEW_HEIGHT, PREVIEW_WIDTH, PREVIEW_HEIGHT,
		ms, PREVIEW_WIDTH, PREVIEW_HEIGHT, ms);

	if (fclose(fp))
		return -1;

	return 0;
}

int
main(int argc, char **argv)
{
	if (VIPS_INIT(argv[0]))
		vips_error_exit(NULL);

	if (argc != 3) {
		fprintf(stderr, "usage: %s INPUT.jpg OUTDIR\n", argv[0]);
		return 1;
	}

	const char *input = argv[1];
	const char *outdir = argv[2];

	char preview_path[4096];
	char contact_path[4096];
	char catalog720_path[4096];
	char catalog360_path[4096];
	char page_path[4096];

	g_snprintf(preview_path, sizeof(preview_path),
		"%s/preview-%dx%d.png", outdir, PREVIEW_WIDTH, PREVIEW_HEIGHT);
	g_snprintf(contact_path, sizeof(contact_path),
		"%s/contact-%dx%d.png", outdir, CONTACT_WIDTH, CONTACT_HEIGHT);
	g_snprintf(catalog720_path, sizeof(catalog720_path),
		"%s/catalog-%dx%d.png", outdir, PREVIEW_WIDTH, PREVIEW_HEIGHT);
	g_snprintf(catalog360_path, sizeof(catalog360_path),
		"%s/catalog-%dx%d.png", outdir, CONTACT_WIDTH, CONTACT_HEIGHT);
	g_snprintf(page_path, sizeof(page_path), "%s/index.html", outdir);

	/* Time the lightbox sheet on its own: this is the figure the page
	 * reports and the run the 2500 ms budget applies to.
	 */
	gint64 start = g_get_monotonic_time();

	if (write_linear(input, preview_path, PREVIEW_WIDTH, PREVIEW_HEIGHT))
		vips_error_exit(NULL);

	gint64 stop = g_get_monotonic_time();
	double preview_ms = (stop - start) / 1000.0;

	/* Same linear shrink, different target size. */
	if (write_linear(input, contact_path, CONTACT_WIDTH, CONTACT_HEIGHT))
		vips_error_exit(NULL);

	/* The print-archive sheets stay on the ordinary sRGB path. */
	if (write_plain(input, catalog720_path, PREVIEW_WIDTH, PREVIEW_HEIGHT) ||
		write_plain(input, catalog360_path, CONTACT_WIDTH, CONTACT_HEIGHT))
		vips_error_exit(NULL);

	/* Last: the page names the PNG already on disk. */
	if (write_page(page_path, preview_ms))
		vips_error_exit(NULL);

	printf("%.0f ms\n", preview_ms);

	vips_shutdown();

	return 0;
}
