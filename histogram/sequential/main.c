#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <png.h>

#include <mpi.h>

/***
 * discover the flags to libpng: libpng-config --cflags --ldflags
 * to compile you must use something as: mpicc -I/opt/local/include/libpng16 -L/opt/local/lib -lpng16 main.c -o main
 * to execute you must use something as: mpirun ./main image.png x1
 */

#define x0 1
#define x1 12500
#define x2 25000
#define x3 50000
#define x4 100000
#define x5 200000
#define x6 300000

/* global variables */
int img_width;
int img_height;
long img_all_values;

/* prototype */
int * load_image(char *, long);
long get_length_problem(char *);
void abort_execution(char *);
void close_file_and_abort_execution(FILE *, char *);

int main(int argc, char **argv) {
    int count, r, g, b, luminosity, len_problem, n_processes, process_id;
    int *rgb_values, histogram[256];
    double start, end, execution_time;

    /* identify the file and the size problem */
    if (argc < 2) abort_execution("error: file not informed");

    if (argv[2] != NULL) {
        len_problem = get_length_problem(argv[2]);
    } else {
        len_problem = x1;
    }

    /***
     * mpi calls
     * used just to call MPI_Wtime function
     */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    /* load image */
    rgb_values = load_image(argv[1], len_problem);

    /* point to identify execution time */
    start = MPI_Wtime();

    /* clean values of the histogram */
    for (count = 0; count < 256; count++)
        histogram[count] = 0;

    /* calculate own histogram */
    for (count = 0; count < img_all_values; count+=3) {
        r = rgb_values[count];
        g = rgb_values[count + 1];
        b = rgb_values[count + 2];

        /* luminosity equation - ref: https://www.cambridgeincolour.com/tutorials/histograms2.htm */
        luminosity = floor(r * .30 + g * .59 + b * .11);
        histogram[luminosity] += 1;
    }

    /* read point to identify execution time */
    end = MPI_Wtime();
    execution_time = (double) end - start;

    /* print execution data */
    printf("threads:%d\n", 1);
    printf("time:%lf\n", execution_time);
    printf("work:%dx\n", len_problem);

    MPI_Finalize();

    return 0;
}

/***
 * load the image on vector with the RGB values
 * @param filename
 * @param len_problem
 * @return
 */
int * load_image(char *filename, long len_problem) {
    FILE *image_file;
    int count, count_2, new_position, h, w, r, g, b;
    int *rgb_values, bytes_per_row;
    long len_image, len_image_expanded;

    png_structp img_structure;
    png_infop img_information;
    png_byte img_color_type;
    png_bytep *row_pointers;

    image_file = fopen(filename, "rb");
    if (!image_file) abort_execution("error: impossible to open the file");

    /* load structures and information */
    img_structure = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!img_structure) abort_execution("error: impossible to load the structure image");

    img_information = png_create_info_struct(img_structure);
    if (!img_information) abort_execution("error: impossible to load the information image");

    if (setjmp(png_jmpbuf(img_structure))) abort_execution("error: impossible to execute the setjmp function");

    /* init */
    png_init_io(img_structure, image_file);
    png_read_info(img_structure, img_information);

    img_width = (int) png_get_image_width(img_structure, img_information);
    img_height = (int) png_get_image_height(img_structure, img_information);
    img_color_type = png_get_color_type(img_structure, img_information);

    if(img_color_type == PNG_COLOR_TYPE_GRAY ||
       img_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(img_structure);

    /* these color_type don't have an alpha channel then fill it with 0xff */
    if(img_color_type == PNG_COLOR_TYPE_RGB ||
       img_color_type == PNG_COLOR_TYPE_GRAY ||
       img_color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(img_structure, 0xFF, PNG_FILLER_AFTER);

    png_read_update_info(img_structure, img_information);

    bytes_per_row = (int) png_get_rowbytes(img_structure, img_information);
    /* identify the data layout */
    if (img_width != bytes_per_row / 4)
        close_file_and_abort_execution(image_file, "error: this image format is not accepted");

    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * img_height);
    for(count = 0; count < img_height; count++)
        row_pointers[count] = (png_byte*) malloc(bytes_per_row);

    png_read_image(img_structure, row_pointers);
    fclose(image_file);

    /* transform in int array */
    /* 3 because rgb values */
    len_image = img_height * img_width * 3;
    len_image_expanded = len_image * len_problem;
    img_all_values = len_image_expanded;
    rgb_values = (int *) malloc(sizeof(int) * img_all_values);
    count = 0;

    for (h = 0; h < img_height; h++) {
        for (w = 0; w < img_width; w++) {
            r = row_pointers[h][w * 4];
            g = row_pointers[h][w * 4 + 1];
            b = row_pointers[h][w * 4 + 2];

            /* expanding the image */
            for (count_2 = 0; count_2 < len_problem; count_2++) {
                new_position = (count_2 * len_image) + count;
                rgb_values[new_position++] = r;
                rgb_values[new_position++] = g;
                rgb_values[new_position] = b;
            }
            count += 3;
        }
    }

    return rgb_values;
}

/***
 * get the size of image
 * @param problem
 * @return
 */
long get_length_problem(char *problem) {
    if (!strcmp(problem, "x1")) {
        return x1;
    } else if (!strcmp(problem, "x2")) {
        return x2;
    } else if (!strcmp(problem, "x3")) {
        return x3;
    } else if (!strcmp(problem, "x4")) {
        return x4;
    } else if (!strcmp(problem, "x5")) {
        return x5;
    } else if (!strcmp(problem, "x6")) {
        return x6;
    } else {
        return x0;
    }
}

/***
 * abort the execution
 * @param message
 */
void abort_execution(char *message) {
    printf("%s\n", message);
    abort();
}

/***
 * abort the execution
 * @param image_file
 * @param message
 */
void close_file_and_abort_execution(FILE *image_file, char *message) {
    fclose(image_file);

    printf("%s\n", message);
    abort();
}