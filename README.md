# AED Proyecto R-Tree: Buscador Espacial

Este repositorio contiene el proyecto final del curso **CS2023 - Algoritmos y Estructuras de Datos**, enfocado en la investigación, implementación desde cero y evaluación de un **R-Tree (Árbol R)**. La aplicación principal es un **buscador espacial interactivo** que permite realizar consultas por rango rectangular y de los $k$ vecinos más cercanos ($k$-NN) sobre puntos de interés geográficos (POIs), comparando su desempeño contra una búsqueda lineal.

---

## 1. Información del Curso e Integrantes

* **Institución:** Universidad de Ingeniería y Tecnología (UTEC)
* **Curso:** CS2023 - Algoritmos y Estructuras de Datos
* **Sección:** 1
* **Profesor:** Brenner Ojeda
* **Semestre:** 2026-I

### Integrantes del Equipo
1. **Alvarado León, Adriana Celeste** - *Desarrollador Algorítmico / Arquitecto Core*
2. **Kurth Dávila** - *Especialista en Datos y Métricas / Gestor de Datos y KNN*
3. **Estrada Fiestas, Christian Estrada** - *Diseñador de GUI e Interacción / Desarrollador Visual* (GitHub: [Chrissrtx](https://github.com/Chrissrtx))

---

## 2. R-Tree: Descripción y Fundamentos Teóricos

El **R-Tree** es una estructura de datos de acceso espacial basada en árboles balanceados (similar a un B-Tree, pero adaptado para múltiples dimensiones). Fue introducido por Antonin Guttman en 1984 y se utiliza para indexar información espacial multidimensional (como coordenadas geográficas, polígonos y datos geométricos).

### Conceptos Clave y Operaciones

1. **MBR (Minimum Bounding Rectangle / Rectángulo Delimitador Mínimo):**
   Cada nodo intermedio del árbol contiene un MBR que encierra a todos los MBRs de sus nodos hijos. De esta manera, el espacio se subdivide en cajas jerárquicas alineadas con los ejes de coordenadas.
2. **Límites de Capacidad ($m, M$):**
   * Cada nodo (excepto la raíz) debe contener al menos $m$ entradas y como máximo $M$ entradas, garantizando el balanceo del árbol.
3. **Inserción y Split Cuadrático (Quadratic Split):**
   Cuando un nodo supera su capacidad máxima $M$, se debe dividir en dos utilizando la estrategia de **Split Cuadrático**:
   * **Selección de Semillas (Seeds):** Se escogen las dos entradas que, de ser agrupadas juntas, generarían el mayor desperdicio de área (área del MBR conjunto menos las áreas individuales).
   * **Distribución de Entradas:** Las entradas restantes se asignan al grupo que experimente el menor incremento de área del MBR. Si un grupo corre el riesgo de no cumplir con el mínimo de entradas ($m$), se le asignan los elementos restantes automáticamente.
4. **Búsqueda por Rango (Range Query):**
   Permite encontrar todos los objetos contenidos dentro de una ventana de consulta rectangular. El árbol descarta masivamente ramas enteras cuyos MBRs no intersectan con la ventana de consulta.
5. **Búsqueda de $k$ Vecinos Más Cercanos ($k$-NN):**
   Implementado eficientemente mediante una **cola de prioridad** (algoritmo de prioridad guiada), priorizando los nodos con la menor distancia (mínima distancia al MBR) hacia el punto de consulta.

```
                  +--------------------------------+
                  |             MBR 1              |
                  |   +--------+      +--------+   |
                  |   |  Node  |      |  Node  |   |
                  |   |   A    |      |   B    |   |
                  |   +--------+      +--------+   |
                  +--------------------------------+
                                  |
               +------------------+------------------+
               |                                     |
               v                                     v
       +---------------+                     +---------------+
       |     MBR A     |                     |     MBR B     |
       | [P1] [P2] [P3]|                     | [P4] [P5] [P6]|
       +---------------+                     +---------------+
```

---

## 3. Relevancia para el Curso

En el curso de *Algoritmos y Estructuras de Datos*, se estudian estructuras de almacenamiento fundamentales (listas, pilas, colas, tablas hash) y árboles de búsqueda de una sola dimensión (como AVL o Red-Black Trees). El R-Tree representa un salto cualitativo hacia la **indexación multidimensional y espacial**, abordando los siguientes aspectos clave de la formación en Computer Science:

* **Manejo Dinámico del Espacio:** Aprender cómo subdividir y buscar en el espacio multidimensional de forma jerárquica.
* **Optimización de Consultas:** Evitar el escaneo lineal de millones de puntos geográficos (complejidad $O(n)$) y reducir la complejidad temporal a escala logarítmica ($O(\log n)$) en consultas de rango y cercanía.
* **Diseño Algorítmico Avanzado:** Comprender el balanceo multidimensional, la lógica de *split* geométrico, y el uso de colas de prioridad aplicadas a geometría computacional.

---

## 4. Aplicaciones del R-Tree en el Mundo Real

El R-Tree es la columna vertebral de numerosos sistemas y servicios modernos de gran escala:

* **Sistemas de Información Geográfica (GIS):** Indexar y buscar eficientemente rutas, parcelas de tierra, límites municipales y mapas en software como QGIS o ArcGIS.
* **Bases de Datos Espaciales:** Motor de indexación espacial en gestores líderes de la industria como **PostgreSQL (PostGIS)**, **MySQL**, **Oracle Spatial** y **SQLite (SpatiaLite)**.
* **Servicios de Mapas y Delivery:** Búsqueda rápida de conductores cercanos, restaurantes en un radio de entrega, u hospitales de emergencia en aplicaciones como Google Maps, Waze, Uber y Rappi.
* **Visión por Computadora y Gráficos:** Detección de colisiones 2D/3D y aceleración de consultas de visibilidad en motores de videojuegos y software de modelado.

---

## 5. Diseño y Arquitectura de la Aplicación

La aplicación se diseñará bajo la siguiente estructura técnica en C++:

### Estructura del Objeto Espacial
```cpp
struct SpatialObject {
    int id;                  // Identificador único
    std::string name;        // Nombre del lugar/POI
    double x, y;             // Coordenadas espaciales (longitud, latitud)
    std::string category;    // Categoría (parque, hospital, restaurante, etc.)
};
```

### Plan de Distribución de Tareas (Sprint de 6 Días)
1. **Día 1:** Definición de Interfaces, Contratos e Inicialización del repositorio.
2. **Día 2:** Programación de la estructura del nodo del R-Tree, cálculo de MBR e implementación de búsqueda lineal básica y parser de datasets.
3. **Día 3:** Implementación del Split Cuadrático e Inserción dinámica en el R-Tree. Desarrollo del algoritmo $k$-NN con colas de prioridad.
4. **Día 4:** Conexión de componentes, unificación con la interfaz gráfica (utilizando SFML, raylib o Qt) y depuración de punteros/memoria.
5. **Día 5:** Pruebas experimentales y recolección de métricas empíricas con conjuntos de datos crecientes (1,000, 5,000 y 15,000+ puntos de GeoNames Cities o OpenStreetMap).
6. **Día 6:** Redacción del reporte comparativo, análisis de rendimiento y preparación de la exposición final.

---

## 6. Instrucciones de Uso y Compilación

*(Próximamente, al integrar el código fuente del proyecto)*

El proyecto utilizará **CMake** para la gestión de dependencias y la compilación.
