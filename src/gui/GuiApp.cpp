#include "GuiApp.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <algorithm>

GuiApp::GuiApp() : window(sf::VideoMode(1200, 800), "AED R-Tree Spatial Engine - SFML & ImGui") {
    window.setFramerateLimit(60);

    // Inicializar ImGui-SFML
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Error al inicializar ImGui-SFML" << std::endl;
    }

    // Configurar tema oscuro para ImGui (aspecto moderno y clasico a la vez)
    ImGui::StyleColorsDark();

    // Cargar la fuente tuffy.ttf
    if (font.loadFromFile("tuffy.ttf")) {
        fontLoaded = true;
    } else {
        std::cerr << "No se pudo cargar la fuente tuffy.ttf. Las etiquetas de texto no se renderizaran." << std::endl;
    }

    // Cargar el dataset inicial (1000 elementos)
    cargarDataset(1000);
}

GuiApp::~GuiApp() {
    ImGui::SFML::Shutdown();
}

void GuiApp::cargarDataset(int size) {
    std::string datasetPath = "src/gestor_datos/cities15000.txt";
    
    // Cargar archivo
    if (!datasetManager.loadDataset(datasetPath, size)) {
        std::cerr << "Error al cargar dataset de tamaño " << size << std::endl;
        return;
    }
    
    datasetSize = size;
    currentLoadedSize = (int)datasetManager.getDatasetSize();

    // Reconstruir R-Tree
    rtree = RTree(); // Resetear
    auto start_build = std::chrono::high_resolution_clock::now();
    for (const auto& obj : datasetManager.getDataset()) {
        rtree.insert(obj);
    }
    auto end_build = std::chrono::high_resolution_clock::now();
    rtreeBuildTimeMs = std::chrono::duration<double, std::milli>(end_build - start_build).count();

    // Extraer categorias unicas
    categories.clear();
    selectedCategories.clear();
    std::set<std::string> uniqueCats;
    for (const auto& obj : datasetManager.getDataset()) {
        uniqueCats.insert(obj.category);
    }
    for (const auto& cat : uniqueCats) {
        categories.push_back(cat);
        selectedCategories[cat] = true; // Activo por defecto
    }

    // Obtener estructura del arbol
    rtreeStructure = rtree.getStructure();

    // Limpiar busquedas previas
    queryResults.clear();
    visitedNodes.clear();
    hasKnnPoint = false;
    isDrawingRange = false;

    // Resetear metricas
    rtreeQueryTimeMs = 0.0;
    linearQueryTimeMs = 0.0;
    speedup = 0.0;
    rtreeVisitedNodesCount = 0;
    linearVisitedPointsCount = 0;

    // Adaptar la camara para mostrar los nuevos datos
    encuadrarDatos();
}

void GuiApp::encuadrarDatos() {
    if (datasetManager.getDataset().empty()) {
        mapView.setCenter(MAP_WIDTH / 2.0f, MAP_HEIGHT / 2.0f);
        mapView.setSize(MAP_WIDTH, MAP_HEIGHT);
        window.setView(mapView);
        return;
    }

    double minLon = 180.0, maxLon = -180.0;
    double minLat = 90.0, maxLat = -90.0;

    for (const auto& obj : datasetManager.getDataset()) {
        if (obj.x < minLon) minLon = obj.x;
        if (obj.x > maxLon) maxLon = obj.x;
        if (obj.y < minLat) minLat = obj.y;
        if (obj.y > maxLat) maxLat = obj.y;
    }

    sf::Vector2f pMin = projectCoord(minLon, maxLat); // Latitud max -> Y min (arriba)
    sf::Vector2f pMax = projectCoord(maxLon, minLat); // Latitud min -> Y max (abajo)

    float width = std::abs(pMax.x - pMin.x);
    float height = std::abs(pMax.y - pMin.y);

    // Padding
    width = std::max(width, 100.0f) * 1.2f;
    height = std::max(height, 100.0f) * 1.2f;

    sf::Vector2f center(pMin.x + (pMax.x - pMin.x) / 2.0f, pMin.y + (pMax.y - pMin.y) / 2.0f);

    mapView.setCenter(center);
    mapView.setSize(width, height);
    window.setView(mapView);
}

sf::Vector2f GuiApp::projectCoord(double lon, double lat) const {
    // Proyeccion Equirectangular simple
    // Longitud [-180, 180] -> [0, MAP_WIDTH]
    // Latitud [-90, 90] -> [MAP_HEIGHT, 0] (Y invertido en SFML)
    float x = (float)((lon + 180.0) / 360.0 * MAP_WIDTH);
    float y = (float)((90.0 - lat) / 180.0 * MAP_HEIGHT);
    return sf::Vector2f(x, y);
}

sf::Vector2f GuiApp::unprojectCoord(sf::Vector2f screenPos) const {
    double lon = (screenPos.x / MAP_WIDTH) * 360.0 - 180.0;
    double lat = 90.0 - (screenPos.y / MAP_HEIGHT) * 180.0;
    return sf::Vector2f((float)lon, (float)lat);
}

void GuiApp::ejecutar() {
    sf::Clock clock;
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();
        procesarEventos();
        actualizar(deltaTime);
        renderizar();
    }
}

void GuiApp::procesarEventos() {
    sf::Event event;
    while (window.pollEvent(event)) {
        ImGui::SFML::ProcessEvent(window, event);

        if (event.type == sf::Event::Closed) {
            window.close();
        }

        // Si el cursor esta sobre un elemento de ImGui, ignorar eventos del mapa
        if (ImGui::GetIO().WantCaptureMouse) {
            isDrawingRange = false;
            isPanning = false;
            continue;
        }

        // Zoom con la rueda del mouse
        if (event.type == sf::Event::MouseWheelScrolled) {
            if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                float zoomFactor = (event.mouseWheelScroll.delta > 0) ? 0.85f : 1.15f;
                mapView.zoom(zoomFactor);
                window.setView(mapView);
            }
        }

        // Paneo del mapa (Boton Derecho del Mouse)
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Right) {
                isPanning = true;
                lastMousePos = sf::Mouse::getPosition(window);
            }
        }
        if (event.type == sf::Event::MouseButtonReleased) {
            if (event.mouseButton.button == sf::Mouse::Right) {
                isPanning = false;
            }
        }
        if (event.type == sf::Event::MouseMoved) {
            if (isPanning) {
                sf::Vector2i currentMousePos = sf::Mouse::getPosition(window);
                sf::Vector2f lastWorldPos = window.mapPixelToCoords(lastMousePos, mapView);
                sf::Vector2f currentWorldPos = window.mapPixelToCoords(currentMousePos, mapView);
                sf::Vector2f delta = lastWorldPos - currentWorldPos;
                
                mapView.move(delta);
                window.setView(mapView);
                lastMousePos = currentMousePos;
            }
        }

        // Logica de consultas interactiva (Boton Izquierdo)
        if (queryType == "range") {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                isDrawingRange = true;
                rangeStartPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), mapView);
                rangeEndPos = rangeStartPos;
            }
            else if (event.type == sf::Event::MouseMoved && isDrawingRange) {
                rangeEndPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), mapView);
            }
            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && isDrawingRange) {
                isDrawingRange = false;
                rangeEndPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), mapView);

                // Calcular caja en coordenadas geograficas
                sf::Vector2f geoStart = unprojectCoord(rangeStartPos);
                sf::Vector2f geoEnd = unprojectCoord(rangeEndPos);

                double minX = std::min(geoStart.x, geoEnd.x);
                double maxX = std::max(geoStart.x, geoEnd.x);
                double minY = std::min(geoStart.y, geoEnd.y);
                double maxY = std::max(geoStart.y, geoEnd.y);

                ejecutarConsultaRango(minX, minY, maxX, maxY);
            }
        }
        else if (queryType == "knn") {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                hasKnnPoint = true;
                knnPoint = window.mapPixelToCoords(sf::Mouse::getPosition(window), mapView);
                
                sf::Vector2f geoPos = unprojectCoord(knnPoint);
                ejecutarConsultaKNN(geoPos.x, geoPos.y, kValue);
            }
        }
    }
}

void GuiApp::ejecutarConsultaRango(double minX, double minY, double maxX, double maxY) {
    MBR queryBox(minX, minY, maxX, maxY);

    // Consulta en el R-Tree
    visitedNodes.clear();
    auto start_rtree = std::chrono::high_resolution_clock::now();
    queryResults = rtree.rangeQueryWithVisited(queryBox, visitedNodes);
    auto end_rtree = std::chrono::high_resolution_clock::now();
    rtreeQueryTimeMs = std::chrono::duration<double, std::milli>(end_rtree - start_rtree).count();
    rtreeVisitedNodesCount = (int)visitedNodes.size();

    // Consulta lineal (fuerza bruta)
    auto start_linear = std::chrono::high_resolution_clock::now();
    auto linearResults = datasetManager.linearRangeSearch(minX, minY, maxX, maxY);
    auto end_linear = std::chrono::high_resolution_clock::now();
    linearQueryTimeMs = std::chrono::duration<double, std::milli>(end_linear - start_linear).count();
    linearVisitedPointsCount = (int)datasetManager.getDatasetSize();

    // Calcular aceleracion
    speedup = linearQueryTimeMs / (rtreeQueryTimeMs > 0 ? rtreeQueryTimeMs : 0.0001);
}

void GuiApp::ejecutarConsultaKNN(double x, double y, int k) {
    // 1. Ejecutar consulta KNN en el R-Tree (implementado por Adriana)
    auto start_rtree = std::chrono::high_resolution_clock::now();
    queryResults = rtree.knnQuery(x, y, k);
    auto end_rtree = std::chrono::high_resolution_clock::now();
    rtreeQueryTimeMs = std::chrono::duration<double, std::milli>(end_rtree - start_rtree).count();
    rtreeVisitedNodesCount = 0; // No se almacena el conteo en esta versión

    // 2. Ejecutar búsqueda KNN lineal (implementado por Kurth)
    auto start_linear = std::chrono::high_resolution_clock::now();
    auto linear_results = datasetManager.knnSearch(x, y, k);
    auto end_linear = std::chrono::high_resolution_clock::now();
    linearQueryTimeMs = std::chrono::duration<double, std::milli>(end_linear - start_linear).count();
    linearVisitedPointsCount = (int)datasetManager.getDatasetSize();

    // Calcular aceleración real
    speedup = linearQueryTimeMs / (rtreeQueryTimeMs > 0 ? rtreeQueryTimeMs : 0.0001);
}

void GuiApp::actualizar(sf::Time deltaTime) {
    ImGui::SFML::Update(window, deltaTime);
    dibujarControlesImGui();
}

void GuiApp::dibujarControlesImGui() {
    // Posicionar el panel de control a la izquierda
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(340, 780), ImGuiCond_Always);

    ImGui::Begin("Panel de Control R-Tree", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("CS2023 - Algoritmos y Estructuras de Datos");
    ImGui::Separator();
    ImGui::Spacing();

    // 1. Seleccion de Dataset
    if (ImGui::CollapsingHeader("Conjunto de Datos", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Cantidad de ciudades a cargar:");
        if (ImGui::RadioButton("1,000 Puntos", datasetSize == 1000)) {
            cargarDataset(1000);
        }
        if (ImGui::RadioButton("5,000 Puntos", datasetSize == 5000)) {
            cargarDataset(5000);
        }
        if (ImGui::RadioButton("15,000 Puntos", datasetSize == 15000)) {
            cargarDataset(15000);
        }
        ImGui::TextDisabled("Cargados en memoria: %d", currentLoadedSize);
        ImGui::TextDisabled("Tiempo constr. R-Tree: %.3f ms", rtreeBuildTimeMs);
    }

    ImGui::Spacing();

    // 2. Modo de Consulta
    if (ImGui::CollapsingHeader("Operación de Búsqueda", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::RadioButton("Consulta por Rango", queryType == "range")) {
            queryType = "range";
            queryResults.clear();
            visitedNodes.clear();
            hasKnnPoint = false;
        }
        if (ImGui::RadioButton("K-Vecinos Cercanos (K-NN)", queryType == "knn")) {
            queryType = "knn";
            queryResults.clear();
            visitedNodes.clear();
            hasKnnPoint = false;
        }

        ImGui::Spacing();

        if (queryType == "range") {
            ImGui::TextWrapped("Instrucciones: Haz CLIC IZQUIERDO y arrastra en el canvas para dibujar un rectangulo.");
        } else {
            ImGui::TextWrapped("Instrucciones: Haz CLIC IZQUIERDO en el canvas para colocar el punto de consulta.");
            ImGui::SliderInt("Vecinos (K)", &kValue, 1, 50);
            if (hasKnnPoint) {
                // Actualizar inmediatamente al mover el slider
                sf::Vector2f geoPos = unprojectCoord(knnPoint);
                if (ImGui::IsItemEdited()) {
                    ejecutarConsultaKNN(geoPos.x, geoPos.y, kValue);
                }
            }
        }
    }

    ImGui::Spacing();

    // 3. Opciones de Capas
    if (ImGui::CollapsingHeader("Visualización y Filtros", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Mostrar MBRs de R-Tree", &showRTreeStructure);
        ImGui::Checkbox("Mostrar Nodos Inspeccionados", &showVisitedNodes);
        
        ImGui::Spacing();
        ImGui::Text("Filtros de Categoría (Geográfica):");
        
        ImGui::BeginChild("CategoryFilterList", ImVec2(0, 120), true);
        for (const auto& cat : categories) {
            bool currentVal = selectedCategories[cat];
            if (ImGui::Checkbox(cat.c_str(), &currentVal)) {
                selectedCategories[cat] = currentVal;
            }
        }
        ImGui::EndChild();
    }

    ImGui::Spacing();

    // 4. Panel de Metricas
    if (ImGui::CollapsingHeader("Métricas Comparativas", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Tiempo de Búsqueda:");
        ImGui::BulletText("R-Tree:  %.4f ms", rtreeQueryTimeMs);
        ImGui::BulletText("Lineal:  %.4f ms", linearQueryTimeMs);

        if (queryType == "knn") {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Búsqueda KNN por R-Tree");
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "y Lineal operativas.");
        }

        ImGui::Spacing();
        
        if (speedup > 1.0) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Aceleración: %.2fx más rápido", speedup);
        } else if (speedup > 0.0) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Aceleración: %.2fx", speedup);
        } else {
            ImGui::Text("Aceleración: --");
        }

        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Carga Evaluada:");
        if (queryType == "range") {
            ImGui::BulletText("Nodos visitados (R-Tree): %d", rtreeVisitedNodesCount);
        } else {
            ImGui::BulletText("Nodos visitados (R-Tree): N/D");
        }
        ImGui::BulletText("Puntos evaluados (Lineal): %d", linearVisitedPointsCount);
        ImGui::BulletText("Encontrados: %d", (int)queryResults.size());
    }

    ImGui::End();
}

void GuiApp::renderizar() {
    window.clear(sf::Color(18, 18, 24)); // Fondo oscuro premium

    // Aplicar camara del mapa
    window.setView(mapView);

    // Colores para representar niveles del R-Tree
    sf::Color colors[] = {
        sf::Color(239, 68, 68, 150),   // Rojo (Raiz)
        sf::Color(245, 158, 11, 150),  // Naranja
        sf::Color(16, 185, 129, 150),  // Verde esmeralda
        sf::Color(59, 130, 246, 150),  // Azul electrico
        sf::Color(139, 92, 246, 150),  // Violeta
        sf::Color(236, 72, 153, 150)   // Rosado
    };

    // 1. Dibujar nodos visitados durante la busqueda (Range Query)
    if (showVisitedNodes && queryType == "range") {
        for (const auto& node : visitedNodes) {
            sf::Vector2f pMin = projectCoord(node.mbr.x_min, node.mbr.y_max); // Lat max -> Y min
            sf::Vector2f pMax = projectCoord(node.mbr.x_max, node.mbr.y_min); // Lat min -> Y max

            sf::RectangleShape rect;
            rect.setPosition(pMin);
            rect.setSize(pMax - pMin);
            
            if (node.intersected) {
                rect.setFillColor(sf::Color(239, 68, 68, 30)); // Rojo traslucido si intersecta
                rect.setOutlineColor(sf::Color(239, 68, 68, 100));
            } else {
                rect.setFillColor(sf::Color(107, 114, 128, 15)); // Gris si se descarta/poda
                rect.setOutlineColor(sf::Color(107, 114, 128, 40));
            }
            rect.setOutlineThickness(1.0f);
            window.draw(rect);
        }
    }

    // 2. Dibujar estructura completa del R-Tree MBR
    if (showRTreeStructure) {
        for (const auto& node : rtreeStructure) {
            sf::Vector2f pMin = projectCoord(node.mbr.x_min, node.mbr.y_max);
            sf::Vector2f pMax = projectCoord(node.mbr.x_max, node.mbr.y_min);

            sf::RectangleShape rect;
            rect.setPosition(pMin);
            rect.setSize(pMax - pMin);
            rect.setFillColor(sf::Color::Transparent);

            sf::Color color = colors[node.depth % 6];
            if (node.isLeaf) {
                // Hojas delineadas punteadas o mas claras
                color.a = 70;
                rect.setOutlineThickness(0.8f);
            } else {
                rect.setOutlineThickness(1.5f);
            }
            rect.setOutlineColor(color);
            window.draw(rect);
        }
    }

    // 3. Dibujar todos los puntos del dataset en color apagado
    for (const auto& obj : datasetManager.getDataset()) {
        // Filtrar por categoria activa en ImGui
        auto it = selectedCategories.find(obj.category);
        if (it != selectedCategories.end() && !it->second) {
            continue; 
        }

        sf::Vector2f pos = projectCoord(obj.x, obj.y);
        sf::CircleShape point(1.5f);
        point.setOrigin(1.5f, 1.5f);
        point.setPosition(pos);
        point.setFillColor(sf::Color(100, 116, 139, 180)); // Gris azulado discreto
        window.draw(point);
    }

    // 4. Dibujar los puntos que cumplen con el filtro de consulta (Resultados en verde)
    for (const auto& obj : queryResults) {
        sf::Vector2f pos = projectCoord(obj.x, obj.y);
        sf::CircleShape point(4.0f);
        point.setOrigin(4.0f, 4.0f);
        point.setPosition(pos);
        point.setFillColor(sf::Color(34, 197, 94)); // Verde brillante
        point.setOutlineColor(sf::Color::White);
        point.setOutlineThickness(1.0f);
        window.draw(point);

        // Si se hace zoom suficiente, dibujar etiquetas de texto para las ciudades encontradas
        float zoomFactor = mapView.getSize().x / window.getSize().x;
        if (fontLoaded && zoomFactor < 0.15f) {
            sf::Text text;
            text.setFont(font);
            text.setString(obj.name + " (" + obj.category + ")");
            text.setCharacterSize(10);
            text.setFillColor(sf::Color::White);
            
            // Sombra
            text.setOutlineColor(sf::Color::Black);
            text.setOutlineThickness(1.5f);
            
            text.setPosition(pos.x + 8.0f, pos.y - 6.0f);
            window.draw(text);
        }
    }

    // 5. Dibujar la accion activa (Arrastre de recuadro o punto de KNN)
    if (queryType == "range" && isDrawingRange) {
        sf::RectangleShape selection;
        selection.setPosition(rangeStartPos);
        selection.setSize(rangeEndPos - rangeStartPos);
        selection.setFillColor(sf::Color(59, 130, 246, 30)); // Azul traslucido
        selection.setOutlineColor(sf::Color(59, 130, 246));   // Borde azul
        selection.setOutlineThickness(1.5f);
        window.draw(selection);
    } 
    else if (queryType == "knn" && hasKnnPoint) {
        // Dibujar el punto central de consulta KNN
        sf::CircleShape centralPoint(6.0f);
        centralPoint.setOrigin(6.0f, 6.0f);
        centralPoint.setPosition(knnPoint);
        centralPoint.setFillColor(sf::Color(239, 68, 68)); // Rojo brillante
        centralPoint.setOutlineColor(sf::Color::White);
        centralPoint.setOutlineThickness(1.5f);
        window.draw(centralPoint);

        // Dibujar lineas que unen el centro de la consulta con cada vecino encontrado
        for (const auto& obj : queryResults) {
            sf::Vector2f dest = projectCoord(obj.x, obj.y);
            sf::Vertex line[] = {
                sf::Vertex(knnPoint, sf::Color(239, 68, 68, 180)),
                sf::Vertex(dest, sf::Color(34, 197, 94, 180))
            };
            window.draw(line, 2, sf::Lines);
        }
    }

    // Dibujar interfaz de ImGui
    window.setView(window.getDefaultView());
    ImGui::SFML::Render(window);

    window.display();
}
