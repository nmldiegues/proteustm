#ifndef MESH_H
#define MESH_H 1


#include "element-struct.h"
#include "queue-struct.h"
#include "rbtree-struct.h"
#include "random.h"
#include "tm.h"
#include "vector.h"


typedef struct mesh  mesh_t;

struct mesh {
    element_t* rootElementPtr;
    queue_t* initBadQueuePtr;
    long size;
    SET_T* boundarySetPtr;
};



#endif /* MESH_H */
