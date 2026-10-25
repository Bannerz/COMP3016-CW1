#pragma once

#include <box2d/box2d.h>
#include <map>

//create and remove the floor sections around the ball
class GroundManager {
public:
    //main owns the world, which must outlive this manager
    explicit GroundManager(b2WorldId world);
    ~GroundManager();

    //only one manager should own these floor bodies
    GroundManager(const GroundManager&) = delete;
    GroundManager& operator=(const GroundManager&) = delete;

    void update(float ballPhysicsX);

private:
    b2WorldId world;
    static constexpr float chunkWidth = 50.0f;
    std::map<int, b2BodyId> chunks;
};
