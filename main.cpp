#include "funcion1archivos.hh"
#include "funcion2archivos.hh"
#include "funcionGENERARrep.hh"
#include "mostrarvoidmenu.hh"
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <cstdlib>

using namespace std;

void mostrarMenu() {
    cout << "\n--- Sistema de Monitoreo Biomédico ---" << endl;
    cout << "1. Cargar archivos de configuración y pacientes" << endl;
    cout << "2. Leer archivo de datos (.bsf)" << endl;
    cout << "3. Generar reporte de anomalías" << endl;
    cout << "4. Calcular estadísticas y exportar dichos datos procesados" << endl;
    cout << "5. Salir" << endl;
    cout << "Seleccione una opción: ";
}

int main() {
    int opcion;
    bool salir = false;

    do {
        mostrarMenu();
        cin >> opcion;

        switch (opcion) {
            case 1:{
                cout << "Opción 1 seleccionada: Cargando archivos..." << endl;
                opcion1();
                break;
            }
            case 2:{
                cout << "Opción 2 seleccionada: Leyendo archivo .bsf..." << endl;
                opcion2();
                break;
            }
            case 3:{
                cout << "Opción 3 Generar reporte." << endl;
                opcion3();
                break;
            }
            case 4:{
                cout << "Opción 4 Calculando estadisticas y exportando datos." << endl;
                opcion4();
                break;
            }
            case 5:{
                salir = true;
                cout << "Saliendo del programa. ¡Hasta luego!" << endl;
                break;
            }
            default:
                cout << "Opción no válida. Por favor, intente de nuevo." << endl;
                break;
        }
    } while (!salir);

    return 0;
}