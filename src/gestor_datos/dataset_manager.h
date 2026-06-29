#ifndef DATASET_MANAGER_H
#define DATASET_MANAGER_H


#include <string>
#include <vector>
#include <queue>
#include <cmath>

#include "../rtree/spatial_object.h"

// Estructura auxiliar para manejar la cola de prioridad en el KNN
struct KNNResult {
    SpatialObject obj;
    double distance;
    
    // Sobrecarga del operador < para que std::priority_queue funcione como un Max-Heap.
    // Esto nos permite mantener siempre los K más cercanos y descartar los lejanos.
    bool operator<(const KNNResult& other) const {
        return distance < other.distance;
    }
};

class DatasetManager {
private:
    // Aquí vivirá toda la información en memoria
    std::vector<SpatialObject> dataset;

    // Función auxiliar matemática para el KNN y comprobaciones
    double calculateDistance(double x1, double y1, double x2, double y2) const {
        return std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
    }

public:
    DatasetManager() = default;
    ~DatasetManager() = default;

    // Lector/Parser de archivos
    // El parámetro limit te servirá para cargar la cantidad de objetos que quieras
    bool loadDataset(const std::string& filepath, int limit = -1);

    // Implementación de la Solución Ingenua (búsqueda lineal por rango)
    std::vector<SpatialObject> linearRangeSearch(double minX, double minY, double maxX, double maxY) const;

    // Algoritmo KNN usando colas de prioridad
    std::vector<SpatialObject> knnSearch(double queryX, double queryY, int k) const;

    const std::vector<SpatialObject>& getDataset() const { return dataset; }
    size_t getDatasetSize() const { return dataset.size(); }
};

#endif // DATASET_MANAGER_H