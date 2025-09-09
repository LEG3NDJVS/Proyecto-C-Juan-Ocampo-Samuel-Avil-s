#ifndef FUNCION2_HH
#define FUNCION2_HH

#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <cstddef>
#include <fstream>
#include "libs/PACIENTE.hh"
#include "libs/ARCHIVO_PACIENTES.hh"

using namespace std;

// --------- utils ---------
inline char* clone_cstr(const char* s) {
    size_t n = strlen(s);
    char* p = new char[n+1];
    strcpy(p, s);
    return p;
}

static char* trim_inplace(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

static size_t split_simple(const std::string& str, char delim, char tokens[][128], size_t maxTok) {
    size_t token_count = 0;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim) && token_count < maxTok) {
        // Trim
        size_t a = 0, b = token.size();
        while (a < b && isspace((unsigned char)token[a])) a++;
        while (b > a && isspace((unsigned char)token[b-1])) b--;
        size_t n = b - a;
        if (n == 0) {
            tokens[token_count][0] = '\0';
        } else {
            if (n >= 128) n = 127;
            strncpy(tokens[token_count], token.c_str()+a, n);
            tokens[token_count][n] = '\0';
        }
        token_count++;
    }
    return token_count;
}

// --------- liberar memoria de un Pacientes ---------
inline void liberarPaciente(Pacientes& p) {
    delete[] p.tipoDocumento;
    delete[] p.documento;
    delete[] p.nombres;
    delete[] p.apellidos;
    delete[] p.fechaNacimiento;
    delete[] p.telefono;
    delete[] p.email;
    delete[] p.tipoSangre;
    delete[] p.entidadSalud;
    delete[] p.medicinaPrepagada;
    // poner punteros a null por seguridad
    p.tipoDocumento = p.documento = p.nombres = p.apellidos = p.fechaNacimiento =
    p.telefono = p.email = p.tipoSangre = p.entidadSalud = p.medicinaPrepagada = nullptr;
}

// --------- cargar una línea en Pacientes (dinámico) ---------
inline void separarLineaEnPaciente(const std::string& lineapac, Pacientes& paciente) {
    char tokens[20][128];
    size_t count = split_simple(lineapac, ';', tokens, 20);

    if (count >= 10) {
        paciente.id = atoi(trim_inplace(tokens[0]));

        paciente.tipoDocumento   = clone_cstr(trim_inplace(tokens[1]));
        paciente.documento       = clone_cstr(trim_inplace(tokens[2]));
        paciente.nombres         = clone_cstr(trim_inplace(tokens[3]));
        paciente.apellidos       = clone_cstr(trim_inplace(tokens[4]));
        paciente.fechaNacimiento = clone_cstr(trim_inplace(tokens[5]));
        paciente.telefono        = clone_cstr(trim_inplace(tokens[6]));
        paciente.email           = clone_cstr(trim_inplace(tokens[7]));
        paciente.tipoSangre      = clone_cstr(trim_inplace(tokens[8]));
        paciente.entidadSalud    = clone_cstr(trim_inplace(tokens[9]));

        if (count > 10 && strlen(tokens[10]) > 0) {
            paciente.medicinaPrepagada = clone_cstr(trim_inplace(tokens[10]));
        } else {
            paciente.medicinaPrepagada = new char[1];
            paciente.medicinaPrepagada[0] = '\0';
        }
    } else {
        // rellena vacío robusto
        paciente.id = 0;
        paciente.tipoDocumento   = clone_cstr("");
        paciente.documento       = clone_cstr("");
        paciente.nombres         = clone_cstr("");
        paciente.apellidos       = clone_cstr("");
        paciente.fechaNacimiento = clone_cstr("");
        paciente.telefono        = clone_cstr("");
        paciente.email           = clone_cstr("");
        paciente.tipoSangre      = clone_cstr("");
        paciente.entidadSalud    = clone_cstr("");
        paciente.medicinaPrepagada = clone_cstr("");
        cout << "Advertencia: línea sin formato esperado: " << lineapac << "\n";
    }
}

// --------- carga dinámica de pacientes ---------
inline void cargarPacientes(const char* nombreArchivoP, ArchivoPacientes& AP) {
    // init contenedor si hace falta
    AP.numPacientes = (AP.numPacientes<0?0:AP.numPacientes);
    if (AP.capacidad <= 0) {
        AP.capacidad = 8;
        AP.listaDePacientes = new Pacientes[AP.capacidad];
    }

    ifstream archivo(nombreArchivoP);
    if(!archivo){
        cout<<"El archivo no abrio: "<<nombreArchivoP<<endl;
        return;
    }

    const int MAX=512;
    char lineabuffer[MAX];

    while(archivo.getline(lineabuffer, MAX)) {
        if(lineabuffer[0]=='\0') continue;

        // crecer si hace falta
        if (AP.numPacientes >= AP.capacidad) {
            int nuevaCap = AP.capacidad * 2;
            Pacientes* nuevo = new Pacientes[nuevaCap];
            // copiar "shallow" (punteros se mueven tal cual)
            for (int i=0;i<AP.numPacientes;++i) nuevo[i] = AP.listaDePacientes[i];
            delete[] AP.listaDePacientes;
            AP.listaDePacientes = nuevo;
            AP.capacidad = nuevaCap;
        }

        Pacientes p{};
        separarLineaEnPaciente(std::string(lineabuffer), p);
        AP.listaDePacientes[AP.numPacientes++] = p; // copia de punteros (OK)
    }

    archivo.close();
}

// --------- liberar todo el contenedor ---------
inline void liberarArchivoPacientes(ArchivoPacientes& AP) {
    for (int i=0; i<AP.numPacientes; ++i) liberarPaciente(AP.listaDePacientes[i]);
    delete[] AP.listaDePacientes;
    AP.listaDePacientes = nullptr;
    AP.numPacientes = AP.capacidad = 0;
}

#endif