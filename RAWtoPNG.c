#include "RAWtoPNG.h"


#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>



static int save_rgb_to_png(const uint8_t *RGB_data, size_t width, size_t height, const char *path){
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        perror("Erro ao abrir arquivo para gravacao do PNG");
        return -1;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr){
        fclose(fp);
        return -1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return -1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return -1;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8,
                 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    
    png_bytep row_pointers[height];
    for (size_t y = 0; y < height; y++){
        row_pointers[y] = (png_bytep)(RGB_data + (y * width * 3));
    }

    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return 0;
}

int bgrx_to_png_file(const uint8_t *BGRX_data, size_t width, size_t height, const char *path)
{
    if(!BGRX_data || !path || width == 0 || height ==0) return -1;

    size_t i, RGB_size;
    const uint8_t *pixel_in;
    uint8_t *pixel_out;
    uint8_t *RGB_data = NULL;
    size_t total_pixels = width * height;

    

    RGB_size = total_pixels * 3;

    RGB_data = malloc(RGB_size);
	if(!RGB_data){
        fprintf(stderr, "Falha ao alocar memoria para RGB\n");
        return -1;
    }

    pixel_in = BGRX_data;
	pixel_out = RGB_data;

    for (i = 0; i < total_pixels; i++) {
		pixel_out[0]  = pixel_in[2];
		pixel_out[1] = pixel_in[1];
		pixel_out[2]   = pixel_in[0];
		pixel_in+=4; // ignore alpha(x)
		pixel_out+=3; // to next pixel
	}

    int result = save_rgb_to_png(RGB_data, width, height, path);
    free(RGB_data);

    return result;


  
}

/*int main(int argc, char **argv){
    size_t i, width, height, BGRX_size, RGB_size;
    int status = 0;
    char *input, *output;
    uint8_t *buffer = NULL;
    const uint8_t *pixel_in;
    uint8_t *pixel_out;
    uint8_t *RGB_data = NULL;
    FILE * fp = NULL;

    if(argc != 5){
    printf("usage: %s <width> <height> <input> <output>\n", argv[0]);
    exit(1);
    }

    width  = (size_t) atoi(argv[1]);
    height = (size_t) atoi(argv[2]);
    input = argv[3];
    output = argv[4];

    BGRX_size = width * height * 4;
    RGB_size = width * height * 3;

    //uint8_t *rgb_data = malloc(rgb_size);

    buffer = malloc(BGRX_size);
    if (!buffer) {
    fprintf(stderr, "Falha ao alocar memoria para entrada BGRx\n");
    status = 1;
    goto free_val;
    }
    
    RGB_data = malloc (RGB_size);
	if(!RGB_data){
        fprintf(stderr, "Falha ao alocar memoria para RGB\n");
        status = 1;
        goto free_val;
    }

	fp = fopen(input, "rb");
	if (!fp) {
		printf ("Error opening file (%d): %s.\n", errno, strerror (errno));
		status = 1;
		goto free_val;
	}

	size_t bytes_read = fread(buffer, 1, BGRX_size, fp);
    if (bytes_read < BGRX_size) {
        printf("Warning: file has less bytes than expected.\n");
    }

    fclose(fp);

	pixel_in = buffer;
	pixel_out = RGB_data;

    size_t total_pixels = width * height;

	for (i = 0; i < total_pixels; i++) {
		pixel_out[0]  = pixel_in[2];
		pixel_out[1] = pixel_in[1];
		pixel_out[2]   = pixel_in[0];
		pixel_in+=4; // ignore alpha
		pixel_out+=3; // to next pixel
	}
	 
    int retval = save_rgb_to_png(RGB_data, width, height, output);
    if (retval != 0) {
        printf("Error on saving PNG\n");
        status = 1;
    } else {
        printf("PNG sucessfully saved! %s\n", output);
    }


free_val:
	if (buffer) free(buffer);
    if (RGB_data) free(RGB_data);
	return status;
}
*/
 
