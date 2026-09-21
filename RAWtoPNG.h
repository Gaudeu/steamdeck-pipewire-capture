#ifndef RAW_TO_PNG_H
#define RAW_TO_PNG_H

#include <stddef.h>
#include <stdint.h>

/** 
 * Converte um buffer BGRx em RGB e salva como arquivo PNG no disco.
 * 
 * "@param bgrx_data" Ponteiro para o buffer bruto BGRx (4 bytes por pixel).
 * "@param width"     Largura em pixels.
 * "@param height  "  Altura em pixels.
 * "@param path"      Caminho de saída para o arquivo .png.
 * "@return 0" em caso de sucesso, ou -1 em caso de falha.
 * 
 * 
*/
int bgrx_to_png_file(const uint8_t *bgrx_data, size_t width, size_t height, const char *path);

#endif /* RAW_TO_PNG_H */