# Módulo de Datos, Búsqueda Lineal y KNN

**Responsable:** Integrante 2 (Gestor de Datos y KNN)

**Dependencias:** Requiere los archivos `spatial_object.h`, `mbr.h` y `rtree.h` del Integrante 1 (Core).



## 📁 Arquitectura del Módulo y Explicación de Archivos

Para mantener el código limpio y evitar conflictos con la interfaz gráfica, este módulo aplica una estricta **Separación de Responsabilidades**.


### 1. `dataset_manager.h` (Cabecera / Interfaz)

Actúa como el contrato público del módulo de datos.

* **¿Qué hace?** Define la clase `DatasetManager` que encapsula todo el dataset en memoria (`std::vector<SpatialObject>`). Contiene las firmas de los métodos `loadDataset`, `linearRangeSearch` y `knnSearch`.

* **Detalle Técnico:** Define la estructura auxiliar `KNNResult` con la sobrecarga del operador `<` (necesaria para que la `std::priority_queue` funcione como un Max-Heap) e importa la estructura `SpatialObject` del Integrante 1 para evitar redefiniciones.

### 2. `dataset_manager.cpp` (El Motor Lógico)

Contiene la implementación pesada de los algoritmos matemáticos y de lectura.

* **Parser:** Lee el archivo de texto, lo separa por tabulaciones (`\t`) y mapea los datos ignorando filas corruptas.

* **Búsqueda Lineal:** Recorre todo el vector evaluando matemáticamente si las coordenadas caen dentro de la caja de consulta.

* **KNN:** Calcula las distancias euclidianas y mantiene el Top `K` de los vecinos más cercanos de forma eficiente, descartando automáticamente los más lejanos.

* **👉 NOTA PARA EL INTEGRANTE 3 (Visual):** Este es el **único** archivo `.cpp` de este módulo que necesitas incluir en tu compilación para poder conectar los datos reales con tu interfaz gráfica (junto con su respectivo `.h`).

### 3. `main_metrics.cpp` (El Entorno de Pruebas Empíricas)

Es el punto de entrada exclusivo para la experimentación y medición de tiempos (Entregable del Día 5).

* **¿Qué hace?** Instancia el R-Tree y el Gestor de Datos. Usa `std::chrono` para ejecutar pruebas automatizadas con 1,000, 5,000 y 15,000 objetos. Imprime en consola los tiempos comparativos (en milisegundos) entre la solución ingenua y el R-Tree.

* **🛑 ADVERTENCIA PARA EL INTEGRANTE 3 (Visual):** ¡NO INCLUIR NI COMPILAR ESTE ARCHIVO! Este archivo contiene un `int main()`. Si lo incluyes en tu entorno de SFML/Qt/Raylib, causará un conflicto de "múltiples puntos de entrada". Este archivo es de uso exclusivo para las métricas en consola del Integrante 2.

# ⚙️ Compilación y Ejecución

Para compilar y ejecutar las pruebas de rendimiento (métricas) desde la terminal, ubícate en la raíz del proyecto y ejecuta el siguiente comando:

```bash
g++ -std=c++17 data_metrics/main_metrics.cpp data_metrics/dataset_manager.cpp -o metricas_test
```

Una vez compilado sin errores, ejecuta el programa según tu sistema operativo.

## En Linux / macOS

```bash
./metricas_test
```

## En Windows

```cmd
metricas_test.exe
```