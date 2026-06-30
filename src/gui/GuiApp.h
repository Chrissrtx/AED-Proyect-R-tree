#ifndef GUIAPP_H
#define GUIAPP_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <map>
#include <set>
#include "../gestor_datos/dataset_manager.h"
#include "../rtree/rtree.h"

class GuiApp {
private:
    sf::RenderWindow window;
    sf::View mapView;
    sf::Font font;
    bool fontLoaded = false;

    // Gestores de datos
    DatasetManager datasetManager;
    RTree rtree;

    // Configuración y estado de la simulación
    int datasetSize = 1000;
    int currentLoadedSize = 0;
    std::string queryType = "range"; // "range" o "knn"
    int kValue = 5;

    // Parámetros de visualización
    bool showRTreeStructure = true;
    bool showVisitedNodes = true;

    // Filtros de categorías
    std::vector<std::string> categories;
    std::map<std::string, bool> selectedCategories;

    // Estado del mouse e interacción
    bool isDrawingRange = false;
    sf::Vector2f rangeStartPos;
    sf::Vector2f rangeEndPos;
    bool hasKnnPoint = false;
    sf::Vector2f knnPoint;

    // Estado del arrastre para paneo (botón derecho)
    bool isPanning = false;
    sf::Vector2i lastMousePos;

    // Resultados de la última consulta
    std::vector<SpatialObject> queryResults;
    std::vector<RTree::VisitedNodeInfo> visitedNodes;
    std::vector<RTree::RTreeStructureNode> rtreeStructure;

    // Métricas del rendimiento
    double rtreeBuildTimeMs = 0.0;
    double rtreeQueryTimeMs = 0.0;
    double linearQueryTimeMs = 0.0;
    int rtreeVisitedNodesCount = 0;
    int linearVisitedPointsCount = 0;
    double speedup = 0.0;

    // Dimensiones lógicas de proyección
    const float MAP_WIDTH = 2000.0f;
    const float MAP_HEIGHT = 1000.0f;

    // Métodos del ciclo de vida
    void procesarEventos();
    void actualizar(sf::Time deltaTime);
    void renderizar();

    // Dibujo de paneles con ImGui
    void dibujarControlesImGui();

    // Proyecciones geográfica <-> Pantalla
    sf::Vector2f projectCoord(double lon, double lat) const;
    sf::Vector2f unprojectCoord(sf::Vector2f screenPos) const;

    // Carga de datos y construcción del árbol
    void cargarDataset(int size);

    // Consultas
    void ejecutarConsultaRango(double minX, double minY, double maxX, double maxY);
    void ejecutarConsultaKNN(double x, double y, int k);

    // Ajustar vista para encuadrar los datos
    void encuadrarDatos();

public:
    GuiApp();
    ~GuiApp();

    void ejecutar();
};

#endif // GUIAPP_H
