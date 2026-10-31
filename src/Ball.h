#pragma once

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <cstddef>

class Camera;

//own the balls physics body, position, sprite and launch state
class Ball {
public:
    enum class State { Ready, Dragging, Flying };

    //the world and texture must stay alive until the ball is destroyed
    Ball(b2WorldId world, SDL_Texture* texture, SDL_FPoint startPosition,
         float floorY, float pixelsPerMetre, float height, std::size_t spriteIndex);
    ~Ball();

    Ball(const Ball&) = delete;
    Ball& operator=(const Ball&) = delete;

    bool beginDrag(SDL_FPoint mousePosition);
    void dragTo(SDL_FPoint worldPosition);
    void cancelDrag();
    void launch(SDL_FPoint velocityPixelsPerSecond);
    void bounce(SDL_FPoint landingPosition, float upwardSpeed);
    void setGravityScale(float scale);
    void boost(float upwardVelocity);
    void reset();
    void updateFromPhysics();
    void draw(SDL_Renderer* renderer, const Camera& camera) const;
    void setSpriteIndex(std::size_t index);
    void setSkin(SDL_Texture* sheet, SDL_FRect source);

    State getState() const;
    SDL_FPoint getPosition() const;
    b2Vec2 getPhysicsPosition() const;
    b2Vec2 getPhysicsVelocity() const;
    float getRadius() const;

private:
    b2Vec2 toPhysics(SDL_FPoint worldPosition) const;

    b2BodyId body = b2_nullBodyId;
    SDL_Texture* texture;
    SDL_FPoint startPosition;
    SDL_FPoint position;
    float floorY;
    float pixelsPerMetre;
    float height;
    std::size_t spriteIndex = 0;
    SDL_FRect spriteSource{};
    State state = State::Ready;
};
