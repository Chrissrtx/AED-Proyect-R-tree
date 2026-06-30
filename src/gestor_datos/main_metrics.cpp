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
    for (const auto& obj : manager.getDataset()) {
        rtree.insert(obj);
    }

    // --- COORDENADAS DE PRUEBA ---
    double minX = 1.0, minY = 40.0, maxX = 2.0, maxY = 43.0;
    double queryX = 1.5, queryY = 42.5;
    int k = 5;
    
    // VARIABLES PARA EL BENCHMARK
    int iteraciones = 100; // Ejecutaremos todo 100 veces
    size_t dummy_sum = 0;  // Para evitar que el compilador ignore el bucle

    // 2. --- CONSULTA: BÚSQUEDA LINEAL POR RANGO ---
    auto start_range = std::chrono::high_resolution_clock::now();
    size_t range_found = 0;
    for(int i = 0; i < iteraciones; i++) {
        auto res = manager.linearRangeSearch(minX, minY, maxX, maxY);
        range_found = res.size();
        dummy_sum += range_found; 
    }
    auto end_range = std::chrono::high_resolution_clock::now();
    double avg_range_linear = std::chrono::duration<double, std::milli>(end_range - start_range).count() / iteraciones;

    std::cout << "[Rango Lineal]    Encontrados: " << range_found << "\n";
    std::cout << "[Rango Lineal]    Tiempo prom: " << avg_range_linear << " ms\n";

    // 3. --- CONSULTA: R-TREE POR RANGO ---
    MBR queryBox(minX, minY, maxX, maxY); 
    auto start_rtree = std::chrono::high_resolution_clock::now();
    size_t rtree_found = 0;
    for(int i = 0; i < iteraciones; i++) {
        auto res = rtree.rangeQuery(queryBox);
        rtree_found = res.size();
        dummy_sum += rtree_found;
    }
    auto end_rtree = std::chrono::high_resolution_clock::now();
    double avg_range_rtree = std::chrono::duration<double, std::milli>(end_rtree - start_rtree).count() / iteraciones;

    std::cout << "[Rango R-Tree]    Encontrados: " << rtree_found << "\n";
    std::cout << "[Rango R-Tree]    Tiempo prom: " << avg_range_rtree << " ms\n\n";

    // 4. --- PRUEBA DE KNN (BÚSQUEDA LINEAL) ---
    auto start_knn_linear = std::chrono::high_resolution_clock::now();
    std::string top1_linear = "";
    for(int i = 0; i < iteraciones; i++) {
        auto res = manager.knnSearch(queryX, queryY, k);
        if(i == 0 && !res.empty()) top1_linear = res[0].name;
        dummy_sum += res.size();
    }
    auto end_knn_linear = std::chrono::high_resolution_clock::now();
    double avg_knn_linear = std::chrono::duration<double, std::milli>(end_knn_linear - start_knn_linear).count() / iteraciones;

    std::cout << "[KNN Lineal] (" << k << " vecinos) Tiempo prom: " << avg_knn_linear << " ms\n";
    std::cout << "Top 1 encontrado: " << top1_linear << "\n";

    // 5. --- PRUEBA DE KNN (R-TREE) ---
    auto start_knn_rtree = std::chrono::high_resolution_clock::now();
    std::string top1_rtree = "";
    for(int i = 0; i < iteraciones; i++) {
        auto res = rtree.knnQuery(queryX, queryY, k);
        if(i == 0 && !res.empty()) top1_rtree = res[0].name;
        dummy_sum += res.size();
    }
    auto end_knn_rtree = std::chrono::high_resolution_clock::now();
    double avg_knn_rtree = std::chrono::duration<double, std::milli>(end_knn_rtree - start_knn_rtree).count() / iteraciones;

    std::cout << "[KNN R-Tree] (" << k << " vecinos) Tiempo prom: " << avg_knn_rtree << " ms\n";
    std::cout << "Top 1 encontrado: " << top1_rtree << "\n";
    
    // 6. --- REPORTE DE RENDIMIENTO ---
    double speedup_range = avg_range_linear / (avg_range_rtree > 0 ? avg_range_rtree : 0.0001);
    double speedup_knn = avg_knn_linear / (avg_knn_rtree > 0 ? avg_knn_rtree : 0.0001);
    
    std::cout << "\n>>> RENDIMIENTO GLOBAL (Promedio de " << iteraciones << " iteraciones) <<<\n";
    std::cout << "* Consulta Rango: El R-Tree fue " << std::fixed << std::setprecision(2) << speedup_range << "x veces mas rapido.\n";
    std::cout << "* Consulta KNN  : El R-Tree fue " << std::fixed << std::setprecision(2) << speedup_knn << "x veces mas rapido.\n";
    
    // Prevención de optimización del compilador (silencioso)
    if (dummy_sum == 0) std::cout << " "; 
}
int main() {
    DatasetManager manager;
    std::string datasetPath = "src/gestor_datos/cities15000.txt"; 

    std::vector<int> testSizes = {1000, 5000, 15000, 100000};

    for (int size : testSizes) {
        if (manager.loadDataset(datasetPath, size)) {
            runExperiments(manager, size);
        }
    }
    return 0;
}