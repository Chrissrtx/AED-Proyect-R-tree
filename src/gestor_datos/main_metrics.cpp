#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>


#include "dataset_manager.h"          
#include "../rtree/rtree.h"    
#include "../rtree/mbr.h"       

void runExperiments(DatasetManager& manager, int dataSize) {
    std::cout << "\n============================================\n";
    std::cout << "EJECUTANDO PRUEBAS CON " << dataSize << " OBJETOS\n";
    std::cout << "============================================\n";

    // 1. --- TIEMPO DE CONSTRUCCIÓN DEL R-TREE ---
    RTree rtree;
    auto start_build = std::chrono::high_resolution_clock::now();
    
    // Inyectamos todos los datos leídos por tu parser al árbol usando su método insert 
    for (const auto& obj : manager.getDataset()) {
        rtree.insert(obj);
    }
    
    auto end_build = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_build = end_build - start_build;
    std::cout << "[R-Tree] Tiempo de construccion: " << duration_build.count() << " ms\n\n";

    // --- COORDENADAS DE PRUEBA ---
    double minX = 1.0, minY = 40.0, maxX = 2.0, maxY = 43.0;

    // 2. --- CONSULTA: BÚSQUEDA LINEAL (Tu código) ---
    auto start_range = std::chrono::high_resolution_clock::now();
    auto range_results = manager.linearRangeSearch(minX, minY, maxX, maxY);
    auto end_range = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_range = end_range - start_range;

    std::cout << "[Busqueda Lineal] Encontrados: " << range_results.size() << "\n";
    std::cout << "[Busqueda Lineal] Tiempo:      " << duration_range.count() << " ms\n";

    // 3. --- CONSULTA: R-TREE ---
    MBR queryBox(minX, minY, maxX, maxY); 
    
    auto start_rtree = std::chrono::high_resolution_clock::now();
    auto rtree_results = rtree.rangeQuery(queryBox);
    auto end_rtree = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_rtree = end_rtree - start_rtree;

    std::cout << "[R-Tree]          Encontrados: " << rtree_results.size() << "\n";
    std::cout << "[R-Tree]          Tiempo:      " << duration_rtree.count() << " ms\n\n";

    // 4. --- PRUEBA DE KNN ---
    double queryX = 1.5, queryY = 42.5;
    int k = 5;

    auto start_knn = std::chrono::high_resolution_clock::now();
    auto knn_results = manager.knnSearch(queryX, queryY, k);
    auto end_knn = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_knn = end_knn - start_knn;

    std::cout << "[KNN (" << k << " vecinos)] Tiempo global: " << duration_knn.count() << " ms\n";
    std::cout << "Top 1 encontrado: " << (knn_results.empty() ? "N/A" : knn_results[0].name) << "\n";
    
    // Cálculo rápido de cuánto más veloz es el árbol frente a la solución ingenua
    double speedup = duration_range.count() / (duration_rtree.count() > 0 ? duration_rtree.count() : 0.001);
    std::cout << "\n>>> RENDIMIENTO: El R-Tree fue " << std::fixed << std::setprecision(2) << speedup << "x veces mas rapido en la consulta por rango. <<<\n";
}

int main() {
    DatasetManager manager;
    std::string datasetPath = "cities15000.txt"; 

    std::vector<int> testSizes = {1000, 5000, 15000};

    for (int size : testSizes) {
        if (manager.loadDataset(datasetPath, size)) {
            runExperiments(manager, size);
        }
    }
    return 0;
}