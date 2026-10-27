#include "Ball.h"
#include "BallSprites.h"
#include "Camera.h"

#include <algorithm>
#include <numbers>
#include <stdexcept>

Ball::Ball(b2WorldId world, SDL_Texture* texture, SDL_FPoint startPosition,
           float floorY, float pixelsPerMetre, float height, std::size_t spriteIndex)
    : texture(texture), startPosition(startPosition), position(startPosition),
      floorY(floorY), pixelsPerMetre(pixelsPerMetre), height(height)
{
    if (pixelsPerMetre <= 0.0f || height <= 0.0f) {
        throw std::invalid_argument("Ball size and physics scale must be positive");
    }
    setSpriteIndex(spriteIndex);

    //create the ball with physics disabled until launched
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = toPhysics(position);
    bodyDef.isEnabled = false;
    bodyDef.fixedRotation = false;
    body = b2CreateBody(world, &bodyDef);

    b2Circle circle = {};
    circle.radius = getRadius() / pixelsPerMetre;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.4f;
    shapeDef.material.restitution = 0.35f;
    shapeDef.material.rollingResistance = 0.15f;
    b2CreateCircleShape(body, &shapeDef, &circle);
}

Ball::~Ball()
{
    //the game destroys the ball before destroying its physics world
    b2DestroyBody(body);
}

b2Vec2 Ball::toPhysics(SDL_FPoint worldPosition) const
{
    return {worldPosition.x / pixelsPerMetre, (floorY - worldPosition.y) / pixelsPerMetre};
}

bool Ball::beginDrag(SDL_FPoint mousePosition)
{
    if (state != State::Ready) {
        return false;
    }
    const float dx = mousePosition.x - position.x;
    const float dy = mousePosition.y - position.y;
    if (dx * dx + dy * dy > getRadius() * getRadius()) {
        return false;
    }
    state = State::Dragging;
    return true;
}

void Ball::dragTo(SDL_FPoint worldPosition)
{
    if (state == State::Dragging) {
        position = worldPosition;
    }
}

void Ball::cancelDrag()
{
    if (state == State::Dragging) {
        position = startPosition;
        state = State::Ready;
    }
}

void Ball::launch(SDL_FPoint velocityPixelsPerSecond)
{
    if (state != State::Dragging) {
        return;
    }

    //convert screen-style velocity into metres with positive Y pointing upwards
    const b2Vec2 velocity = {
        velocityPixelsPerSecond.x / pixelsPerMetre,
        -velocityPixelsPerSecond.y / pixelsPerMetre
    };
    b2Body_SetTransform(body, toPhysics(position), b2Rot_identity);
    b2Body_Enable(body);
    b2Body_SetLinearVelocity(body, velocity);

    //limit the starting spin so powerful launches remain readable
    const float physicsRadius = getRadius() / pixelsPerMetre;
    b2Body_SetAngularVelocity(body, std::clamp(-velocity.x / physicsRadius, -18.0f, 18.0f));
    b2Body_SetAwake(body, true);
    state = State::Flying;
}

void Ball::bounce(SDL_FPoint landingPosition, float upwardSpeed)
{
    if (state != State::Flying) {
        return;
    }
    //every trampoline sets the same upward speed and keeps horizontal motion and spin
    position = landingPosition;
    b2Body_SetTransform(body, toPhysics(position), b2Body_GetRotation(body));
    const b2Vec2 velocity = b2Body_GetLinearVelocity(body);
    b2Body_SetLinearVelocity(body, {velocity.x, upwardSpeed / pixelsPerMetre});
    b2Body_SetAwake(body, true);
}

void Ball::reset()
{
    //stop movement and spin before returning to the launcher
    b2Body_SetLinearVelocity(body, {0.0f, 0.0f});
    b2Body_SetAngularVelocity(body, 0.0f);
    b2Body_Disable(body);
    b2Body_SetTransform(body, toPhysics(startPosition), b2Rot_identity);
    position = startPosition;
    state = State::Ready;
}

void Ball::updateFromPhysics()
{
    if (state == State::Flying) {
        const b2Vec2 physicsPosition = b2Body_GetPosition(body);
        position = {physicsPosition.x * pixelsPerMetre,
                    floorY - physicsPosition.y * pixelsPerMetre};
    }
}

void Ball::draw(SDL_Renderer* renderer, const Camera& camera) const
{
    const SDL_FRect& source = ballSprites.at(spriteIndex);
    const float width = height * source.w / source.h;
    const SDL_FPoint screen = camera.worldToScreen(position.x, position.y);
    SDL_FRect destination = {screen.x - width * 0.5f, screen.y - height * 0.5f, width, height};

    //match the sprite rotation to the physics body
    const double angle = -static_cast<double>(b2Rot_GetAngle(b2Body_GetRotation(body))) *
                         180.0 / std::numbers::pi;
    SDL_RenderTextureRotated(renderer, texture, &source, &destination, angle, nullptr, SDL_FLIP_NONE);
}

void Ball::setSpriteIndex(std::size_t index)
{
    if (index >= ballSprites.size()) {
        throw std::out_of_range("Ball sprite index must be between 0 and 39");
    }
    spriteIndex = index;
}

Ball::State Ball::getState() const { return state; }
SDL_FPoint Ball::getPosition() const { return position; }
b2Vec2 Ball::getPhysicsPosition() const { return b2Body_GetPosition(body); }
float Ball::getRadius() const { return height * 0.5f; }
