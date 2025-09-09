#ifndef MOSTRARVOIDMENU_HH
#define MOSTRARVOIDMENU_HH

#include "funcion1archivos.hh"
#include "funcion2archivos.hh"
#include "funcionGENERARrep.hh"
#include "Ecg.hh"
#include "libs\SALADEUCI.hh"
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

void opcion1(){
    // ================= CONFIGURACIONES DINÁMICAS =================
    ArchivoConfiguracion AC{};
    AC.listaDeReglas = nullptr;
    AC.numReglas = 0;
    AC.capacidad = 0;

    cout<<"Cargando configuracion...\n";
    cargarConfiguracion("configuracion.txt", AC);
    cout<<"Carga de configuracion finalizada, se leyeron "<<AC.numReglas<<" registros\n\n";

    cout<<"=== CONFIGS ===\n";
    for(int i=0; i<AC.numReglas; i++){
        cout<<"Registro "<<(i+1)<<":\n";
        cout<<"  Tipo = "<< AC.listaDeReglas[i].tipo <<"\n";
        cout<<"  minimo = "<< AC.listaDeReglas[i].min <<"\n";
        cout<<"  maximo = "<< AC.listaDeReglas[i].max <<"\n";
        if (AC.listaDeReglas[i].tipo && AC.listaDeReglas[i].tipo[0]=='P') {
            cout<<"  Dia(min,max) = ["<<AC.listaDeReglas[i].umbralMinDiastolica
                <<","<<AC.listaDeReglas[i].umbralMaxDiastolica<<"]\n";
        }
    }
    cout<<"\n";

    // ================= PACIENTES DINÁMICOS =================
    ArchivoPacientes AP{};
    AP.listaDePacientes = nullptr;
    AP.numPacientes = 0;
    AP.capacidad = 0;

    cout<<"Cargando pacientes...\n";
    cargarPacientes("pacientes.csv", AP);
    cout<<"Carga de pacientes finalizada, se leyeron "<<AP.numPacientes<<" registros\n\n";

    cout<<"=== PACIENTES ===\n";
    for(int i=0; i<AP.numPacientes; i++){
        cout<<"Paciente "<<(i+1)<<":\n";
        cout <<"  ID: "<< AP.listaDePacientes[i].id << "\n";
        cout <<"  Tipo Documento: "<< AP.listaDePacientes[i].tipoDocumento << "\n";
        cout <<"  Documento: " << AP.listaDePacientes[i].documento << "\n";
        cout <<"  Nombres: " << AP.listaDePacientes[i].nombres << "\n";
        cout <<"  Apellidos: " << AP.listaDePacientes[i].apellidos << "\n";
        cout <<"  Fecha Nacimiento: "<< AP.listaDePacientes[i].fechaNacimiento << "\n";
        cout <<"  Telefono: " << AP.listaDePacientes[i].telefono << "\n";
        cout <<"  Email: " << AP.listaDePacientes[i].email << "\n";
        cout <<"  Tipo de Sangre: " << AP.listaDePacientes[i].tipoSangre << "\n";
        cout <<"  Entidad de Salud: "<< AP.listaDePacientes[i].entidadSalud << "\n";
        if (AP.listaDePacientes[i].medicinaPrepagada && AP.listaDePacientes[i].medicinaPrepagada[0] != '\0') {
            cout << "  Med. Prepagada: " << AP.listaDePacientes[i].medicinaPrepagada << "\n";
        } else {
            cout << "  Med. Prepagada: N/A\n";
        }
    }

    // ================= LIBERAR MEMORIA =================
    // liberar configs
    for (int i=0;i<AC.numReglas;++i) delete[] AC.listaDeReglas[i].tipo;
    delete[] AC.listaDeReglas;
    AC.listaDeReglas = nullptr;
    AC.numReglas = AC.capacidad = 0;

    // liberar pacientes (cada campo + arreglo)
    liberarArchivoPacientes(AP);
}

void opcion2(){
    const char* ruta = "GenerarBinarioProfe/patient_readings_simulation.bsf";
    ifstream f(ruta, ios::binary);
    if (!f.is_open()) { 
        cout << "No pude abrir " << ruta << "\n";  
        return;
    }

    SalaUCI S{}; 
    S.maquinas = nullptr;
    S.id = 0;
    S.cantidadMaquinas = 0;

    // --- Sala (cabecera .bsf) ---
    uint8_t salaId = 0, numMaq = 0;
    if (!f.read(reinterpret_cast<char*>(&salaId), 1)) { cout << "Fallo leyendo salaId\n"; return; }
    if (!f.read(reinterpret_cast<char*>(&numMaq), 1))  { cout << "Fallo leyendo numMaq\n"; return; }

    S.id = static_cast<char>(salaId);
    S.cantidadMaquinas = static_cast<char>(numMaq);

    cout << "Sala=" << (int)S.id << " | Maquinas=" << (int)S.cantidadMaquinas << "\n";

    // Reservar EXACTO segun archivo
    if (S.cantidadMaquinas > 0) {
        S.maquinas = new MaquinaUCI[static_cast<unsigned char>(S.cantidadMaquinas)];
    }

    // --- Máquinas ---
    for (uint8_t m = 0; m < numMaq && f; ++m) {
        MaquinaUCI& MQ = S.maquinas[m];
        MQ.id = 0;
        MQ.cantidadMediciones = 0;
        MQ.mediciones = nullptr;

        uint8_t idMaqTmp = 0;      
        uint32_t nMedsTmp = 0;

        if (!f.read(reinterpret_cast<char*>(&idMaqTmp), 1)) { cout << "Fallo idMaqTmp\n"; break; }
        if (!f.read(reinterpret_cast<char*>(&nMedsTmp), 4))  { cout << "Fallo nMedsTmp\n"; break; }

        MQ.id = static_cast<int>(idMaqTmp);
        MQ.cantidadMediciones = static_cast<int>(nMedsTmp);

        cout << "  Maq " << MQ.id << " | Mediciones=" << MQ.cantidadMediciones << "\n";

        if (MQ.cantidadMediciones > 0) {
            MQ.mediciones = new Medicion[MQ.cantidadMediciones];
        }

        // --- Mediciones ---
        for (int j = 0; j < MQ.cantidadMediciones && f; ++j) {
            Medicion& M = MQ.mediciones[j];
            M.idPaciente = nullptr;
            M.fechaHora = nullptr;
            M.cantidadLecturas = 0;
            M.lecturas = nullptr;

            // Leer a buffers fijos y luego copiar a dinámico (+ '\0')
            char tmpId[12], tmpFecha[25];
            if (!f.read(tmpId, 11))    { cout << "Fallo idPaciente(11)\n"; break; }
            if (!f.read(tmpFecha, 24)) { cout << "Fallo fechaHora(24)\n"; break; }
            tmpId[11] = '\0';
            tmpFecha[24] = '\0';

            uint32_t nLecsTmp = 0;
            if (!f.read(reinterpret_cast<char*>(&nLecsTmp), 4)) { cout << "Fallo numLecturas\n"; break; }
            M.cantidadLecturas = static_cast<int>(nLecsTmp);

            // Reservar dinámicos exactos para strings/lecturas
            M.idPaciente = new char[12];
            M.fechaHora  = new char[25];
            strcpy(M.idPaciente, tmpId);
            strcpy(M.fechaHora,  tmpFecha);

            if (M.cantidadLecturas > 0) {
                M.lecturas = new Lectura[M.cantidadLecturas];
            }

            cout << "    Med " << j
                 << " | Paciente:[" << M.idPaciente << "]"
                 << " | Fecha:["    << M.fechaHora  << "]"
                 << " | Lecturas="  << M.cantidadLecturas << "\n";

            // --- Lecturas ---
            for (int k = 0; k < M.cantidadLecturas && f; ++k) {
                Lectura& L = M.lecturas[k];
                L.tipoSensor = 0; L.valor = 0.0; L.sistolica = L.diastolica = 0;

                if (!f.read(reinterpret_cast<char*>(&L.tipoSensor), 1)) { 
                    cout << "Fallo tipoSensor\n"; break; 
                }

                cout << "      L" << k << " tipo=" << L.tipoSensor;

                if (L.tipoSensor == 'T' || L.tipoSensor == 'E' || L.tipoSensor == 'O') {
                    if (!f.read(reinterpret_cast<char*>(&L.valor), 8)) { 
                        cout << " (fallo double)\n"; break; 
                    }
                    L.sistolica = L.diastolica = 0;
                    cout << " " << L.valor << "\n";
                } else if (L.tipoSensor == 'P') {
                    if (!f.read(reinterpret_cast<char*>(&L.sistolica), 4) ||
                        !f.read(reinterpret_cast<char*>(&L.diastolica), 4)) { 
                        cout << " (fallo P sis/dia)\n"; break; 
                    }
                    L.valor = 0.0;
                    cout << " " << L.sistolica << "-" << L.diastolica << "\n";
                } else {
                    cout << " (desconocido)\n"; 
                    break;
                }
            }
        }
    }

    // ---- Aquí ya tienes toda la sala en memoria dinámica (S) ----
    // Puedes usar S.maquinas[..].mediciones[..].lecturas[..] como necesites.

    // ========== LIBERAR MEMORIA ==========
    for (int m = 0; m < static_cast<int>(numMaq); ++m) {
        MaquinaUCI& MQ = S.maquinas[m];
        for (int j = 0; j < MQ.cantidadMediciones; ++j) {
            Medicion& M = MQ.mediciones[j];
            delete[] M.idPaciente;
            delete[] M.fechaHora;
            delete[] M.lecturas;
            M.idPaciente = M.fechaHora = nullptr;
            M.lecturas = nullptr;
            M.cantidadLecturas = 0;
        }
        delete[] MQ.mediciones;
        MQ.mediciones = nullptr;
        MQ.cantidadMediciones = 0;
    }
    delete[] S.maquinas;
    S.maquinas = nullptr;
    S.cantidadMaquinas = 0;

    f.close();
}

void opcion3() {
    const char* config_file = "configuracion.txt";
    const char* bsf_file    = "GenerarBinarioProfe/patient_readings_simulation_small.bsf";
    const char* dat_file    = "pacientes_ecg_anomalos.dat";
    const char* out_file    = "validation_ecg_anomalies.txt";

    ConfigSensor ecg_config{};     // ← importante inicializar
    ecg_config.tipo = nullptr;     // porque ahora es char*

    if (!LoadEcgConfig(config_file, ecg_config)) return;

    if (!ExportEcgAnomalousPatients(bsf_file, dat_file, ecg_config)) {
        cout << "Fallo exportando pacientes ECG anómalos.\n";
        delete[] ecg_config.tipo;  // liberar si falló
        return;
    }

    if (!ValidateEcgExportFile(dat_file, out_file, ecg_config)) {
        cout << "Fallo validando el archivo .dat\n";
        delete[] ecg_config.tipo;
        return;
    }

    delete[] ecg_config.tipo;      // liberar al final
    cout << "Proceso completado. Revisa " << out_file << "\n";
}

void opcion4(){
    // ====== CONFIGURACIÓN DINÁMICA ======
    ArchivoConfiguracion AC{};
    AC.listaDeReglas = nullptr;
    AC.numReglas = 0;
    AC.capacidad = 0;

    cout << "=======================================================\n";
    cout << "    SISTEMA DE ANÁLISIS DE LECTURAS MÉDICAS\n";
    cout << "=======================================================\n";

    // Cargar configuración
    cout << "Cargando archivo de configuración...\n";
    cargarConfiguracion("configuracion.txt", AC);

    if (AC.numReglas == 0) {
        cout << "ERROR: No se pudo cargar la configuración.\n";
        cout << "Verifique que existe el archivo 'configuracion.txt'.\n\n";
        // No return: seguimos mostrando formato esperado por si acaso
    } else {
        cout << "Configuración cargada exitosamente.\n";
        cout << "Total de sensores configurados: " << AC.numReglas << "\n\n";

        // Mostrar configuración cargada
        cout << "=== CONFIGURACIÓN DE SENSORES ===\n";
        for (int i = 0; i < AC.numReglas; i++) {
            cout << "Sensor " << AC.listaDeReglas[i].tipo << ": ";

            if (strcmp(AC.listaDeReglas[i].tipo, "T")==0) {
                cout << "Temperatura - ";
            } else if (strcmp(AC.listaDeReglas[i].tipo, "P")==0) {
                cout << "Presión Arterial - ";
            } else if (strcmp(AC.listaDeReglas[i].tipo, "O")==0) {
                cout << "Oxigenación - ";
            } else if (strcmp(AC.listaDeReglas[i].tipo, "E")==0) {
                cout << "ECG - ";
            }

            cout << "Rango [" << AC.listaDeReglas[i].min
                 << " - " << AC.listaDeReglas[i].max << "]";

            if (strcmp(AC.listaDeReglas[i].tipo, "P")==0) {
                cout << " | Diastólica [" << AC.listaDeReglas[i].umbralMinDiastolica
                     << " - " << AC.listaDeReglas[i].umbralMaxDiastolica << "]";
            }
            cout << "\n";
        }
        cout << "\n";
    }

    // Verificar si existe el archivo de lecturas
    ifstream verificarArchivo("lecturas.txt");
    if (!verificarArchivo.is_open()) {
        cout << "ERROR: No se pudo abrir el archivo 'lecturas.txt'\n";
        cout << "Asegúrese de que el archivo existe en el directorio indicado.\n\n";
        cout << "Formato esperado de archivos:\n";
        cout << "--- configuracion.txt ---\n";
        cout << "T,36.0,37.5\n";
        cout << "P,90,140,60,90\n";
        cout << "O,95,100\n";
        cout << "E,-3.858747,1.228621\n\n";
        cout << "--- lecturas.txt ---\n";
        cout << "PAC001;15/01/2024 08:30:15.123;T:36.5,P:120:80,O:98\n";
        cout << "PAC001;15/01/2024 12:30:22.456;T:38.2,P:150:95,O:92,E:0.75\n";
    }
    verificarArchivo.close();

    // ====== Menú ======
    char opcion;
    do {
        cout << "======== MENÚ PRINCIPAL ========\n";
        cout << "1. Analizar todas las lecturas\n";
        cout << "2. Generar reporte de mediciones por paciente\n";
        cout << "3. Salir\n";
        cout << "Seleccione una opción (1-3): ";
        cin >> opcion;
        cout << "\n";

        switch (opcion) {
            case '1': {
                // Analizar todas las lecturas
                cout << "=== ANÁLISIS DE TODAS LAS LECTURAS ===\n\n";

                ifstream archivo("lecturas.txt");
                if (!archivo.is_open()) {
                    cout << "No se pudo abrir lecturas.txt\n\n";
                    break;
                }

                char linea[512];
                int numeroLinea = 0;

                while (archivo.getline(linea, 512)) {
                    numeroLinea++;
                    if (strlen(linea) == 0) continue;

                    cout << "--- LÍNEA " << numeroLinea << " ---\n";
                    cout << "Contenido: \"" << linea << "\"\n";

                    char tokens[10][101];
                    int numTokens = dividirCadena(linea, ';', tokens, 10);

                    if (numTokens < 3) {
                        cout << "ERROR: Formato de línea inválido.\n";
                        continue;
                    }

                    const char* idPaciente = tokens[0];
                    const char* fecha = tokens[1];
                    const char* lecturas = tokens[2];

                    cout << "ID Paciente: " << idPaciente << "\n";
                    cout << "Fecha/Hora: " << fecha << "\n";
                    cout << "Lecturas: " << lecturas << "\n";

                    char lecturasIndividuales[10][101];
                    int numLecturas = dividirCadena(lecturas, ',', lecturasIndividuales, 10);

                    cout << "Análisis:\n";
                    // OJO: ahora pasamos el arreglo dinámico de configs y su tamaño
                    for (int i = 0; i < numLecturas; i++) {
                        analizarLectura(lecturasIndividuales[i], AC.listaDeReglas, AC.numReglas);
                    }
                    cout << "\n";
                }

                archivo.close();
                cout << "=== ANÁLISIS COMPLETADO ===\n\n";
                break;
            }

            case '2': {
                // Generar reporte de mediciones por paciente
                char idPaciente[50];
                cout << "Ingrese el ID del paciente: ";
                cin >> idPaciente;
                cout << "\n";

                cout << "Generando reporte de mediciones para: " << idPaciente << "\n";
                // Igual que antes, pero pasando arreglo dinámico:
                generarMedicionesPaciente(idPaciente,
                    "lecturas.txt",
                    AC.listaDeReglas, AC.numReglas);
                cout << "\n";
                break;
            }

            case '3': {
                cout << "Saliendo del programa...\n";
                break;
            }

            default: {
                cout << "Opción inválida. Por favor seleccione 1, 2 o 3.\n\n";
                break;
            }
        }

    } while (opcion != '3');

    cout << "\n=======================================================\n";
    cout << "Gracias por usar el Sistema de Análisis de Lecturas Médicas\n";
    cout << "=======================================================\n";

    // ====== LIBERAR MEMORIA DE CONFIGURACIÓN ======
    for (int i = 0; i < AC.numReglas; ++i) {
        delete[] AC.listaDeReglas[i].tipo; // tipo es char*
    }
    delete[] AC.listaDeReglas;
    AC.listaDeReglas = nullptr;
    AC.numReglas = AC.capacidad = 0;
}

#endif