#include "dataset_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm> 

bool DatasetManager::loadDataset(const std::string& filepath, int limit) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << filepath << std::endl;
        return false;
    }

    std::string line;
    int count = 0;
    dataset.clear(); 

    while (std::getline(file, line)) {
        if (limit != -1 && count >= limit) break;

        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> tokens;
        
        // El parser por tabulaciones
        while (std::getline(ss, item, '\t')) {
            tokens.push_back(item);
        }

        if (tokens.size() >= 8) {
            try {
                SpatialObject obj;
                obj.id = std::stoi(tokens[0]);
                obj.name = tokens[1];
                obj.y = std::stod(tokens[4]); // Latitud
                obj.x = std::stod(tokens[5]); // Longitud
                obj.category = tokens[7];
                
                dataset.push_back(obj);
                count++;
            } catch (...) {
                // Ignorar filas con datos faltantes o errores de formato
            }
        }
    }
    return true;
}

// Implementación de la Solución Ingenua (Búsqueda Lineal)
std::vector<SpatialObject> DatasetManager::linearRangeSearch(double minX, double minY, double maxX, double maxY) const {
    std::vector<SpatialObject> results;
    
    // Recorremos TODO el dataset evaluando si cae dentro de la caja
    for (const auto& obj : dataset) {
        if (obj.x >= minX && obj.x <= maxX && obj.y >= minY && obj.y <= maxY) {
            results.push_back(obj);
        }
    }
    return results;
}

// Algoritmo KNN con Colas de Prioridad
std::vector<SpatialObject> DatasetManager::knnSearch(double queryX, double queryY, int k) const {
    // La cola actuará como un Max-Heap gracias al operator< en el .h
    std::priority_queue<KNNResult> pq;

    for (const auto& obj : dataset) {
        double dist = calculateDistance(obj.x, obj.y, queryX, queryY);
        pq.push({obj, dist});
        
        // Si excedemos K, sacamos el elemento más alejado (que siempre queda arriba en el Max-Heap)
        if (pq.size() > (size_t)k) {
            pq.pop(); 
        }
    }

    // Extraemos los resultados de la cola
    std::vector<SpatialObject> results;
    while (!pq.empty()) {
        results.push_back(pq.top().obj);
        pq.pop();
    }
    
    // Como salen del más lejano al más cercano, lo invertimos
    std::reverse(results.begin(), results.end());
    
    return results;
}