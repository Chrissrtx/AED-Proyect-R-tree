# AED Proyecto R-Tree: Buscador Espacial Interactivo
---

## 1. Información del Curso e Integrantes

*   **Institución:** Universidad de Ingeniería y Tecnología (UTEC)
*   **Curso:** CS2023 - Algoritmos y Estructuras de Datos
*   **Sección:** 1
*   **Profesor:** Brenner Ojeda
*   **Semestre:** 2026-I

### Integrantes del Equipo
1.  **Alvarado León, Adriana Celeste** - *Desarrollador Algorítmico / Arquitecto Core*
2.  **Kurth Dávila** - *Especialista en Datos y Métricas / Gestor de Datos y KNN*
3.  **Estrada Fiestas, Christian Estrada** - *Diseñador de GUI e Interacción / Desarrollador Visual*

---

## 2. Estructura del R-Tree y Justificación

El **R-Tree** es una estructura de datos balanceada de indexación multidimensional que organiza objetos espaciales (en este caso bidimensionales) usando cajas envolventes jerárquicas denominadas **MBR (Minimum Bounding Rectangles)**.

### Características Clave de la Implementación
*   **Cajas MBR:** Almacenan los límites mínimos y máximos en los ejes X e Y (`x_min`, `y_min`, `x_max`, `y_max`) que encierran a un conjunto de puntos o sub-MBRs.
*   **Capacidad de Nodos (Justificación de la Optimización):**
    *   **Configuración Final:** `MAX_ENTRIES = 32` y `MIN_ENTRIES = 16`.
    *   **Justificación:** Inicialmente se probó con una capacidad clásica muy pequeña (máximo 4, mínimo 2). Si bien es útil con fines educativos, esto obligaba al árbol a tener una gran altura (profundidad) y fragmentar en exceso el plano geográfico. Al optimizar los límites a un máximo de **32 elementos**, se redujo drásticamente la altura del árbol, disminuyendo el número de accesos recursivos, minimizando el solapamiento de MBRs y mejorando la fluidez del dibujado en la interfaz gráfica a tasas de refresco óptimas (60 FPS).
*   **Algoritmo de Inserción y Split Cuadrático:** Cuando un nodo excede sus 32 entradas, se realiza un *Split Cuadrático* seleccionando las dos semillas que generen mayor desperdicio de área y distribuyendo el resto de los objetos en el nodo que menos incremente su volumen espacial.
*   **Eliminación y Underflow (Condensación del Árbol):**
    *   Cuando un elemento es borrado mediante `remove(int id)` y su nodo cae por debajo del mínimo de **16 entradas**, se produce un *underflow*.
    *   El algoritmo remueve dicho nodo de la estructura, recalcula las dimensiones de los nodos padres, y recolecta todas las hojas y puntos de la rama eliminada (`orphanObjects`). Posteriormente, estas entradas se reinsertan dinámicamente desde la raíz. Esto garantiza que la densidad espacial del árbol permanezca optimizada y balanceada en todo momento.

---

## 3. Principales Features del Motor Algorítmico

*   **Inserción Dinámica:** Construcción incremental del R-Tree conforme se leen los datos.
*   **Consulta por Rango Rectangular:** Filtrado espacial que descarta ramas enteras cuyos MBRs no intersectan la ventana de consulta ($O(\log n)$ promedio).
*   **Consulta KNN Optimizada:** Búsqueda de los $k$ vecinos más cercanos a través del R-Tree utilizando el algoritmo de prioridad guiada (Best-First Search) implementado con una cola de prioridad Min-Heap.
*   **Eliminación con Condensación por Underflow:** Re-balanceo dinámico de MBRs tras borrar elementos del índice.
*   **Parser Robusto:** Lector de bases de datos geográficas tabuladas (`\t`) preparado para manejar de forma segura entradas corruptas o registros sin coordenadas.

---

## 4. Capacidades de la Interfaz Gráfica (SFML + ImGui)

La aplicación visual unifica el motor de dibujado 2D de **SFML** y los componentes HUD de **Dear ImGui**:

*   **Mapa Geográfico Interactivo:** Dibuja las ciudades de la base de datos como puntos en pantalla. Debido a sus coordenadas de latitud y longitud reales, al alejarse con la cámara se puede observar la silueta planetaria real de los continentes.
*   **Control de Paneo y Zoom:** Paneo fluido del mapa manteniendo pulsado el **Botón Derecho** del mouse y arrastrando, y zoom interactivo mediante la **Rueda del Mouse**.
*   **Consulta por Rango Visual:** Al mantener presionado el **Botón Izquierdo** y arrastrar, se dibuja una caja de selección azul. Al soltarla, las ciudades encontradas se marcan en verde y se muestran las cajas del R-Tree evaluadas.
*   **Consulta KNN Dinámica:** Un clic izquierdo coloca un marcador rojo y dibuja **líneas de conexión verdes** a los vecinos más cercanos. Un slider permite cambiar el valor de $K$ (de 1 a 50) y ver el recálculo al instante.
*   **Visualizador de Estructura de MBRs:** Permite activar/desactivar el dibujado de las divisiones del R-Tree. Los rectángulos están coloreados según su profundidad (Rojo = Raíz, Naranja = Nivel 1, Amarillo = Nivel 2, etc.).
*   **Visualizador del Proceso de Poda:** Destaca en rojo traslúcido las cajas del R-Tree abiertas para buscar elementos, y en gris tenue aquellas inspeccionadas pero podadas, mostrando empíricamente el ahorro de CPU.
*   **Filtros Dinámicos de Categorías:** Checkboxes que permiten activar o desactivar la representación de ciertos tipos de puntos en el mapa en tiempo real.
*   **Carga de Dataset al Vuelo:** Botones en la UI para cargar conjuntos de **1,000, 5,000 o 15,000 puntos**. La interfaz reconstruye el árbol y re-encuadra la cámara automáticamente en el centro geográfico de los datos.

---

## 5. Comparaciones de Rendimiento (Métricas Empíricas)

Pruebas comparativas ejecutadas promediando **100 iteraciones** por consulta (tiempos medidos en milisegundos):

| Tamaño del Dataset | Operación | Tiempo Lineal (Bruta) | Tiempo R-Tree | Aceleración (Speedup) |
| :--- | :--- | :--- | :--- | :--- |
| **1,000 objetos** | Rango <br> KNN (k=5) | 0.0047 ms <br> 0.4600 ms | 0.0007 ms <br> 0.0275 ms | **6.66x** más rápido <br> **16.72x** más rápido |
| **5,000 objetos** | Rango <br> KNN (k=5) | 0.0200 ms <br> 2.5200 ms | 0.0012 ms <br> 0.0600 ms | **15.97x** más rápido <br> **39.60x** más rápido |
| **15,000 objetos** | Rango <br> KNN (k=5) | 0.0700 ms <br> 7.2700 ms | 0.0100 ms <br> 0.1400 ms | **7.60x** más rápido <br> **52.22x** más rápido |
| **100,000 objetos** | Rango <br> KNN (k=5) | 0.1500 ms <br> 19.7900 ms | 0.0100 ms <br> 0.4500 ms | **20.16x** más rápido <br> **43.57x** más rápido |

*Nota: La búsqueda KNN sobre el R-Tree demuestra el mayor crecimiento en eficiencia, llegando a ser hasta **52 veces más rápida** que el escaneo lineal de todo el dataset.*

---

## 6. Instrucciones de Compilación y Ejecución

El proyecto utiliza **CMake** y el gestor de descargas de dependencias `FetchContent`, lo que significa que **no requieres instalar localmente SFML ni ImGui**; el proceso de compilación las descargará y enlazará de manera automática.

### Requisitos Previos
*   **Compilador C++:** Soporte para C++17 (se recomienda GCC/MinGW-w64 en Windows a través de MSYS2, o GCC en Linux/macOS).
*   **Herramientas de construcción:** CMake (Versión >= 3.14) y Make/Ninja.

### Compilación (Paso a Paso en Consola)
Desde la raíz del proyecto, ejecuta:

1.  Crear y entrar al directorio de compilación:
    ```bash
    mkdir build
    cd build
    ```
2.  Configurar el proyecto con CMake (especificando MinGW si estás en Windows con MSYS2):
    ```bash
    cmake -G "MinGW Makefiles" ..
    ```
3.  Compilar el proyecto:
    ```bash
    cmake --build .
    ```

### Ejecución
Una vez compilado correctamente:
1.  **Copiar DLLs (Solo en Windows):** Asegúrate de copiar las librerías dinámicas DLL generadas en los subdirectorios de dependencias hacia el mismo directorio de los ejecutables en `build/`. Puedes hacerlo mediante terminal:
    ```powershell
    copy _deps/imgui-sfml-build/libImGui-SFML.dll .
    copy _deps/sfml-build/lib/*.dll .
    ```
2.  **Lanzar la interfaz gráfica:**
    ```bash
    ./rtree_gui.exe
    ```
3.  **Lanzar las pruebas de métricas en consola:**
    ```bash
    ./metricas_test.exe
    ```
