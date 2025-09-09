#ifndef FUNCION1ARCHIVOS_HH
#define FUNCION1ARCHIVOS_HH

#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <cstddef>
#include <fstream>
#include "libs/CONFIGURACION_SENSOR.hh"
#include "libs/ARCHIVO_CONFIGURACION.hh"  

using namespace std;

// ----------------- util -----------------
static size_t split_simple(const std::string& str, char delim, char tokens[][64], size_t maxTok) {
    size_t token_count = 0;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim) && token_count < maxTok) {
        // Trim simple
        size_t a = 0, b = token.size();
        while (a < b && isspace((unsigned char)token[a])) a++;
        while (b > a && isspace((unsigned char)token[b-1])) b--;
        size_t n = b - a;
        if (n == 0) continue;
        if (n >= 64) n = 63;
        strncpy(tokens[token_count], token.c_str()+a, n);
        tokens[token_count][n] = '\0';
        token_count++;
    }
    return token_count;
}

// ----------------- carga config dinámica -----------------
inline void cargarConfiguracion(const char* nombreArchivo, ArchivoConfiguracion& AC) {
    // inicialización mínima
    AC.numReglas = 0;
    if (AC.capacidad <= 0) {
        AC.capacidad = 4;
        AC.listaDeReglas = new ConfigSensor[AC.capacidad];
    }

    ifstream archivo(nombreArchivo);
    if(!archivo){
        cout<<"El archivo no abrio: "<<nombreArchivo<<endl;
        return;
    }

    const int MAX=256;
    char lineabuffer[MAX];
    while(archivo.getline(lineabuffer, MAX)) {
        if(lineabuffer[0]=='\0') continue;

        // crecer si hace falta
        if (AC.numReglas >= AC.capacidad) {
            int nuevaCap = AC.capacidad * 2;
            ConfigSensor* nuevo = new ConfigSensor[nuevaCap];
            // copiar lo existente (¡OJO! tipo es char*, solo copiamos puntero; pero abajo asignamos uno nuevo)
            for (int i=0; i<AC.numReglas; ++i) nuevo[i] = AC.listaDeReglas[i];
            delete[] AC.listaDeReglas;
            AC.listaDeReglas = nuevo;
            AC.capacidad = nuevaCap;
        }

        // parse: esperado "T,36.0,38.0"  o  "P,90,140,60,90"
        char toks[5][64];
        size_t n = split_simple(std::string(lineabuffer), ',', toks, 5);
        if (n < 3) continue; // línea inválida

        ConfigSensor cfg{};
        // reservar y copiar tipo (1..2 chars típicamente)
        size_t len = strlen(toks[0]);
        cfg.tipo = new char[len+1];
        strcpy(cfg.tipo, toks[0]);

        cfg.min = atof(toks[1]);
        cfg.max = atof(toks[2]);
        // si tiene umbrales diastólicos (solo P)
        if (n >= 5) {
            cfg.umbralMinDiastolica = atof(toks[3]);
            cfg.umbralMaxDiastolica = atof(toks[4]);
        } else {
            cfg.umbralMinDiastolica = 0;
            cfg.umbralMaxDiastolica = 0;
        }

        AC.listaDeReglas[AC.numReglas++] = cfg;
    }
    archivo.close();
}

#endif