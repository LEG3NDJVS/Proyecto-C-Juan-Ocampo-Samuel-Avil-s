#ifndef ECG_HH
#define ECG_HH

#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <cstdlib>   // atof
#include "libs/CONFIGURACION_SENSOR.hh"
#include "libs/LECTURA.hh"

using namespace std;

// Tamaños fijos del formato
const int BSF_ID_PACIENTE_BYTES = 11;
const int BSF_TIMESTAMP_BYTES   = 24;

// --------- Utils mínimos ---------
static bool leerBloque(std::ifstream &in, char* buf, std::streamsize n) {
    in.read(buf, n);
    return static_cast<std::streamsize>(in.gcount()) == n;
}
static bool idsIguales11(const char a[BSF_ID_PACIENTE_BYTES],
                         const char b[BSF_ID_PACIENTE_BYTES]) {
    for (int i = 0; i < BSF_ID_PACIENTE_BYTES; ++i) if (a[i] != b[i]) return false;
    return true;
}
static void copiar11(char dst[BSF_ID_PACIENTE_BYTES],
                     const char src[BSF_ID_PACIENTE_BYTES]) {
    for (int i = 0; i < BSF_ID_PACIENTE_BYTES; ++i) dst[i] = src[i];
}

// --------- Stats por paciente (dinámico) ---------
struct EcgStatsPaciente {
    char id[BSF_ID_PACIENTE_BYTES]; // 11 bytes exactos, sin '\0'
    bool usado;
    int  cuentaECG;
    double minECG;
    double maxECG;
    bool anomalo;
};

// crece el arreglo dinámico de stats cuando haga falta
static bool grow(EcgStatsPaciente* &arr, int &cap) {
    int nueva = (cap == 0 ? 8 : cap * 2);
    EcgStatsPaciente* nuevo = new(std::nothrow) EcgStatsPaciente[nueva];
    if (!nuevo) return false;
    for (int i = 0; i < cap; ++i) nuevo[i] = arr[i];
    delete[] arr;
    arr = nuevo;
    cap = nueva;
    return true;
}

// busca por id o agrega uno nuevo
static int findOrAdd(EcgStatsPaciente* &arr, int &size, int &cap,
                     const char id[BSF_ID_PACIENTE_BYTES]) {
    for (int i = 0; i < size; ++i) {
        if (arr[i].usado && idsIguales11(arr[i].id, id)) return i;
    }
    if (size >= cap) {
        if (!grow(arr, cap)) return -1;
    }
    int idx = size++;
    arr[idx].usado = true;
    copiar11(arr[idx].id, id);
    arr[idx].cuentaECG = 0;
    arr[idx].minECG =  1e300;
    arr[idx].maxECG = -1e300;
    arr[idx].anomalo = false;
    return idx;
}

// --------- PASADA 1: recolecta stats (dinámico) ---------
static int primeraPasadaBSF_dyn(const char* bsf_filename,
                                EcgStatsPaciente* &stats, int &size, int &cap) {
    stats = nullptr; size = 0; cap = 0;

    std::ifstream bin(bsf_filename, std::ios::binary);
    if (!bin.is_open()) {
        std::cout << "No se pudo abrir el archivo BSF: " << bsf_filename << "\n";
        return -1;
    }

    unsigned char uci_id = 0, num_machines = 0;
    if (!leerBloque(bin, reinterpret_cast<char*>(&uci_id), 1)) { bin.close(); return -1; }
    if (!leerBloque(bin, reinterpret_cast<char*>(&num_machines), 1)) { bin.close(); return -1; }

    for (int m = 0; m < (int)num_machines && bin; ++m) {
        unsigned char machine_id = 0;
        if (!leerBloque(bin, reinterpret_cast<char*>(&machine_id), 1)) break;

        unsigned int number_of_measurements = 0;
        if (!leerBloque(bin, reinterpret_cast<char*>(&number_of_measurements), 4)) break;

        for (unsigned int j = 0; j < number_of_measurements && bin; ++j) {
            char patient_id[BSF_ID_PACIENTE_BYTES];
            char timestamp[BSF_TIMESTAMP_BYTES];
            unsigned int number_of_readings = 0;

            if (!leerBloque(bin, patient_id, BSF_ID_PACIENTE_BYTES)) { bin.close(); return -1; }
            if (!leerBloque(bin, timestamp, BSF_TIMESTAMP_BYTES))     { bin.close(); return -1; }
            if (!leerBloque(bin, reinterpret_cast<char*>(&number_of_readings), 4)) { bin.close(); return -1; }

            int idx = findOrAdd(stats, size, cap, patient_id);
            if (idx < 0) { bin.close(); return -1; }

            for (unsigned int k = 0; k < number_of_readings && bin; ++k) {
                char tipo;
                if (!leerBloque(bin, &tipo, 1)) { bin.close(); return -1; }

                if (tipo == 'E') {
                    double v;
                    if (!leerBloque(bin, reinterpret_cast<char*>(&v), sizeof(double))) { bin.close(); return -1; }
                    if (v < stats[idx].minECG) stats[idx].minECG = v;
                    if (v > stats[idx].maxECG) stats[idx].maxECG = v;
                    stats[idx].cuentaECG += 1;
                } else if (tipo == 'T' || tipo == 'O') {
                    double d;
                    if (!leerBloque(bin, reinterpret_cast<char*>(&d), sizeof(double))) { bin.close(); return -1; }
                } else if (tipo == 'P') {
                    unsigned int sis, dia;
                    if (!leerBloque(bin, reinterpret_cast<char*>(&sis), 4)) { bin.close(); return -1; }
                    if (!leerBloque(bin, reinterpret_cast<char*>(&dia), 4)) { bin.close(); return -1; }
                } else {
                    bin.close(); return -1;
                }
            }
        }
    }

    bin.close();
    return size; // pacientes usados
}

// --------- PASADA 2: re-lee .bsf y escribe solo anómalos a .dat ---------
static bool segundaPasadaBSF_dyn(const char* bsf_filename,
                                 const char* dat_filename,
                                 EcgStatsPaciente* stats, int size) {
    std::ifstream bin(bsf_filename, std::ios::binary);
    if (!bin.is_open()) {
        std::cout << "No se pudo abrir el archivo BSF (2da pasada): " << bsf_filename << "\n";
        return false;
    }
    std::ofstream out(dat_filename, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cout << "No se pudo crear el archivo DAT: " << dat_filename << "\n";
        bin.close();
        return false;
    }

    unsigned char uci_id = 0, num_machines = 0;
    if (!leerBloque(bin, reinterpret_cast<char*>(&uci_id), 1)) { bin.close(); out.close(); return false; }
    if (!leerBloque(bin, reinterpret_cast<char*>(&num_machines), 1)) { bin.close(); out.close(); return false; }

    for (int p = 0; p < size; ++p) {
        if (!stats[p].usado || !stats[p].anomalo || stats[p].cuentaECG <= 0) continue;

        // cabecera paciente en .dat: ID(11) + int nLect
        out.write(stats[p].id, BSF_ID_PACIENTE_BYTES);
        int nLect = stats[p].cuentaECG;
        out.write(reinterpret_cast<const char*>(&nLect), sizeof(int));

        // volver al inicio de máquinas (después de 2 bytes cabecera BSF)
        bin.clear();
        bin.seekg(2, std::ios::beg);

        for (int m = 0; m < (int)num_machines && bin; ++m) {
            unsigned char machine_id = 0;
            if (!leerBloque(bin, reinterpret_cast<char*>(&machine_id), 1)) break;

            unsigned int number_of_measurements = 0;
            if (!leerBloque(bin, reinterpret_cast<char*>(&number_of_measurements), 4)) break;

            for (unsigned int j = 0; j < number_of_measurements && bin; ++j) {
                char patient_id[BSF_ID_PACIENTE_BYTES];
                char timestamp[BSF_TIMESTAMP_BYTES];
                unsigned int number_of_readings = 0;

                if (!leerBloque(bin, patient_id, BSF_ID_PACIENTE_BYTES)) { bin.close(); out.close(); return false; }
                if (!leerBloque(bin, timestamp, BSF_TIMESTAMP_BYTES))     { bin.close(); out.close(); return false; }
                if (!leerBloque(bin, reinterpret_cast<char*>(&number_of_readings), 4)) { bin.close(); out.close(); return false; }

                bool esPaciente = idsIguales11(patient_id, stats[p].id);

                for (unsigned int k = 0; k < number_of_readings && bin; ++k) {
                    char tipo;
                    if (!leerBloque(bin, &tipo, 1)) { bin.close(); out.close(); return false; }

                    if (tipo == 'E') {
                        double v;
                        if (!leerBloque(bin, reinterpret_cast<char*>(&v), sizeof(double))) { bin.close(); out.close(); return false; }
                        if (esPaciente) {
                            out.write(timestamp, BSF_TIMESTAMP_BYTES);
                            out.write(reinterpret_cast<const char*>(&v), sizeof(double));
                        }
                    } else if (tipo == 'T' || tipo == 'O') {
                        double d;
                        if (!leerBloque(bin, reinterpret_cast<char*>(&d), sizeof(double))) { bin.close(); out.close(); return false; }
                    } else if (tipo == 'P') {
                        unsigned int sis, dia;
                        if (!leerBloque(bin, reinterpret_cast<char*>(&sis), 4)) { bin.close(); out.close(); return false; }
                        if (!leerBloque(bin, reinterpret_cast<char*>(&dia), 4)) { bin.close(); out.close(); return false; }
                    } else {
                        bin.close(); out.close(); return false;
                    }
                }
            }
        }
    }

    bin.close();
    out.close();
    return true;
}

// --------- API pública ---------
bool ExportEcgAnomalousPatients(const char* bsf_filename,
                                const char* dat_filename,
                                const ConfigSensor &ecg_config) {
    // PASADA 1: recolectar stats dinámicos
    EcgStatsPaciente* stats = nullptr; int usados = 0; int cap = 0;
    int res = primeraPasadaBSF_dyn(bsf_filename, stats, usados, cap);
    if (res < 0) { delete[] stats; return false; }

    // marcar anomalía: regla: |minPaciente|+|maxPaciente| > |minConf|+|maxConf|
    const double umbral = std::fabs(ecg_config.min) + std::fabs(ecg_config.max);
    for (int i = 0; i < usados; ++i) {
        if (!stats[i].usado || stats[i].cuentaECG == 0) { stats[i].anomalo = false; continue; }
        const double sumaAbs = std::fabs(stats[i].minECG) + std::fabs(stats[i].maxECG);
        stats[i].anomalo = (sumaAbs > umbral);
    }

    // PASADA 2: exportar solo anómalos
    bool ok = segundaPasadaBSF_dyn(bsf_filename, dat_filename, stats, usados);

    delete[] stats;
    return ok;
}

// Lee E,min,max desde configuracion.txt (ahora tipo es char* -> reservar!)
bool LoadEcgConfig(const char* config_filename, ConfigSensor &ecg_config) {
    ifstream config_file(config_filename);
    if (!config_file.is_open()) {
        cout << "No se pudo abrir el archivo de configuracion: " << config_filename << endl;
        return false;
    }

    char line[256];
    bool found = false;

    // liberar ecg_config.tipo previo si viniera con algo
    // (Opcional) // if (ecg_config.tipo) { delete[] ecg_config.tipo; ecg_config.tipo = nullptr; }

    while (config_file.getline(line, sizeof(line))) {
        // Formato esperado: E,MIN,MAX   (ej: E,-3.858747,1.228621)
        if (line[0] == 'E') {
            char tipoLocal[4] = {0};
            char num1[64] = {0}, num2[64] = {0};
            int i = 0, j = 0;

            while (line[i] != ',' && line[i] != '\0') { tipoLocal[j++] = line[i++]; }
            tipoLocal[j] = '\0';
            if (line[i] == ',') i++;

            j = 0; while (line[i] != ',' && line[i] != '\0') { num1[j++] = line[i++]; }
            num1[j] = '\0'; if (line[i] == ',') i++;

            j = 0; while (line[i] != '\0') { num2[j++] = line[i++]; }
            num2[j] = '\0';

            // reservar para ecg_config.tipo y copiar
            size_t len = strlen(tipoLocal);
            ecg_config.tipo = new char[len+1];
            strcpy(ecg_config.tipo, tipoLocal);

            ecg_config.min = atof(num1);
            ecg_config.max = atof(num2);

            found = true;
            // si hay varias E, la última ganaría; normalmente hay una sola
        }
    }

    config_file.close();

    if (!found) {
        cout << "No se encontro una linea de configuracion para ECG (que empiece con 'E')." << endl;
        return false;
    }

    return true;
}

// Validación del .dat (igual a la tuya, solo robustez de lecturas)
bool ValidateEcgExportFile(const char* binary_filename, const char* text_filename, 
                           const ConfigSensor &ecg_config) 
{
    ifstream binary_file(binary_filename, ios::in | ios::binary);
    if (!binary_file.is_open()) {
        cout << "No se pudo abrir el archivo binario para lectura: " << binary_filename << endl;
        return false;
    }

    ofstream text_file(text_filename);
    if (!text_file.is_open()) {
        cout << "No se pudo crear el archivo de texto: " << text_filename << endl;
        return false;
    }

    text_file << "=== VALIDACION DEL ARCHIVO DE ANOMALIAS ECG ===\n";
    text_file << "Limites ECG leidos de configuracion.txt: [" 
              << ecg_config.min << " , " << ecg_config.max << "]\n\n";

    int pacientes = 0;
    int total_anomalias = 0;

    for (;;) {
        char patient_id[BSF_ID_PACIENTE_BYTES];
        binary_file.read(patient_id, BSF_ID_PACIENTE_BYTES);
        if (!binary_file) { if (binary_file.eof()) break; cout<<"Fallo leyendo ID\n"; break; }

        int num_lecturas = 0;
        binary_file.read(reinterpret_cast<char*>(&num_lecturas), sizeof(int));
        if (!binary_file) { cout<<"Fallo leyendo num_lecturas\n"; break; }

        pacientes++;
        text_file << "PACIENTE #" << pacientes << "\n  ID: ";
        for (int i = 0; i < BSF_ID_PACIENTE_BYTES; i++) text_file << patient_id[i];
        text_file << "\n  Numero de lecturas: " << num_lecturas << "\n";

        for (int i = 0; i < num_lecturas; i++) {
            char fecha_hora[BSF_TIMESTAMP_BYTES];
            double valor = 0.0;

            binary_file.read(fecha_hora, BSF_TIMESTAMP_BYTES);
            if (!binary_file) { cout<<"Fallo fecha_hora\n"; break; }

            binary_file.read(reinterpret_cast<char*>(&valor), sizeof(double));
            if (!binary_file) { cout<<"Fallo valor\n"; break; }

            bool anomalo = (valor < ecg_config.min || valor > ecg_config.max);
            if (anomalo) total_anomalias++;

            text_file << "    Lectura " << (i+1) << ":\n";
            text_file << "      Fecha/Hora: ";
            for (int j = 0; j < BSF_TIMESTAMP_BYTES; j++) text_file << fecha_hora[j];
            text_file << "\n";
            text_file << "      Valor ECG: " << valor
                      << (anomalo ? " --> ANOMALO\n" : " --> NORMAL\n");
        }
        text_file << "\n";
        if (!binary_file) { cout<<"Se detuvo el procesamiento por una lectura incompleta.\n"; break; }
    }

    text_file << "=== RESUMEN ===\n";
    text_file << "Pacientes procesados: " << pacientes << "\n";
    text_file << "Total de anomalias: " << total_anomalias << "\n";

    binary_file.close();
    text_file.close();
    return true;
}


#endif
