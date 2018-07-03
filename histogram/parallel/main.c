#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <png.h>

#include <omp.h>
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

/* prototype */
int * load_image(char *, long, int *, int *, long *);
long get_length_problem(char *);
void abort_execution(char *);
void close_file_and_abort_execution(FILE *, char *);

int main(int argc, char **argv) {
    /* support variables */
    int n_processes, process_id, root_id = 0, n_threads = 1;
    int count, r, g, b, luminosity, n_blocks, displacement = 0, len_problem, img_width, img_height;
    int default_blocks_per_process, max_blocks_per_process, n_extra_blocks;
    int default_values_per_process, max_values_per_process;
    int *rgb_values, *share, *n_elements_per_process, *displs_to_each_process, histogram[256], hist_per_process[256];
    long img_all_values;
    double start, end, execution_time, max_execution_time;

    /* identify the file and the size problem */
    if (argc < 2) abort_execution("error: file not informed");

    if (argc >= 3 && argv[2] != NULL) {
        len_problem = get_length_problem(argv[2]);
    } else {
        len_problem = x1;
    }

    if (argc >= 4 && argv[3] != NULL) {
        n_threads = atoi(argv[3]);
        if (n_threads)
            omp_set_num_threads(n_threads);
    }

    /* mpi calls */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    /* load image */
    if (process_id == root_id) {
        rgb_values = load_image(argv[1], len_problem, &img_width, &img_height, &img_all_values);
    }

    /* share image data */
    MPI_Bcast(&img_width, 1, MPI_INT, root_id, MPI_COMM_WORLD);
    MPI_Bcast(&img_height, 1, MPI_INT, root_id, MPI_COMM_WORLD);
    MPI_Bcast(&img_all_values, 1, MPI_LONG, root_id, MPI_COMM_WORLD);

    /* point to identify execution time */
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    /* clean values of the histogram variables */
    for (count = 0; count < 256; count++) {
        hist_per_process[count] = 0;
        if (process_id == root_id)
            histogram[count] = 0;
    }

    /* specific of the each process */
    /* considering: each rgb block has 3 int values */
    default_blocks_per_process = (img_all_values / 3) / n_processes;
    max_blocks_per_process = default_blocks_per_process + 1;
    n_extra_blocks = (img_all_values / 3) % n_processes;

    default_values_per_process = default_blocks_per_process * 3;
    max_values_per_process = max_blocks_per_process * 3;

    n_elements_per_process = (int *) malloc(sizeof(int) * n_processes);
    displs_to_each_process = (int *) malloc(sizeof(int) * n_processes);
    for (count = 0; count < n_processes; count++) {
        if (count < n_extra_blocks) {
            n_elements_per_process[count] = max_values_per_process;
        } else {
            n_elements_per_process[count] = default_values_per_process;
        }

        /* put and calculate the next displacement */
        displs_to_each_process[count] = displacement;
        displacement += n_elements_per_process[count];
    }

    share = (int *) malloc(sizeof(int) * n_elements_per_process[process_id]);
    MPI_Scatterv(rgb_values, n_elements_per_process, displs_to_each_process, MPI_INT,
                 share, n_elements_per_process[process_id], MPI_INT, root_id, MPI_COMM_WORLD);

    /* considering: each rgb block has 3 int values */
    n_blocks = n_elements_per_process[process_id] / 3;

    /* calculate own histogram */
    #pragma omp parallel private(r, g, b, luminosity)
    {
        #pragma omp for reduction(+:hist_per_process[:256])
        for (count = 0; count < n_blocks; count++) {
            r = share[count * 3];
            g = share[count * 3 + 1];
            b = share[count * 3 + 2];

            /* luminosity equation - ref: https://www.cambridgeincolour.com/tutorials/histograms2.htm */
            luminosity = floor(r * .30 + g * .59 + b * .11);
            hist_per_process[luminosity]++;
        }
    }

    /* final sum calculation */
    MPI_Reduce(hist_per_process, histogram, 256, MPI_INT, MPI_SUM, root_id, MPI_COMM_WORLD);

    /* read point to identify execution time */
    end = MPI_Wtime();
    execution_time = (double) end - start;

    /* execution time calculation */
    MPI_Reduce(&execution_time, &max_execution_time, 1, MPI_DOUBLE, MPI_MAX, root_id, MPI_COMM_WORLD);

    /* print the execution information */
    if (process_id == root_id) {
        /* print execution data */
        printf("threads:%d\n", n_processes * n_threads);
        printf("time:%lf\n", max_execution_time);
        printf("work:%dx\n", len_problem);
    }

    MPI_Finalize();

    return 0;
}

/***
 * load the image on vector with the RGB values
 * @param filename
 * @param len_problem
 * @param image_width
 * @param image_height
 * @param image_all_values
 * @return
 */
int * load_image(char *filename, long len_problem, int *image_width, int *image_height, long *image_all_values) {
    FILE *image_file;
    int count, count_2, new_position, h, w, r, g, b;
    int *rgb_values, bytes_per_row, img_width, img_height;
    long img_all_values, len_image, len_image_expanded;

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

    *image_width = img_width;
    *image_height = img_height;
    *image_all_values = img_all_values;

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