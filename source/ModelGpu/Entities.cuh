#pragma once

#include "Base.cuh"
#include "Definitions.cuh"
#include "MultiArrayController.cuh"

struct Entities
{
    MultiArrayController<Cluster*, NUM_CLUSTERPOINTERARRAYS> clusterPointerArrays;
    ArrayController<Cell*> cellPointers;
    ArrayController<Token*> tokenPointers;

    ArrayController<Cluster> clusters;
    ArrayController<Cell> cells;
    ArrayController<Token> tokens;
    ArrayController<Particle> particles;

    void init()
    {
        int sizes[NUM_CLUSTERPOINTERARRAYS];
        for (int i = 0; i < NUM_CLUSTERPOINTERARRAYS; ++i) {
            sizes[i] = MAX_CLUSTERPOINTERS / NUM_CLUSTERPOINTERARRAYS;
        }
        clusterPointerArrays.init(sizes);

        clusters.init(MAX_CLUSTERS);
        cellPointers.init(MAX_CELLPOINTERS);
        cells.init(MAX_CELLS);
        tokenPointers.init(MAX_TOKENPOINTERS);
        tokens.init(MAX_TOKENS);
        particles.init(MAX_PARTICLES);
    }

    void free()
    {
        clusterPointerArrays.free();
        clusters.free();
        cellPointers.free();
        cells.free();
        tokenPointers.free();
        tokens.free();
        particles.free();
    }
};
