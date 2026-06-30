#pragma once
#include "mbr.h"
#include "spatial_object.h"
#include <vector>

class RTreeNode {
public:
    bool isLeaf;
    MBR mbr;
    std::vector<SpatialObject> objects; //Datos si es hoja
    std::vector<RTreeNode*> children;   //Hijos si es nodo interno 

    static const size_t MAX_ENTRIES = 32;
    static const size_t MIN_ENTRIES = 16;

    RTreeNode(bool leaf) : isLeaf(leaf) {}

    //Destructor recursivo para liberar memoria de los hijos
    ~RTreeNode() {
        if (!isLeaf) {
            for (RTreeNode* child : children) {
                delete child;
            }
        }
    }

    void recalculateMBR() {
        mbr = MBR(); 
        if (isLeaf) {
            for (const auto& obj : objects) {
                mbr.expand(obj.x, obj.y);
            }
        } else {
            for (const auto* child : children) {
                if (child) {
                    mbr.expand(child->mbr);
                }
            }
        }
    }
};
