cmake_minimum_required(VERSION 3.14)
project(RTreeBuscadorEspacial)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Evitar la compilación de ejemplos y documentación en las dependencias
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_INSTALL OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

include(FetchContent)

# 1. Descargar SFML (Versión 2.6.1 estable)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
)
FetchContent_MakeAvailable(SFML)

# 2. Descargar Dear ImGui (Versión estable v1.90.4)
FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)
FetchContent_MakeAvailable(ImGui)

# 3. Descargar ImGui-SFML (Binding compatible con SFML 2.6)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v2.6
)
FetchContent_MakeAvailable(ImGui-SFML)

# Executable 1: Métricas en consola (para pruebas e Integrante 2)
add_executable(metricas_test
    src/gestor_datos/main_metrics.cpp
    src/gestor_datos/dataset_manager.cpp
)

# Executable 2: Buscador Espacial R-Tree Interactivo (GUI con SFML + ImGui)
add_executable(rtree_gui
    src/gui/main_gui.cpp
    src/gui/GuiApp.cpp
    src/gestor_datos/dataset_manager.cpp
)

# Enlazar librerías para la interfaz gráfica
target_link_libraries(rtree_gui PRIVATE 
    sfml-graphics 
    sfml-window 
    sfml-system 
    ImGui-SFML::ImGui-SFML
)
