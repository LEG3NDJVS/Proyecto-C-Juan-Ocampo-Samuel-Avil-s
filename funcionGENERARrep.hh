#ifndef FUNCION_GENERAR_REP_HH
#define FUNCION_GENERAR_REP_HH

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include "libs/CONFIGURACION_SENSOR.hh"

using namespace std;

// Estructura para configuración de sensores


// Estructura para almacenar una lectura completa
struct LecturaPaciente {
    char idPaciente[50];
    char fecha[30];
    char tipoSensor;
    double valor;
    double valorDiastolica;
    bool esAnomala;
    char descripcionAnomalia[100];
};

// Estructura para estadísticas
struct EstadisticasSensor {
    char tipoSensor;
    double minimo;
    double maximo;
    double suma;
    int contador;
};

// Función para eliminar espacios al inicio y final de una cadena
void limpiarCadena(char* cadena) {
    // Eliminar espacios al inicio
    int inicio = 0;
    while (cadena[inicio] != '\0' && isspace(cadena[inicio])) {
        inicio++;
    }
    
    // Mover la cadena hacia el inicio
    int i = 0;
    while (cadena[inicio + i] != '\0') {
        cadena[i] = cadena[inicio + i];
        i++;
    }
    cadena[i] = '\0';
    
    // Eliminar espacios al final
    int longitud = strlen(cadena);
    while (longitud > 0 && isspace(cadena[longitud - 1])) {
        longitud--;
    }
    cadena[longitud] = '\0';
}

// Función para dividir una cadena por un delimitador
int dividirCadena(const char* cadena, char delimitador, char resultado[][101], int maxTokens) {
    int numTokens = 0;
    int inicio = 0;
    int longitud = strlen(cadena);
    
    for (int i = 0; i <= longitud && numTokens < maxTokens; i++) {
        if (cadena[i] == delimitador || cadena[i] == '\0') {
            if (i > inicio) {
                int longitudToken = i - inicio;
                strncpy(resultado[numTokens], &cadena[inicio], longitudToken);
                resultado[numTokens][longitudToken] = '\0';
                limpiarCadena(resultado[numTokens]);
                
                if (strlen(resultado[numTokens]) > 0) {
                    numTokens++;
                }
            }
            inicio = i + 1;
        }
    }
    
    return numTokens;
}

// Función para convertir fecha a formato comparable (AAAAMMDDHHMMSS)
void convertirFechaComparable(const char* fechaOriginal, char* fechaComparable) {
    char fechaCopia[30];
    strcpy(fechaCopia, fechaOriginal);
    
    char partes[10][101];
    int numPartes = dividirCadena(fechaCopia, ' ', partes, 10);
    
    if (numPartes >= 2) {
        // Dividir fecha DD/MM/AAAA
        char partesFecha[10][101];
        int numFecha = dividirCadena(partes[0], '/', partesFecha, 10);
        
        // Dividir hora HH:MM:ss.sss
        char partesHora[10][101];
        int numHora = dividirCadena(partes[1], ':', partesHora, 10);
        
        if (numFecha >= 3 && numHora >= 3) {
            // Separar segundos de microsegundos
            char segundos[10][101];
            dividirCadena(partesHora[2], '.', segundos, 10);
            
            sprintf(fechaComparable, "%s%02d%02d%02d%02d%02d", 
                    partesFecha[2],  // año
                    atoi(partesFecha[1]),  // mes
                    atoi(partesFecha[0]),  // día
                    atoi(partesHora[0]),   // hora
                    atoi(partesHora[1]),   // minuto
                    atoi(segundos[0]));    // segundo
        } else {
            strcpy(fechaComparable, fechaOriginal);
        }
    } else {
        strcpy(fechaComparable, fechaOriginal);
    }
}

// Función para comparar dos fechas (retorna true si fecha1 < fecha2)
bool esFechaMenor(const char* fecha1, const char* fecha2) {
    char fecha1Comp[20], fecha2Comp[20];
    convertirFechaComparable(fecha1, fecha1Comp);
    convertirFechaComparable(fecha2, fecha2Comp);
    return strcmp(fecha1Comp, fecha2Comp) < 0;
}

// Función para cargar configuración desde archivo


// Función para verificar si una lectura es anómala
bool esLecturaAnomala(char tipoSensor, double valor, double valorDiastolica, 
                      const ConfigSensor configuraciones[], int totalConfigs, char* descripcion) {
    // 1) buscar la config del mismo tipo
    const ConfigSensor* cfg = nullptr;
    for (int i = 0; i < totalConfigs; i++) {
        // si tu ConfigSensor::tipo es char[3] con "T","P","O","E":
        if (configuraciones[i].tipo[0] == tipoSensor) {
            cfg = &configuraciones[i];
            break;
        }
        // (alternativa) if (strcmp(configuraciones[i].tipo, "T")==0 && tipoSensor=='T') ...
    }

    if (!cfg) { // no se encontró configuración para ese tipo
        strcpy(descripcion, "ERROR - Sensor no configurado");
        return false;
    }

    // 2) aplicar reglas por tipo
    if (tipoSensor == 'P') {
        bool sistolicaAnomala  = (valor < cfg->min || valor > cfg->max);
        bool diastolicaAnomala = (valorDiastolica < cfg->umbralMinDiastolica || 
                                  valorDiastolica > cfg->umbralMaxDiastolica);

        if (sistolicaAnomala && diastolicaAnomala) {
            strcpy(descripcion, "ANOMALA - Presión sistólica y diastólica fuera de rango");
            return true;
        } else if (sistolicaAnomala) {
            strcpy(descripcion, "ANOMALA - Presión sistólica fuera de rango");
            return true;
        } else if (diastolicaAnomala) {
            strcpy(descripcion, "ANOMALA - Presión diastólica fuera de rango");
            return true;
        } else {
            strcpy(descripcion, "NORMAL");
            return false;
        }
    }
    else if (tipoSensor == 'E') {
        // normalizar mensaje según lo que pida el profe
        strcpy(descripcion, "Sin análisis (ECG)");
        return false;
    }
    else { // T u O
        if (valor < cfg->min || valor > cfg->max) {
            // ojo con el formato: usa snprintf para evitar desbordes
            snprintf(descripcion, 100, "ANOMALA - Fuera del rango [%.1f - %.1f]", cfg->min, cfg->max);
            return true;
        } else {
            snprintf(descripcion, 100, "NORMAL - Dentro del rango [%.1f - %.1f]", cfg->min, cfg->max);
            return false;
        }
    }
}

// Función para ordenar lecturas por fecha (algoritmo burbuja)
void ordenarLecturasPorFecha(LecturaPaciente lecturas[], int numLecturas) {
    for (int i = 0; i < numLecturas - 1; i++) {
        for (int j = 0; j < numLecturas - i - 1; j++) {
            if (!esFechaMenor(lecturas[j].fecha, lecturas[j + 1].fecha)) {
                // Intercambiar elementos
                LecturaPaciente temp = lecturas[j];
                lecturas[j] = lecturas[j + 1];
                lecturas[j + 1] = temp;
            }
        }
    }
}

// Función para actualizar estadísticas de un sensor
void actualizarEstadisticas(EstadisticasSensor estadisticas[], int& numEstadisticas, 
                          char tipoSensor, double valor) {
    // Buscar si ya existe estadística para este sensor
    int indice = -1;
    for (int i = 0; i < numEstadisticas; i++) {
        if (estadisticas[i].tipoSensor == tipoSensor) {
            indice = i;
            break;
        }
    }
    
    if (indice == -1) {
        // Crear nueva estadística
        if (numEstadisticas < 10) {
            indice = numEstadisticas;
            estadisticas[indice].tipoSensor = tipoSensor;
            estadisticas[indice].minimo = valor;
            estadisticas[indice].maximo = valor;
            estadisticas[indice].suma = valor;
            estadisticas[indice].contador = 1;
            numEstadisticas++;
        }
    } else {
        // Actualizar estadística existente
        if (valor < estadisticas[indice].minimo) {
            estadisticas[indice].minimo = valor;
        }
        if (valor > estadisticas[indice].maximo) {
            estadisticas[indice].maximo = valor;
        }
        estadisticas[indice].suma += valor;
        estadisticas[indice].contador++;
    }
}

// Función para procesar una lectura individual
void procesarLectura(const char* lecturaStr, const ConfigSensor configuraciones[], int totalConfigs,
                    const char* idPaciente, const char* fecha, LecturaPaciente& lectura) {
    char tokens[10][101];
    int numTokens = dividirCadena(lecturaStr, ':', tokens, 10);
    
    if (numTokens >= 2) {
        strcpy(lectura.idPaciente, idPaciente);
        strcpy(lectura.fecha, fecha);
        lectura.tipoSensor = tokens[0][0];
        lectura.valor = atof(tokens[1]);
        
        if (lectura.tipoSensor == 'P' && numTokens >= 3) {
            lectura.valorDiastolica = atof(tokens[2]);
        } else {
            lectura.valorDiastolica = 0.0;
        }
        
        lectura.esAnomala = esLecturaAnomala(lectura.tipoSensor, lectura.valor, lectura.valorDiastolica,
                                           configuraciones, totalConfigs, lectura.descripcionAnomalia);
    }
}

// Función principal para generar reporte de mediciones por paciente
void generarMedicionesPaciente(const char* idPaciente, const char* archivoLecturas, 
                              const ConfigSensor configuraciones[], int totalConfigs) {
    // Crear nombre del archivo de salida
    char nombreArchivo[200];
    strcpy(nombreArchivo, "mediciones_paciente_");
    strcat(nombreArchivo, idPaciente);
    strcat(nombreArchivo, ".txt");
    
    ifstream archivo(archivoLecturas);
    if (!archivo.is_open()) {
        cout << "Error: No se pudo abrir el archivo " << archivoLecturas << endl;
        return;
    }
    
    // Arreglo para almacenar todas las lecturas del paciente
    const int MAX_LECTURAS = 1000;
    LecturaPaciente lecturas[MAX_LECTURAS];
    int numLecturas = 0;
    
    // Leer todas las lecturas del paciente
    char linea[512];
    while (archivo.getline(linea, 512) && numLecturas < MAX_LECTURAS) {
        if (strlen(linea) == 0) continue;
        
        char tokens[10][101];
        int numTokens = dividirCadena(linea, ';', tokens, 10);
        
        if (numTokens >= 3 && strcmp(tokens[0], idPaciente) == 0) {
            char fecha[30];
            strcpy(fecha, tokens[1]);
            
            char lecturasStr[10][101];
            int numLecturasLinea = dividirCadena(tokens[2], ',', lecturasStr, 10);
            
            for (int i = 0; i < numLecturasLinea && numLecturas < MAX_LECTURAS; i++) {
                procesarLectura(lecturasStr[i], configuraciones, totalConfigs, 
                              idPaciente, fecha, lecturas[numLecturas]);
                numLecturas++;
            }
        }
    }
    archivo.close();
    
    if (numLecturas == 0) {
        cout << "No se encontraron lecturas para el paciente: " << idPaciente << endl;
        return;
    }
    
    // Ordenar lecturas cronológicamente
    ordenarLecturasPorFecha(lecturas, numLecturas);
    
    // Crear archivo de salida
    ofstream reporte(nombreArchivo);
    if (!reporte.is_open()) {
        cout << "Error: No se pudo crear el archivo " << nombreArchivo << endl;
        return;
    }
    
    // Escribir encabezado
    reporte << "=================================================" << endl;
    reporte << "    REPORTE DE MEDICIONES MÉDICAS" << endl;
    reporte << "    Paciente: " << idPaciente << endl;
    reporte << "=================================================" << endl << endl;
    
    // Agrupar y mostrar por tipo de sensor
    char tiposSensores[] = {'T', 'P', 'O', 'E'};
    char nombresSensores[][30] = {"TEMPERATURA", "PRESIÓN ARTERIAL", "OXIGENACIÓN", "ELECTROCARDIOGRAMA"};
    
    EstadisticasSensor estadisticas[10];
    int numEstadisticas = 0;
    
    for (int s = 0; s < 4; s++) {
        char tipoActual = tiposSensores[s];
        bool hayLecturas = false;
        
        // Verificar si hay lecturas de este tipo
        for (int i = 0; i < numLecturas; i++) {
            if (lecturas[i].tipoSensor == tipoActual) {
                hayLecturas = true;
                break;
            }
        }
        
        if (!hayLecturas) continue;
        
        reporte << "=== " << nombresSensores[s] << " ===" << endl;
        reporte << fixed << setprecision(2);
        
        for (int i = 0; i < numLecturas; i++) {
            if (lecturas[i].tipoSensor == tipoActual) {
                reporte << "Fecha: " << lecturas[i].fecha << " | ";
                
                if (tipoActual == 'P') {
                    reporte << "Valor: " << lecturas[i].valor << "/" << lecturas[i].valorDiastolica;
                    // Actualizar estadísticas para sistólica y diastólica
                    actualizarEstadisticas(estadisticas, numEstadisticas, 'S', lecturas[i].valor);
                    actualizarEstadisticas(estadisticas, numEstadisticas, 'D', lecturas[i].valorDiastolica);
                } else {
                    reporte << "Valor: " << lecturas[i].valor;
                    actualizarEstadisticas(estadisticas, numEstadisticas, tipoActual, lecturas[i].valor);
                }
                
                reporte << " | Estado: " << lecturas[i].descripcionAnomalia << endl;
            }
        }
        
        // Mostrar estadísticas para este sensor
        reporte << endl << "--- Estadísticas ---" << endl;
        if (tipoActual == 'P') {
            // Estadísticas separadas para sistólica y diastólica
            for (int e = 0; e < numEstadisticas; e++) {
                if (estadisticas[e].tipoSensor == 'S') {
                    double promedio = estadisticas[e].suma / estadisticas[e].contador;
                    reporte << "Presión Sistólica - Mín: " << estadisticas[e].minimo 
                           << ", Máx: " << estadisticas[e].maximo 
                           << ", Promedio: " << promedio 
                           << " (Total: " << estadisticas[e].contador << " lecturas)" << endl;
                }
                if (estadisticas[e].tipoSensor == 'D') {
                    double promedio = estadisticas[e].suma / estadisticas[e].contador;
                    reporte << "Presión Diastólica - Mín: " << estadisticas[e].minimo 
                           << ", Máx: " << estadisticas[e].maximo 
                           << ", Promedio: " << promedio 
                           << " (Total: " << estadisticas[e].contador << " lecturas)" << endl;
                }
            }
        } else {
            for (int e = 0; e < numEstadisticas; e++) {
                if (estadisticas[e].tipoSensor == tipoActual) {
                    double promedio = estadisticas[e].suma / estadisticas[e].contador;
                    reporte << "Mínimo: " << estadisticas[e].minimo 
                           << ", Máximo: " << estadisticas[e].maximo 
                           << ", Promedio: " << promedio 
                           << " (Total: " << estadisticas[e].contador << " lecturas)" << endl;
                    break;
                }
            }
        }
        reporte << endl;
    }
    
    reporte << "=================================================" << endl;
    reporte << "Reporte generado exitosamente." << endl;
    reporte << "Total de lecturas procesadas: " << numLecturas << endl;
    reporte << "=================================================" << endl;
    
    reporte.close();
    cout << "Reporte generado: " << nombreArchivo << endl;
}

// Función simple para analizar una lectura (compatibilidad)
void analizarLectura(const char* lecturaStr, const ConfigSensor configs[], int totalConfigs) {
    char tokens[10][101];
    int numTokens = dividirCadena(lecturaStr, ':', tokens, 10);
    
    if (numTokens >= 2) {
        char tipoSensor = tokens[0][0];
        double valor = atof(tokens[1]);
        double valorDiastolica = (tipoSensor == 'P' && numTokens >= 3) ? atof(tokens[2]) : 0.0;
        
        char descripcion[100];
        bool anomala = esLecturaAnomala(tipoSensor, valor, valorDiastolica, configs, totalConfigs, descripcion);
        
        cout << "    Sensor " << tipoSensor << ": ";
        if (tipoSensor == 'P') {
            cout << valor << "/" << valorDiastolica;
        } else {
            cout << valor;
        }
        cout << " -> " << descripcion << endl;
    }
}

// Función de compatibilidad para generar reporte (llama a la nueva función)
void generarReporte(const char* idPaciente, const char* nombreArchivoMediciones, 
                   const ConfigSensor configs[], int totalConfigs) {
    cout << "Generando reporte detallado de mediciones para: " << idPaciente << endl;
    generarMedicionesPaciente(idPaciente, nombreArchivoMediciones, configs, totalConfigs);
}

#endif