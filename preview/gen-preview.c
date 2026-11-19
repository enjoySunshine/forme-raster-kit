/* gen-preview: generate the lightbox preview page assets.
 *
 * One run produces, from samples/hall-scan.jpg:
 *
 *   preview/preview-720x480.png   linear-light 720x480 (lightbox page)
 *   preview/contact-360x240.png   linear-light 360x240 (contact sheet)
 *   preview/catalog-720x480.png   non-linear 720x480 (print archive)
 *   preview/catalog-360x240.png   non-linear 360x240 (print archive)
 *   preview/index.html            the preview page
 *
 * The 720x480 linear preview, its file on disk and the millisecond figure
 * on the page all come from the same single linear downscale in this run.
 *
 * Build (from the repository root):
 *
 *   gcc -O2 -o preview/gen-preview preview/gen-preview.c \
 *     -Ibuild -Ilibvips/include -Ibuild/libvips/include \
 *     $(pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0) \
 *     -Lbuild/libvips -lvips \
 *     $(pkg-config --libs glib-2.0 gobject-2.0 gio-2.0) \
 *     -Wl,-rpath,$PWD/build/libvips
 *
 * Run (from the repository root):
 *
 *   ./preview/gen-preview samples/hall-scan.jpg
 */
#include <vips/vips.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

static double
now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

static long
peak_rss_mb(void)
{
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	return ru.ru_maxrss / 1024;
}

/* One thumbnail run: load @infile, shrink to @width x @height, write
 * @outfile. When @linear is set, the shrink happens in linear light.
 * Returns the wall-clock time of the whole run.
 */
static double
make_one(const char *infile, const char *outfile,
	int width, int height, gboolean linear)
{
	VipsImage *out;
	double t0 = now_ms();

	if (vips_thumbnail(infile, &out, width,
			"height", height,
			"linear", linear,
			NULL))
		vips_error_exit(NULL);
	if (vips_image_write_to_file(out, outfile, NULL))
		vips_error_exit(NULL);
	g_object_unref(out);

	return now_ms() - t0;
}

int
main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s SCAN.jpg\n", argv[0]);
		return 1;
	}
	const char *infile = argv[1];

	if (VIPS_INIT(argv[0]))
		vips_error_exit(NULL);

	/* The lightbox preview comes first: the peak RSS right after this
	 * run is the peak for the linear 720x480 production itself.
	 */
	double ms_lin720 = make_one(infile,
		"preview/preview-720x480.png", 720, 480, TRUE);
	long peak_lin720 = peak_rss_mb();

	double ms_lin360 = make_one(infile,
		"preview/contact-360x240.png", 360, 240, TRUE);
	double ms_cat720 = make_one(infile,
		"preview/catalog-720x480.png", 720, 480, FALSE);
	double ms_cat360 = make_one(infile,
		"preview/catalog-360x240.png", 360, 240, FALSE);

	char html[8192];
	snprintf(html, sizeof(html),
		"<!DOCTYPE html>\n"
		"<html lang=\"zh\">\n"
		"<head>\n"
		"<meta charset=\"utf-8\">\n"
		"<title>灯箱预览 · 第 17 张</title>\n"
		"<style>\n"
		"  body { font-family: sans-serif; margin: 2em; background: #1b1b1b; color: #eee; }\n"
		"  h1 { font-size: 1.4em; }\n"
		"  .ms { font-size: 1.2em; color: #8f8; }\n"
		"  figure { margin: 1.5em 0; }\n"
		"  figcaption { margin-bottom: .5em; color: #bbb; }\n"
		"  img { background: #333; border: 1px solid #444; image-rendering: auto; }\n"
		"  table { border-collapse: collapse; margin-top: 1em; }\n"
		"  td, th { border: 1px solid #444; padding: .3em .8em; }\n"
		"</style>\n"
		"</head>\n"
		"<body>\n"
		"<h1>灯箱预览 · 线性光 720×480</h1>\n"
		"<p class=\"ms\">本次线性出图耗时：%.0f 毫秒</p>\n"
		"<figure>\n"
		"  <figcaption>线性光预览 720×480（preview-720x480.png，与页上毫秒数同一次出图）</figcaption>\n"
		"  <img src=\"preview-720x480.png\" width=\"720\" height=\"480\" alt=\"线性预览 720x480\">\n"
		"</figure>\n"
		"<figure>\n"
		"  <figcaption>联络图 · 同一套线性缩小 360×240（contact-360x240.png，本次 %.0f 毫秒）</figcaption>\n"
		"  <img src=\"contact-360x240.png\" width=\"360\" height=\"240\" alt=\"联络图 360x240\">\n"
		"</figure>\n"
		"<h2>目录图（不走线性，对印刷档案）</h2>\n"
		"<figure>\n"
		"  <figcaption>目录图 720×480（catalog-720x480.png，本次 %.0f 毫秒）</figcaption>\n"
		"  <img src=\"catalog-720x480.png\" width=\"720\" height=\"480\" alt=\"目录图 720x480\">\n"
		"</figure>\n"
		"<figure>\n"
		"  <figcaption>目录图 360×240（catalog-360x240.png，本次 %.0f 毫秒）</figcaption>\n"
		"  <img src=\"catalog-360x240.png\" width=\"360\" height=\"240\" alt=\"目录图 360x240\">\n"
		"</figure>\n"
		"<table>\n"
		"  <tr><th>项目</th><th>尺寸</th><th>线性光</th><th>本次耗时</th></tr>\n"
		"  <tr><td>灯箱预览</td><td>720×480</td><td>是</td><td>%.0f 毫秒</td></tr>\n"
		"  <tr><td>联络图</td><td>360×240</td><td>是</td><td>%.0f 毫秒</td></tr>\n"
		"  <tr><td>目录图</td><td>720×480</td><td>否</td><td>%.0f 毫秒</td></tr>\n"
		"  <tr><td>目录图</td><td>360×240</td><td>否</td><td>%.0f 毫秒</td></tr>\n"
		"</table>\n"
		"<p>线性 720×480 出图进程峰值内存：%ld MB。</p>\n"
		"</body>\n"
		"</html>\n",
		ms_lin720,
		ms_lin360,
		ms_cat720,
		ms_cat360,
		ms_lin720, ms_lin360, ms_cat720, ms_cat360,
		peak_lin720);

	FILE *fp = fopen("preview/index.html", "w");
	if (!fp ||
		fwrite(html, 1, strlen(html), fp) != strlen(html) ||
		fclose(fp) != 0) {
		fprintf(stderr, "unable to write preview/index.html\n");
		return 1;
	}

	printf("linear 720x480: %.0f ms, peak %ld MB\n", ms_lin720, peak_lin720);
	printf("linear 360x240: %.0f ms\n", ms_lin360);
	printf("catalog 720x480: %.0f ms\n", ms_cat720);
	printf("catalog 360x240: %.0f ms\n", ms_cat360);

	vips_shutdown();
	return 0;
}
