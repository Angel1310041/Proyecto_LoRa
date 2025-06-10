#ifndef TransmisorRF_H

#define TransmisorRF_H
#include "main.h"
extern RCSwitch Transmisorrf;



void enviarRF_Matriz(uint8_t fila, uint8_t columna, int zona, int tipoSensor, int id);


void mostrarImagenPorTipoSensor(int tipoSensor);

#endif