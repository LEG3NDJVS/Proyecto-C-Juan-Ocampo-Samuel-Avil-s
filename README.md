# Sistema de Monitoreo Biomédico

Este proyecto implementa un sistema para la **gestión, análisis y reporte de lecturas médicas** (temperatura, presión arterial, oxigenación y ECG).  
Se usan archivos de configuración, CSV de pacientes y archivos binarios `.bsf` con mediciones simuladas.

---

## 🚀 Compilación y ejecución

### Requisitos
- Compilador **g++** compatible con C++11 o superior.
- Archivos de entrada:
  - `configuracion.txt` → reglas de validación de sensores.
  - `pacientes.csv` → lista de pacientes.
  - `lecturas.txt` → registros de lecturas médicas.
  - `GenerarBinarioProfe/*.bsf` → archivo binario con mediciones (simulación).

### Compilar
Ejecutar en la carpeta del proyecto:

```bash
g++ main.cpp -o monitoreo
```

### Ejecutar
```bash
./monitoreo
```

---

## 📋 Funcionalidades (Menú principal)

Al ejecutar el programa aparece el siguiente menú:

1. **Cargar archivos de configuración y pacientes**  
   - Lee `configuracion.txt` y `pacientes.csv`.  
   - Muestra las reglas de sensores y los pacientes cargados.

2. **Leer archivo de datos (.bsf)**  
   - Procesa archivos binarios `.bsf` con lecturas de una sala UCI.  
   - Despliega la información de máquinas, pacientes y sensores.

3. **Generar reporte de anomalías**  
   - Analiza las lecturas de ECG comparándolas con los umbrales de configuración.  
   - Exporta pacientes anómalos a un archivo `.dat` y crea un informe en texto plano.

4. **Calcular estadísticas y exportar datos procesados**  
   - Analiza todas las lecturas desde `lecturas.txt`.  
   - Genera reportes detallados por paciente con estadísticas (mínimo, máximo, promedio).  

5. **Salir**  
   - Finaliza la ejecución.

---

## 📂 Descripción de los módulos

### `main.cpp`
- Control principal del programa.  
- Muestra el menú y llama a las funciones `opcion1()`, `opcion2()`, `opcion3()`, `opcion4()`.

### `mostrarvoidmenu.hh`
- Implementa las funciones `opcion1` a `opcion4`.  
- **Opción 1:** carga configuraciones y pacientes.  
- **Opción 2:** lee archivos `.bsf` con la estructura de sala UCI.  
- **Opción 3:** procesa anomalías de ECG y genera reportes.  
- **Opción 4:** analiza lecturas generales y crea reportes por paciente.

### `funcion1archivos.hh`
- Funciones para cargar **configuración de sensores** desde `configuracion.txt`.  
- Estructura: `ArchivoConfiguracion` con una lista dinámica de `ConfigSensor`.

### `funcion2archivos.hh`
- Funciones para cargar **pacientes** desde `pacientes.csv`.  
- Manejo de estructuras `Pacientes` y `ArchivoPacientes`.  
- Incluye funciones para liberar memoria asociada.

### `funcionGENERARrep.hh`
- Generación de **reportes detallados por paciente**.  
- Detecta anomalías en lecturas según configuración.  
- Calcula estadísticas (mínimo, máximo, promedio) por sensor.  
- Exporta reportes a archivos `.txt`.

### `Ecg.hh`
- Funciones especializadas para análisis de **ECG** en archivos `.bsf`.  
- Recolecta estadísticas por paciente, detecta anómalos y exporta a `.dat`.  
- Incluye validación y creación de reportes en texto.

---

## 📑 Archivos de entrada esperados

- **`configuracion.txt`**  
  ```
  T,36.0,37.5
  P,90,140,60,90
  O,95,100
  E,-3.858747,1.228621
  ```

- **`pacientes.csv`**  
  ```
  1;CC;123456789;Juan;Pérez;01/01/1990;3001234567;juan@example.com;O+;EPS SaludTotal;Sura
  2;TI;987654321;Ana;Gómez;05/03/1985;3109876543;ana@example.com;A-;EPS Sanitas;
  ```

- **`lecturas.txt`**  
  ```
  PAC001;15/01/2024 08:30:15.123;T:36.5,P:120:80,O:98
  PAC001;15/01/2024 12:30:22.456;T:38.2,P:150:95,O:92,E:0.75
  ```

---

## 👨‍💻 Autores
Juan Esteban Ocampo Arocha --> Ing. Sistemas
Samuel Andres Aviles Leon --> Ing. Sistemas
