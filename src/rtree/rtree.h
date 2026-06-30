#pragma once
#include "node.h"
#include <vector>
#include <limits>


class RTree {
private:
    RTreeNode* root;

    // --- Helpers para la Búsqueda KNN ---

    // 1. Calcular distancia mínima de un punto a un MBR (Rectángulo)
    double distanceToMBR(double x, double y, const MBR& mbr) const {
        double closestX = std::max(mbr.x_min, std::min(x, mbr.x_max));
        double closestY = std::max(mbr.y_min, std::min(y, mbr.y_max));
        
        double dx = x - closestX;
        double dy = y - closestY;
        return std::sqrt(dx * dx + dy * dy);
    }

    // 2. Calcular distancia exacta de un punto a un Objeto Espacial
    double distanceToObject(double x, double y, const SpatialObject& obj) const {
        double dx = x - obj.x;
        double dy = y - obj.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    // 3. Estructura envolvente para la cola de prioridad
    struct KNNItem {
        double distance;
        bool isNode;           // true si es un RTreeNode, false si es un SpatialObject
        RTreeNode* node;       // Puntero al nodo (si isNode == true)
        SpatialObject obj;     // Datos del objeto (si isNode == false)

        // Sobrecarga para Min-Heap (el menor valor de distancia tiene mayor prioridad)
        bool operator>(const KNNItem& other) const {
            return distance > other.distance;
        }
    };



public:
    RTree() : root(nullptr) {}

    //Libera la memoria recursivamente desde la raíz
    ~RTree() {
        delete root;
    }

private:
    struct SplitEntry {
        MBR mbr;
        SpatialObject obj;
        RTreeNode* child;
    };

    RTreeNode* quadraticSplit(RTreeNode* node) {
        //1. Recolectar todas las entradas
        std::vector<SplitEntry> entries;
        if (node->isLeaf) {
            for (const auto& obj : node->objects) {
                entries.push_back({MBR(obj.x, obj.y, obj.x, obj.y), obj, nullptr});
            }
        } else {
            for (auto* child : node->children) {
                entries.push_back({child->mbr, SpatialObject(), child});
            }
        }

        //2. Elegir semillas
        size_t seed1_idx = 0, seed2_idx = 1;
        double maxWaste = -std::numeric_limits<double>::infinity();

        for (size_t i = 0; i < entries.size(); ++i) {
            for (size_t j = i + 1; j < entries.size(); ++j) {
                MBR combined = entries[i].mbr;
                combined.expand(entries[j].mbr);
                double waste = combined.area() - entries[i].mbr.area() - entries[j].mbr.area();
                if (waste > maxWaste) {
                    maxWaste = waste;
                    seed1_idx = i;
                    seed2_idx = j;
                }
            }
        }

        //3. Crear grupos iniciales
        RTreeNode* newSibling = new RTreeNode(node->isLeaf);
        
        node->objects.clear();
        node->children.clear();

        //Helper para asignar entrada a un nodo y expandir su MBR
        auto assignEntry = [](RTreeNode* target, const SplitEntry& entry) {
            if (target->isLeaf) {
                target->objects.push_back(entry.obj);
            } else {
                target->children.push_back(entry.child);
            }
            target->mbr.expand(entry.mbr);
        };

        node->mbr = MBR();
        newSibling->mbr = MBR();

        assignEntry(node, entries[seed1_idx]);
        assignEntry(newSibling, entries[seed2_idx]);

        std::vector<bool> assigned(entries.size(), false);
        assigned[seed1_idx] = true;
        assigned[seed2_idx] = true;
        size_t remaining = entries.size() - 2;

        //4. Asignar el resto de entradas
        for (size_t i = 0; i < entries.size(); ++i) {
            if (assigned[i]) continue;

            size_t group1_count = node->isLeaf ? node->objects.size() : node->children.size();
            size_t group2_count = newSibling->isLeaf ? newSibling->objects.size() : newSibling->children.size();

            //Verificar el cumplimiento del mínimo
            if (group1_count + remaining == RTreeNode::MIN_ENTRIES) {
                assignEntry(node, entries[i]);
                assigned[i] = true;
                remaining--;
                continue;
            }
            if (group2_count + remaining == RTreeNode::MIN_ENTRIES) {
                assignEntry(newSibling, entries[i]);
                assigned[i] = true;
                remaining--;
                continue;
            }

            //Evaluar qué grupo produce menor incremento de área
            MBR tempMbr1 = node->mbr;
            tempMbr1.expand(entries[i].mbr);
            double increase1 = tempMbr1.area() - node->mbr.area();

            MBR tempMbr2 = newSibling->mbr;
            tempMbr2.expand(entries[i].mbr);
            double increase2 = tempMbr2.area() - newSibling->mbr.area();

            if (increase1 < increase2) {
                assignEntry(node, entries[i]);
            } else if (increase2 < increase1) {
                assignEntry(newSibling, entries[i]);
            } else {
                //Desempate 1: grupo con menor área actual
                double area1 = node->mbr.area();
                double area2 = newSibling->mbr.area();
                if (area1 < area2) {
                    assignEntry(node, entries[i]);
                } else if (area2 < area1) {
                    assignEntry(newSibling, entries[i]);
                } else {
                    //Desempate 2: grupo con menos elementos
                    if (group1_count < group2_count) {
                        assignEntry(node, entries[i]);
                    } else {
                        assignEntry(newSibling, entries[i]);
                    }
                }
            }
            assigned[i] = true;
            remaining--;
        }

        //5. Recalcular MBRs de los dos nodos
        node->recalculateMBR();
        newSibling->recalculateMBR();

        return newSibling;
    }

    //Elegir el mejor hijo para un objeto espacial
    RTreeNode* chooseLeafStep(RTreeNode* node, const SpatialObject& obj) {
        RTreeNode* bestChild = nullptr;
        double minIncrease = std::numeric_limits<double>::infinity();
        double minArea = std::numeric_limits<double>::infinity();

        for (RTreeNode* child : node->children) {
            if (!child) continue;

            double currentArea = child->mbr.area();
            
            MBR tempMbr = child->mbr;
            tempMbr.expand(obj.x, obj.y);
            double expandedArea = tempMbr.area();
            double increase = expandedArea - currentArea;

            if (increase < minIncrease) {
                minIncrease = increase;
                minArea = currentArea;
                bestChild = child;
            } else if (increase == minIncrease) {
                if (currentArea < minArea) {
                    minArea = currentArea;
                    bestChild = child;
                }
            }
        }
        return bestChild;
    }

    //Auxiliar recursivo para la inserción y propagación de MBRs
    RTreeNode* insertHelper(RTreeNode* node, const SpatialObject& obj) {
        if (node->isLeaf) {
            node->objects.push_back(obj);
            node->mbr.expand(obj.x, obj.y);

            //Control de desbordamiento: Split cuadrático
            if (node->objects.size() > RTreeNode::MAX_ENTRIES) {
                return quadraticSplit(node);
            }
            return nullptr;
        }

        //Si es nodo interno, elegir el mejor hijo
        RTreeNode* bestChild = chooseLeafStep(node, obj);

        //Descenso recursivo
        RTreeNode* newSibling = insertHelper(bestChild, obj);

        if (newSibling != nullptr) {
            node->children.push_back(newSibling);
            node->recalculateMBR(); 

            if (node->children.size() > RTreeNode::MAX_ENTRIES) {
                return quadraticSplit(node);
            }
        } else {
            node->mbr.expand(obj.x, obj.y);
        }

        return nullptr;
    }

    //Helper recursivo para la consulta por rango rectangular
    void rangeQueryHelper(RTreeNode* node, const MBR& queryBox, std::vector<SpatialObject>& results) const {
        if (node == nullptr) return;

        //Si el MBR del nodo actual no intersecta con el queryBox, retornar
        if (!node->mbr.intersects(queryBox)) {
            return;
        }

        if (node->isLeaf) {
            for (const auto& obj : node->objects) {
                if (obj.x >= queryBox.x_min && obj.x <= queryBox.x_max &&
                    obj.y >= queryBox.y_min && obj.y <= queryBox.y_max) {
                    results.push_back(obj);
                }
            }
        } else {
            for (RTreeNode* child : node->children) {
                rangeQueryHelper(child, queryBox, results);
            }
        }
    }

public:
    void insert(const SpatialObject& obj) {
        if (root == nullptr) {
            root = new RTreeNode(true); 
        }
        RTreeNode* newSibling = insertHelper(root, obj);
        if (newSibling != nullptr) {
            //La raíz se dividió, creamos una nueva raíz
            RTreeNode* newRoot = new RTreeNode(false); //raíz interna
            newRoot->children.push_back(root);
            newRoot->children.push_back(newSibling);
            newRoot->recalculateMBR();
            root = newRoot;
        }
    }

    std::vector<SpatialObject> rangeQuery(const MBR& queryBox) const {
        std::vector<SpatialObject> results;
        rangeQueryHelper(root, queryBox, results);
        return results;
    }

    RTreeNode* getRoot() const {
        return root;
    }

    //Consultar los k vecinos más cercanos (k-NN Query)
    std::vector<SpatialObject> knnQuery(double x, double y, int k) const {
        std::vector<SpatialObject> results;
        if (root == nullptr || k <= 0) return results;

        // Cola de prioridad Min-Heap (extrae el elemento con la distancia mínima)
        std::priority_queue<KNNItem, std::vector<KNNItem>, std::greater<KNNItem>> pq;

        // Insertar la raíz del árbol en la cola
        pq.push({distanceToMBR(x, y, root->mbr), true, root, SpatialObject()});

        while (!pq.empty() && results.size() < static_cast<size_t>(k)) {
            KNNItem current = pq.top();
            pq.pop();

            if (current.isNode) {
                RTreeNode* node = current.node;
                
                if (node->isLeaf) {
                    // Si llegamos a una hoja, calculamos distancias reales a los objetos
                    for (const auto& obj : node->objects) {
                        double dist = distanceToObject(x, y, obj);
                        pq.push({dist, false, nullptr, obj});
                    }
                } else {
                    // Si es un nodo interno, metemos los MBR de sus hijos a la cola
                    for (RTreeNode* child : node->children) {
                        if (child != nullptr) {
                            double dist = distanceToMBR(x, y, child->mbr);
                            pq.push({dist, true, child, SpatialObject()});
                        }
                    }
                }
            } else {
                // Si el elemento extraído NO es un nodo, es el objeto espacial.
                results.push_back(current.obj);
            }
        }

        return results;
    }

    //Eliminar un objeto por su identificador
    bool remove(int id) {
        //PENDIENTE: Implementar eliminación y reajuste/condensación del árbol
        return false;
    }
};
