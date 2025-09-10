# Informe Final - Sistema de Monitoreo Biomédico

Este proyecto implementa un sistema en C++ para gestionar lecturas médicas (temperatura, presión arterial, oxigenación y ECG), cargar pacientes y generar reportes de anomalías.

---

## 📂 Estructuras utilizadas

### 1. Configuración de sensores (`CONFIGURACION_SENSOR.hh`)
La estructura principal es **ConfigSensor**, que define las reglas de validación:

```cpp
struct ConfigSensor {
    char* tipo;    // Tipo de sensor: "T", "P", "O", "E"
    double min;    // Valor mínimo aceptado
    double max;    // Valor máximo aceptado
    double umbralMinDiastolica;  // Para presión arterial
    double umbralMaxDiastolica;  // Para presión arterial
};
```

**Explicación detallada del funcionamiento:**
- Esta estructura agrupa variables para representar la configuración de un sensor, utilizada para validar lecturas biomédicas.
- `tipo` es un puntero a char (cadena dinámica) que almacena el identificador del sensor (e.g., "T" para temperatura). Se usa un puntero para permitir longitudes variables y asignación dinámica de memoria, lo que evita limitaciones de tamaño fijo.
- `min` y `max` son valores de tipo double que definen el rango aceptable para sensores como temperatura ("T") u oxigenación ("O").
- Para presión arterial ("P"), se incluyen `umbralMinDiastolica` y `umbralMaxDiastolica` para validar el componente diastólico de la presión, ya que este sensor requiere dos valores (sistólica y diastólica).
- **Flujo en el sistema:** Esta estructura se instancia múltiples veces (una por tipo de sensor) y se almacena en un contenedor dinámico, como un array o lista en `ArchivoConfiguracion`. Durante la validación de lecturas, se itera sobre estas configuraciones para comparar los valores medidos contra `min` y `max` (o umbrales diastólicos para presión). 
- **Manejo de memoria:** `tipo` requiere asignación dinámica (e.g., con `new char[]`) y debe liberarse manualmente con `delete[]` en el destructor de la estructura contenedora para evitar memory leaks. Los otros campos son tipos primitivos, por lo que no requieren gestión adicional.
- **Manejo de errores:** No hay validación explícita en la estructura; se asume que los valores se inicializan correctamente en funciones de carga (e.g., `cargarConfiguracion`). Un error en el archivo de configuración (como valores no numéricos) podría causar fallos en `atof` más adelante.
- **Integración:** Se usa en funciones como `esLecturaAnomala` para validar lecturas y en `ExportEcgAnomalousPatients` para ECG específicamente.

👉 Esto permite definir umbrales de normalidad, por ejemplo: `T,36.0,37.5` para temperatura o `P,90,140,60,90` para presión sistólica/diastólica.

---

### 2. Pacientes (`PACIENTE.hh` y `ARCHIVO_PACIENTES.hh`)

Estructura dinámica que almacena todos los datos de un paciente:

```cpp
struct Pacientes {
    int id;
    char* tipoDocumento;
    char* documento;
    char* nombres;
    char* apellidos;
    char* fechaNacimiento;
    char* telefono;
    char* email;
    char* tipoSangre;
    char* entidadSalud;
    char* medicinaPrepagada;
};
```

Y su contenedor:

```cpp
struct ArchivoPacientes {
    Pacientes* listaDePacientes;
    int numPacientes;
    int capacidad;
};
```

**Explicación detallada del funcionamiento:**
- `Pacientes` es una estructura que representa un registro individual de paciente con todos sus datos personales.
- `id` es un entero único para identificar al paciente (e.g., clave primaria).
- Los campos de texto (`tipoDocumento`, `documento`, etc.) son punteros a char para manejar cadenas de longitud variable, asignadas dinámicamente con funciones como `new` o `strdup`.
- `ArchivoPacientes` es un contenedor dinámico que actúa como un array redimensionable (similar a `std::vector` pero implementado manualmente). `listaDePacientes` apunta a un array de `Pacientes`, `numPacientes` cuenta los elementos actuales, y `capacidad` indica el tamaño máximo antes de necesitar reallocar.
- **Flujo en el sistema:** Al cargar desde un archivo CSV (`pacientes.csv`), cada línea se parsea en una instancia de `Pacientes` usando una función como `separarLineaEnPaciente`. Si `numPacientes` alcanza `capacidad`, el array se redimensiona (e.g., duplicando capacidad con `realloc` o `new`). Esto permite manejar un número variable de pacientes.
- **Manejo de memoria:** Cada campo `char*` en `Pacientes` requiere asignación dinámica y debe liberarse individualmente. `listaDePacientes` también se asigna dinámicamente y debe liberarse junto con todas las instancias de `Pacientes`. Un destructor o función de limpieza debe recorrer cada paciente y liberar cada `char*` antes de liberar el array principal.
- **Manejo de errores:** No hay validación explícita de duplicados de ID o formatos de texto (e.g., email válido). Un CSV malformado podría causar accesos fuera de límites o cadenas nulas.
- **Integración:** Los pacientes se asocian con lecturas médicas mediante `idPaciente` en `Medicion`, permitiendo generar reportes específicos por paciente.

👉 Los pacientes se cargan desde `pacientes.csv` y se almacenan dinámicamente en `ArchivoPacientes`.

---

### 3. Sala UCI (`SALADEUCI.hh`)

Modela una sala hospitalaria con máquinas y lecturas biomédicas:

```cpp
struct Lectura {
    char tipoSensor;   // 'T', 'P', 'O', 'E'
    double valor;
    uint32_t sistolica;
    uint32_t diastolica;
};

struct Medicion {
    char* idPaciente;
    char* fechaHora;
    int cantidadLecturas;
    Lectura* lecturas;
};

struct MaquinaUCI {
    int id;
    int cantidadMediciones;
    Medicion* mediciones;
};

struct SalaUCI {
    char id;
    char cantidadMaquinas;
    MaquinaUCI* maquinas;
};
```

**Explicación detallada del funcionamiento:**
- Esta jerarquía de estructuras modela datos de un archivo binario (.bsf) que simula lecturas de una UCI.
- `Lectura`: Representa una medición individual. `tipoSensor` identifica el sensor ('T', 'P', 'O', 'E'), `valor` es el valor principal (double para precisión en temperatura, oxigenación o ECG), y `sistolica`/`diastolica` son relevantes solo para 'P' (presión arterial), usando enteros sin signo para evitar valores negativos inválidos.
- `Medicion`: Agrupa lecturas para un paciente en un momento dado. `idPaciente` y `fechaHora` son cadenas dinámicas, `lecturas` es un array dinámico de `Lectura`.
- `MaquinaUCI`: Representa una máquina en la UCI con un ID y un array dinámico de `Medicion`.
- `SalaUCI`: Nivel superior, con un ID de sala, conteo de máquinas, y array dinámico de `MaquinaUCI`.
- **Flujo en el sistema:** Se lee un archivo .bsf en modo binario (`ifstream` con `ios::binary`). El proceso típico:
  1. Lee `id` y `cantidadMaquinas` de `SalaUCI`.
  2. Para cada máquina, lee `id` y `cantidadMediciones`.
  3. Para cada medición, lee `idPaciente`, `fechaHora`, `cantidadLecturas`, y un array de `Lectura`.
  4. Asigna memoria dinámica para cada array (`new Lectura[cantidadLecturas]`, etc.).
- **Manejo de memoria:** Cada nivel (lecturas, mediciones, máquinas) requiere asignación dinámica y liberación recursiva para evitar memory leaks. Un destructor debe liberar `lecturas`, `mediciones`, y `maquinas` en orden inverso.
- **Manejo de errores:** No se menciona validación de formato del archivo .bsf (e.g., datos corruptos o EOF inesperado). Un archivo malformado podría causar fallos en `read`.
- **Integración:** Se usa en `opcion2` para cargar datos y en `opcion4` para generar reportes, validando lecturas contra `ConfigSensor`.

👉 Estas estructuras permiten interpretar un archivo binario `.bsf` que simula las lecturas de una sala UCI.

---

### 4. Estadísticas de ECG (`Ecg.hh`)

Define estadísticas por paciente:

```cpp
struct EcgStatsPaciente {
    char id[11];   // Identificador del paciente (11 bytes)
    bool usado;
    int cuentaECG;
    double minECG;
    double maxECG;
    bool anomalo;
};
```

**Explicación detallada del funcionamiento:**
- Esta estructura almacena estadísticas agregadas para lecturas ECG por paciente.
- `id` es un array fijo de 11 chars (incluye null-terminator) para IDs como "PAC001". Tamaño fijo evita asignación dinámica pero limita longitud.
- `usado` es un flag booleano para marcar slots ocupados en un array dinámico (evita inicializar slots vacíos).
- `cuentaECG`, `minECG`, `maxECG` rastrean el número de lecturas ECG, y sus valores mínimo y máximo.
- `anomalo` indica si alguna lectura está fuera de rango.
- **Flujo en el sistema:** Se usa en un proceso de dos pasadas sobre un archivo .bsf:
  1. Primera pasada recolecta estadísticas (min, max, cuenta) para cada paciente.
  2. Segunda pasada marca `anomalo` basado en umbrales de `ConfigSensor`.
- **Manejo de memoria:** Un array dinámico de `EcgStatsPaciente` se asigna en `ExportEcgAnomalousPatients`. Como `id` es fijo, no requiere liberación individual, pero el array principal debe liberarse.
- **Manejo de errores:** No valida overflow en `id` (si excede 10 chars). Asume IDs únicos.
- **Integración:** Usado en `opcion3` para exportar pacientes con anomalías ECG a `.dat`.

👉 Se utiliza para detectar pacientes con lecturas **fuera de los rangos permitidos**.

---

## ⚙️ Funciones implementadas

### `funcion1archivos.hh`

Carga la configuración desde archivo de texto:

```cpp
inline void cargarConfiguracion(const char* nombreArchivo, ArchivoConfiguracion& AC) {
    ifstream archivo(nombreArchivo);
    // Se parsea cada línea: "T,36.0,38.0"
    char toks[5][64];
    size_t n = split_simple(std::string(lineabuffer), ',', toks, 5);
    ConfigSensor cfg{};
    cfg.tipo = new char[strlen(toks[0])+1];
    strcpy(cfg.tipo, toks[0]);
    cfg.min = atof(toks[1]);
    cfg.max = atof(toks[2]);
}
```

**Explicación detallada del funcionamiento:**
- Esta función inline (optimizada para evitar overhead de llamada) lee un archivo de texto como `configuracion.txt` que contiene configuraciones de sensores (e.g., "T,36.0,38.0").
- **Flujo de ejecución:**
  1. Abre el archivo con `ifstream`.
  2. Lee línea por línea usando `getline` en un buffer (`lineabuffer`).
  3. Usa `split_simple` (función custom) para dividir cada línea por comas en un array de tokens (`toks`, max 5).
  4. Crea una instancia temporal de `ConfigSensor` (`cfg`).
  5. Asigna memoria dinámica para `cfg.tipo` (`new char[strlen+1]`), copia el primer token (tipo sensor), y convierte los siguientes tokens a double con `atof` para `min` y `max`.
  6. Para sensores tipo 'P', parsea tokens adicionales para umbrales diastólicos.
  7. Agrega `cfg` a `AC` (probablemente un struct con array dinámico `ConfigSensor* listaDeReglas` y contador).
- **Manejo de memoria:** Cada `cfg.tipo` se asigna dinámicamente y debe liberarse en el destructor de `ArchivoConfiguracion`. El array de configs en `AC` también es dinámico.
- **Manejo de errores:** No valida si el archivo no existe, si `atof` falla (e.g., string no numérico), o si hay menos/más tokens de los esperados. Un archivo malformado podría causar fallos.
- **Integración:** Las configuraciones cargadas se usan en `esLecturaAnomala` y `ExportEcgAnomalousPatients` para validar lecturas.

👉 Genera dinámicamente una lista de `ConfigSensor`.

---

### `funcion2archivos.hh`

Carga pacientes desde CSV:

```cpp
inline void separarLineaEnPaciente(const std::string& lineapac, Pacientes& paciente) {
    char tokens[20][128];
    size_t count = split_simple(lineapac, ';', tokens, 20);

    paciente.tipoDocumento   = clone_cstr(tokens[1]);
    paciente.documento       = clone_cstr(tokens[2]);
    paciente.nombres         = clone_cstr(tokens[3]);
    paciente.apellidos       = clone_cstr(tokens[4]);
}
```

**Explicación detallada del funcionamiento:**
- Esta función inline parsea una línea de un archivo CSV (`pacientes.csv`) en una instancia de `Pacientes`.
- **Flujo de ejecución:**
  1. Toma una línea como `std::string` (`lineapac`).
  2. Usa `split_simple` para dividir la línea por ';' en un array de tokens (`tokens`, max 20, cada uno max 128 chars).
  3. Asigna tokens a los campos de `paciente` usando `clone_cstr` (probablemente un wrapper de `strdup` o `new + strcpy` para duplicar strings).
  4. Asume un orden fijo de columnas (e.g., token[1] = tipoDocumento, token[2] = documento, etc.).
- **Manejo de memoria:** Cada campo de texto (`tipoDocumento`, etc.) se asigna dinámicamente con `clone_cstr` y debe liberarse manualmente en el destructor de `Pacientes`.
- **Manejo de errores:** No valida si `count` es menor al número de campos esperados (podría asignar nulos) ni verifica formatos (e.g., email válido). Un CSV malformado podría causar accesos inválidos.
- **Integración:** Llamada por una función superior que lee el CSV línea por línea y agrega pacientes a `ArchivoPacientes`. Usado para asociar lecturas con pacientes vía `idPaciente`.

👉 Convierte cada línea del archivo `pacientes.csv` en un registro de la estructura `Pacientes`.

---

### `funcionGENERARrep.hh`

Procesa lecturas y genera reportes por paciente:

```cpp
bool esLecturaAnomala(char tipoSensor, double valor, double valorDiastolica, 
                      const ConfigSensor configuraciones[], int totalConfigs, char* descripcion) {
    if (tipoSensor == 'P') {
        bool sistolicaAnomala  = (valor < cfg->min || valor > cfg->max);
        bool diastolicaAnomala = (valorDiastolica < cfg->umbralMinDiastolica || 
                                  valorDiastolica > cfg->umbralMaxDiastolica);
        if (sistolicaAnomala) strcpy(descripcion, "ANOMALA - Sistólica fuera de rango");
    }
}
```

**Explicación detallada del funcionamiento:**
- Esta función booleana determina si una lectura es anómala comparándola con las configuraciones de sensores.
- **Flujo de ejecución:**
  1. Itera sobre `configuraciones` (array de `ConfigSensor`) para encontrar el que coincide con `tipoSensor`.
  2. Para sensores tipo 'P' (presión arterial), verifica si `valor` (sistólica) está fuera de `[min, max]` y si `valorDiastolica` está fuera de `[umbralMinDiastolica, umbralMaxDiastolica]`.
  3. Si hay anomalía, escribe un mensaje en `descripcion` usando `strcpy` (e.g., "ANOMALA - Sistólica fuera de rango").
  4. Retorna `true` si la lectura es anómala, `false` si es normal.
- **Manejo de memoria:** `descripcion` debe ser un buffer preasignado con espacio suficiente. No asigna memoria dinámica.
- **Manejo de errores:** No valida si `configuraciones` es null o si `tipoSensor` no se encuentra (podría causar acceso inválido). Asume `descripcion` tiene espacio suficiente.
- **Integración:** Usada en `generarMedicionesPaciente` para clasificar lecturas y escribir reportes en archivos `.txt`.

👉 Clasifica lecturas como **NORMAL** o **ANOMALA** y escribe reportes detallados.

---

### `Ecg.hh`

Procesa archivos binarios `.bsf` para detectar anomalías en ECG:

```cpp
bool ExportEcgAnomalousPatients(const char* bsf_filename,
                                const char* dat_filename,
                                const ConfigSensor &ecg_config) {
    // PASADA 1: recolectar stats dinámicos
    EcgStatsPaciente* stats = nullptr;
    int res = primeraPasadaBSF_dyn(bsf_filename, stats, usados, cap);

    // Marcar anomalías
    const double umbral = fabs(ecg_config.min) + fabs(ecg_config.max);
    stats[i].anomalo = (sumaAbs > umbral);
}
```

**Explicación detallada del funcionamiento:**
- Esta función exporta pacientes con lecturas ECG anómalas a un archivo `.dat`.
- **Flujo de ejecución:**
  1. **Primera pasada:** Llama `primeraPasadaBSF_dyn` (función custom) para leer el archivo `.bsf` y recolectar estadísticas en un array dinámico `stats` de `EcgStatsPaciente`. Actualiza `usados` (número de slots ocupados) y `cap` (capacidad del array).
  2. Calcula un umbral para ECG como la suma de valores absolutos de `ecg_config.min` y `ecg_config.max`.
  3. Itera sobre `stats`, marcando `anomalo = true` si la suma absoluta de lecturas (`sumaAbs`, calculada en la pasada) excede el umbral.
  4. Escribe pacientes con `anomalo = true` a `dat_filename` (probablemente en modo binario con `ofstream`).
- **Manejo de memoria:** `stats` se asigna dinámicamente en `primeraPasadaBSF_dyn` y debe liberarse tras uso. `id` en `EcgStatsPaciente` es fijo, sin necesidad de liberación individual.
- **Manejo de errores:** No valida si `bsf_filename` existe o si `primeraPasadaBSF_dyn` falla. La lógica de `sumaAbs` no está explícita en el fragmento (probablemente calculada en la pasada).
- **Integración:** Usada en `opcion3`, depende de `ecg_config` cargada desde `configuracion.txt`.

👉 Exporta pacientes con valores ECG fuera de rango a un archivo `.dat`.

---

### `mostrarvoidmenu.hh`

Implementa las opciones del menú:

```cpp
void opcion1() {
    ArchivoConfiguracion AC{};
    cargarConfiguracion("configuracion.txt", AC);
    cargarPacientes("pacientes.csv", AP);
}
```

**Explicación detallada del funcionamiento:**
- Inicializa un `ArchivoConfiguracion` vacío (`AC`) y carga configuraciones desde `configuracion.txt` usando `cargarConfiguracion`.
- Carga pacientes desde `pacientes.csv` usando una función que llena `AP` (probablemente una variable global `ArchivoPacientes`).
- **Flujo:** Ejecuta las dos cargas secuencialmente. Sirve como inicialización del sistema.
- **Manejo de memoria:** `AC` y `AP` deben liberar sus recursos dinámicos (configs y pacientes) al salir del programa.
- **Manejo de errores:** No valida si los archivos existen.

```cpp
void opcion2() {
    ifstream f("GenerarBinarioProfe/patient_readings_simulation.bsf", ios::binary);
    // Lee sala, máquinas, mediciones y lecturas dinámicas
}
```

**Explicación detallada del funcionamiento:**
- Abre un archivo `.bsf` en modo binario.
- Lee estructuras anidadas (`SalaUCI`, `MaquinaUCI`, `Medicion`, `Lectura`) usando `f.read()` para cada campo.
- Asigna memoria dinámica para arrays en cada nivel.
- **Flujo:** Lee header de sala, itera por máquinas, mediciones y lecturas.
- **Manejo de memoria:** Requiere liberación recursiva de arrays.
- **Manejo de errores:** No valida EOF o formato incorrecto.

```cpp
void opcion3() {
    LoadEcgConfig("configuracion.txt", ecg_config);
    ExportEcgAnomalousPatients("patient_readings_simulation_small.bsf", "pacientes_ecg_anomalos.dat", ecg_config);
}
```

**Explicación detallada del funcionamiento:**
- Carga solo la configuración ECG desde `configuracion.txt` en `ecg_config` (probablemente un solo `ConfigSensor`).
- Llama `ExportEcgAnomalousPatients` para procesar `.bsf` y exportar anomalías.
- **Flujo:** Especializado en ECG, ejecuta carga y exportación.
- **Manejo de memoria:** `ecg_config` y recursos de exportación deben liberarse.
- **Manejo de errores:** Depende de funciones subyacentes.

```cpp
void opcion4() {
    // Análisis de lecturas en lecturas.txt
    generarMedicionesPaciente(idPaciente, "lecturas.txt", AC.listaDeReglas, AC.numReglas);
}
```

**Explicación detallada del funcionamiento:**
- Genera un reporte para un paciente específico (`idPaciente`) leyendo lecturas desde `lecturas.txt`.
- Usa `AC.listaDeReglas` para validar lecturas con `esLecturaAnomala`.
- Escribe estadísticas y estados (NORMAL/ANOMALA) a un archivo `.txt`.
- **Flujo:** Lee archivo, valida lecturas, genera reporte.
- **Manejo de memoria:** Depende de `AC` pre-cargado.
- **Manejo de errores:** No valida existencia de archivo o ID.

---

### `main.cpp`

El archivo principal muestra el menú y llama a las opciones:

```cpp
int main() {
    int opcion;
    do {
        mostrarMenu();
        cin >> opcion;
        switch (opcion) {
            case 1: opcion1(); break;
            case 2: opcion2(); break;
            case 3: opcion3(); break;
            case 4: opcion4(); break;
            case 5: salir = true; break;
        }
    } while (!salir);
}
```

**Explicación detallada del funcionamiento:**
- Implementa un bucle interactivo para el menú principal.
- **Flujo de ejecución:**
  1. Llama `mostrarMenu()` (no mostrado, probablemente imprime opciones 1-5).
  2. Lee la opción del usuario con `cin`.
  3. Usa `switch` para despachar a la función correspondiente (`opcion1` a `opcion4`, o salir).
  4. Continúa hasta que `salir` (variable global o local) sea `true`.
- **Manejo de memoria:** No asigna recursos directamente, pero depende de la limpieza en las funciones llamadas.
- **Manejo de errores:** No valida entradas inválidas (e.g., no numéricas), lo que podría causar comportamiento indefinido.
- **Integración:** Punto de entrada que coordina todas las funcionalidades.

---

## 📑 Ejemplos de reportes generados

### Ejemplo de Reporte por Paciente

Archivo: `mediciones_paciente_PAC001.txt`

```txt
=================================================
    REPORTE DE MEDICIONES MÉDICAS
    Paciente: PAC001
=================================================

=== TEMPERATURA ===
Fecha: 15/01/2024 08:30:15.123 | Valor: 36.50 | Estado: NORMAL - Dentro del rango [36.0 - 37.5]
Fecha: 15/01/2024 12:30:22.456 | Valor: 38.20 | Estado: ANOMALA - Fuera del rango [36.0 - 37.5]

--- Estadísticas ---
Mínimo: 36.50, Máximo: 38.20, Promedio: 37.35 (Total: 2 lecturas)

=== PRESIÓN ARTERIAL ===
Fecha: 15/01/2024 08:30:15.123 | Valor: 120/80 | Estado: NORMAL
Fecha: 15/01/2024 12:30:22.456 | Valor: 150/95 | Estado: ANOMALA - Sistólica y Diastólica fuera de rango

--- Estadísticas ---
Presión Sistólica - Mín: 120, Máx: 150, Promedio: 135.0 (Total: 2 lecturas)
Presión Diastólica - Mín: 80, Máx: 95, Promedio: 87.5 (Total: 2 lecturas)

=== OXIGENACIÓN ===
Fecha: 15/01/2024 08:30:15.123 | Valor: 98 | Estado: NORMAL
Fecha: 15/01/2024 12:30:22.456 | Valor: 92 | Estado: ANOMALA - Fuera del rango [95.0 - 100.0]

--- Estadísticas ---
Mínimo: 92.00, Máximo: 98.00, Promedio: 95.00 (Total: 2 lecturas)

=== ELECTROCARDIOGRAMA ===
Fecha: 15/01/2024 12:30:22.456 | Valor: 0.75 | Estado: Sin análisis (ECG)

=================================================
Reporte generado exitosamente.
Total de lecturas procesadas: 7
=================================================
```

---

### Ejemplo de Validación de Anomalías ECG

Archivo: `validation_ecg_anomalies.txt`

```txt
=== VALIDACION DEL ARCHIVO DE ANOMALIAS ECG ===
Limites ECG leidos de configuracion.txt: [-3.858747 , 1.228621]

PACIENTE #1
  ID: PAC001    
  Numero de lecturas: 3
    Lectura 1:
      Fecha/Hora: 15/01/2024 12:30:22.456
      Valor ECG: -4.12 --> ANOMALO
    Lectura 2:
      Fecha/Hora: 15/01/2024 13:15:10.222
      Valor ECG: 0.85 --> NORMAL
    Lectura 3:
      Fecha/Hora: 15/01/2024 14:01:45.987
      Valor ECG: 1.40 --> ANOMALO

PACIENTE #2
  ID: PAC002
  Numero de lecturas: 2
    Lectura 1:
      Fecha/Hora: 15/01/2024 09:45:30.789
      Valor ECG: -2.10 --> NORMAL
    Lectura 2:
      Fecha/Hora: 15/01/2024 10:25:50.123
      Valor ECG: 1.35 --> ANOMALO

=== RESUMEN ===
Pacientes procesados: 2
Total de anomalias: 3
```

---

## 🤖 Uso de Inteligencia Artificial

Durante la elaboración de la documentación se utilizaron conversaciones con ChatGPT para:

* Explicación detallada de los headers (`.hh`).
* Generación del archivo `README.md`.
* Elaboración de este informe con ejemplos de código y reportes simulados.

Utilizando las siguientes prompths
* ¿Cómo leo una línea y la guardo en estructura? 
* ¿Cómo uso la función trim y split en el sprint 1?
* ¿Cómo es la sintaxis de los binarios y cómo los relaciono al proyecto?
* ¿Cómo comparo con el strcmp un arreglo de cadenas con una variable char?
* ¿Cómo creo un binario? 
* ¿Cómo paso un binario a un txt?
* ¿Cómo cambiar todo a dinámico? 
* ¿En qué partes se tiene que liberar memoria?

Autores: Juan Esteban Ocampo & Samuel Andres Aviles Leon
