#pragma once
#include <algorithm>
#include <limits>

struct MBR {
    double x_min;
    double y_min;
    double x_max;
    double y_max;

    //Constructor por defecto: inicializa un MBR vacío/invalido (infinitos invertidos)
    MBR() {
        x_min = std::numeric_limits<double>::infinity();
        y_min = std::numeric_limits<double>::infinity();
        x_max = -std::numeric_limits<double>::infinity();
        y_max = -std::numeric_limits<double>::infinity();
    }

    MBR(double xmin, double ymin, double xmax, double ymax)
        : x_min(xmin), y_min(ymin), x_max(xmax), y_max(ymax) {}

    double area() const {
        if (x_min >= x_max || y_min >= y_max) return 0.0;
        return (x_max - x_min) * (y_max - y_min);
    }

    bool intersects(const MBR& other) const {
        return (x_min <= other.x_max && x_max >= other.x_min &&
                y_min <= other.y_max && y_max >= other.y_min);
    }

    //Retornar el MBR resultante de la intersección con otro MBR
    MBR getIntersection(const MBR& other) const {
        if (!intersects(other)) {
            return MBR(); 
        }
        return MBR(
            std::max(x_min, other.x_min),
            std::max(y_min, other.y_min),
            std::min(x_max, other.x_max),
            std::min(y_max, other.y_max)
        );
    }

    //Expandir el MBR para incluir un punto
    void expand(double x, double y) {
        x_min = std::min(x_min, x);
        y_min = std::min(y_min, y);
        x_max = std::max(x_max, x);
        y_max = std::max(y_max, y);
    }

    //Expandir el MBR para incluir otro MBR
    void expand(const MBR& other) {
        x_min = std::min(x_min, other.x_min);
        y_min = std::min(y_min, other.y_min);
        x_max = std::max(x_max, other.x_max);
        y_max = std::max(y_max, other.y_max);
    }
};
