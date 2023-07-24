#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#define AITHRESHOLD_VERSION "0.20130725"

void aithreshold_usage(char* prog, int npart, float tscale, int delta, int wed, float ked)
{
    printf("Adaptive Integral Threshold.\n");
    printf("version %s.\n", AITHRESHOLD_VERSION);
    printf("usage: %s [options] image_in out.png\n", prog);
    printf("options:\n");
    printf("  -d NUM    delta local threshold (default %d)\n", delta);
    printf("  -e N.N    edgediv factor local threshold (default %f)\n", ked);
    printf("  -p NUM    count parts (default %d)\n", npart);
    printf("  -s N.N    scale factor local threshold (default %f)\n", tscale);
    printf("  -w NUM    window size for edgediv (default %d)\n", wed);
    printf("  -h        show this help message and exit\n");
}

void aithreshold(unsigned char *data, size_t *dataint, int height, int width, int npart, float tscale, int delta, int wed, float ked)
{
    unsigned int wy, wx, ry, rx, y, x, y1, x1, y2, x2, y1w, x1w, y2w, x2w, count, countw;
    int pix, thres;
    size_t k, k11, k12, k21, k22, kw11, kw12, kw21, kw22, sum, sumw;
    float meant, meanw, edge, edgeplus, edgeinv, edgenorm, retval;

    wy = (height > (npart + npart)) ? (height / npart) : 2;
    wx = (width > (npart + npart)) ? (width / npart) : 2;
    ry = wy / 2;
    rx = wx / 2;

    k = 0;
    for (y = 0; y < height; y++)
    {
        y1 = (y < ry) ? 0 : (y - ry);
        y2 = ((y + ry) < height) ? (y + ry) : (height - 1);
        y1w = (y < wed) ? 0 : (y - wed);
        y2w = ((y + wed) < height) ? (y + wed) : (height - 1);
        for (x = 0; x < width; x++)
        {
            x1 = (x < rx) ? 0 : (x - rx);
            x2 = ((x + rx) < width) ? (x + rx) : (width - 1);
            count = (x2 - x1) * (y2 - y1);
            k11 = y1 * width + x1;
            k12 = y1 * width + x2;
            k21 = y2 * width + x1;
            k22 = y2 * width + x2;
            sum = dataint[k22] + dataint[k11] - dataint[k12] - dataint[k21];
            pix = (int) data[k];
            meant = (float) sum / count;
            thres = (int) (meant * (1.0f - tscale) + delta);
            if (ked > 0.0f)
            {
                // EdgeDiv = {EdgePlus + BlurDiv}
                x1w = (x < wed) ? 0 : (x - wed);
                x2w = ((x + wed) < width) ? (x + wed) : (width - 1);
                countw = (x2w - x1w) * (y2w - y1w);
                kw11 = y1w * width + x1w;
                kw12 = y1w * width + x2w;
                kw21 = y2w * width + x1w;
                kw22 = y2w * width + x2w;
                sumw = dataint[kw22] + dataint[kw11] - dataint[kw12] - dataint[kw21];
                meanw = (float) sumw / countw;
                retval = (float) pix;

                // EdgePlus
                // edge = I / blur (shift = -0.5) {0.0 .. >1.0}, mean value = 0.5
                edge = (retval + 1.0f) / (meanw + 1) - 0.5f;
                // edgeplus = I * edge, mean value = 0.5 * mean(I)
                edgeplus = retval * edge;
                // return k * edgeplus + (1 - k) * I
                retval = ked * edgeplus + (1.0f - ked) * retval;
                retval = (retval < 0.0f) ? 0.0f : retval;

                // BlurDiv
                // edge = blur / I (shift = -0.5) {0.0 .. >1.0}, mean value = 0.5
                edgeinv = (float) (meanw + 1) / (retval + 1.0f) - 0.5f;
                // edgenorm = edge * k + max * (1 - k), mean value = {0.5 .. 1.0} * mean(I)
                edgenorm = ked * edgeinv + (1.0f - ked);
                // return I / edgenorm
                retval = (edgenorm > 0.0f) ? (retval / edgenorm) : retval;

                pix = (int) (retval + 0.5f);
                // trim value {0..255}
                pix = (pix < 0) ? 0 : (pix < 255) ? pix : 255;
                // EdgeDiv end
            }
            data[k] = (pix < thres) ? 0 : 255;
            k++;
        }
    }
}

int main(int argc, char **argv)
{
    int height, width, channels, y, x, d, s;
    int npart = 8;
    float tscale = 0.15f;
    int delta = 0;
    int wed = 20;
    float ked = 0.0f;
    int fhelp = 0;
    int opt;
    size_t ki, kd, sums;
    unsigned char *data = NULL;
    stbi_uc *img = NULL;
    size_t *dataint = NULL;

    while ((opt = getopt(argc, argv, ":d:e:p:s:w:h")) != -1)
    {
        switch(opt)
        {
        case 'd':
            delta = atoi(optarg);
            break;
        case 'e':
            ked = atof(optarg);
            break;
        case 'p':
            npart = atoi(optarg);
            break;
        case 's':
            tscale = atof(optarg);
            break;
        case 'w':
            wed = atoi(optarg);
            wed = (wed > 1) ? wed : 2;
            break;
        case 'h':
            fhelp = 1;
            break;
        case ':':
            fprintf(stderr, "ERROR: option needs a value\n");
            return 2;
            break;
        case '?':
            fprintf(stderr, "ERROR: unknown option: %c\n", optopt);
            return 3;
            break;
        }
    }
    if(optind + 2 > argc || fhelp)
    {
        aithreshold_usage(argv[0], npart, tscale, delta, wed, ked);
        return 0;
    }
    const char *src_name = argv[optind];
    const char *dst_name = argv[optind + 1];


    printf("Load: %s\n", src_name);
    if (!(img = stbi_load(src_name, &width, &height, &channels, STBI_rgb_alpha)))
    {
        fprintf(stderr, "ERROR: not read image: %s\n", src_name);
        return 1;
    }
    printf("image: %dx%d:%d\n", width, height, channels);
    if (!(data = (unsigned char*) malloc(height * width * sizeof(unsigned char))))
    {
        fprintf(stderr, "ERROR: not use memmory\n");
        return 1;
    }
    if (!(dataint = (size_t*) malloc(height * width * sizeof(size_t))))
    {
        fprintf(stderr, "ERROR: not use memmory\n");
        return 1;
    }
    ki = 0;
    kd = 0;
    sums = 0;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            s = 0;
            for (d = 0; d < channels; d++)
            {
                s += (int)img[ki + d];
            }
            s /= channels;
            sums = (size_t) s;
            if (x > 0)
            {
                sums += dataint[kd - 1];
            }
            if (y > 0)
            {
                sums += dataint[kd - width];
            }
            if ((x > 0) && (y > 0))
            {
                sums -= dataint[kd - width - 1];
            }
            data[kd] = (unsigned char) s;
            dataint[kd] = sums;
            ki += STBI_rgb_alpha;
            kd++;
        }
    }
    stbi_image_free(img);

    printf("part: %d\n", npart);
    printf("scale: %f\n", tscale);
    printf("delta: %d\n", delta);
    printf("window: %d\n", wed);
    printf("edge: %f\n", ked);

    aithreshold(data, dataint, height, width, npart, tscale, delta, wed, ked);

    printf("Save png: %s\n", dst_name);
    if (!(stbi_write_png(dst_name, width, height, 1, data, width)))
    {
        fprintf(stderr, "ERROR: not write image: %s\n", dst_name);
        return 1;
    }

    free(dataint);
    free(data);
    return 0;
}
