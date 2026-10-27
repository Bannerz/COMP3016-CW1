#include "GroundManager.h"

#include <cmath>
#include <stdexcept>

GroundManager::GroundManager(b2WorldId world, float friction, float rollingResistance)
    : world(world), friction(friction), rollingResistance(rollingResistance)
{
    if (!std::isfinite(friction) || !std::isfinite(rollingResistance) ||
        friction < 0.0f || rollingResistance < 0.0f) {
        throw std::invalid_argument("Ground resistance must be finite and nonnegative");
    }
}

GroundManager::~GroundManager()
{
    //remove owned floor bodies before Game destroys the physics world
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

        //body position is the centre of this section at floor height
        groundDef.position = {
            (index + 0.5f) * chunkWidth,
            0.0f
        };

        b2BodyId body = b2CreateBody(world, &groundDef);

        //the middle edge is the floor, with a ghost point beyond each end
        //neighbouring chains share three points so the ball cannot catch a corner
        const b2Vec2 points[] = {
            {chunkWidth * 1.5f, 0.0f},
            {chunkWidth * 0.5f, 0.0f},
            {-chunkWidth * 0.5f, 0.0f},
            {-chunkWidth * 1.5f, 0.0f}
        };

        //points run right to left so the floor collides from above
        b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
        //friction slows sliding, while rolling resistance slows the balls spin
        //both only act when the ball is touching the floor
        material.friction = friction;
        material.rollingResistance = rollingResistance;

        b2ChainDef chainDef = b2DefaultChainDef();
        chainDef.points = points;
        chainDef.count = 4;
        chainDef.materials = &material;
        chainDef.materialCount = 1;
        b2CreateChain(body, &chainDef);

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
