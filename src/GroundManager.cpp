#include "GroundManager.h"

#include <cmath>

GroundManager::GroundManager(b2WorldId world)
    : world(world)
{
}

GroundManager::~GroundManager()
{
    //remove owned floor bodies before main destroys the physics world
    for (const auto& chunk : chunks) {
        b2DestroyBody(chunk.second);
    }
}

void GroundManager::update(float ballPhysicsX)
{
    const int currentChunk =
        static_cast<int>(std::floor(ballPhysicsX / chunkWidth));

    const int firstChunk = currentChunk - 2;
    const int lastChunk = currentChunk + 2;

    //create any missing sections
    for (int index = firstChunk; index <= lastChunk; ++index) {
        if (chunks.contains(index)) {
            continue;
        }

        b2BodyDef groundDef = b2DefaultBodyDef();

        //body position is the centre of this section
        groundDef.position = {
            (index + 0.5f) * chunkWidth,
            -0.5f
        };

        b2BodyId body = b2CreateBody(world, &groundDef);

        //top of each section stays at physics Y = 0
        b2Polygon groundBox = b2MakeBox(
            chunkWidth * 0.5f,
            0.5f
        );

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.material.friction = 0.6f;

        b2CreatePolygonShape(body, &shapeDef, &groundBox);

        chunks.emplace(index, body);
    }

    //remove sections that are no longer nearby
    for (auto it = chunks.begin(); it != chunks.end();) {
        if (it->first < firstChunk || it->first > lastChunk) {
            b2DestroyBody(it->second);
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}
