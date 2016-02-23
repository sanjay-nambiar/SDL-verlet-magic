
#include "simulation.hpp"

#include <iostream>

#include "SDL2/SDL.h"
#include "SDL2/SDL2_gfxPrimitives.h"

#include "vector2d.hpp"
#include "particle.hpp"
#include "constraints.hpp"
#include "composite.hpp"
#include "objects.hpp"
#include "verlet.hpp"


#define VERLET_PARTICLE_COLOR 0xFF00FF00
#define VERLET_PIN_COLOR 0xFF0000FF
#define VERLET_LINE_COLOR 0xFFFFFFFF
#define WORLD_WIDTH 800
#define WORLD_HEIGHT 600
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600


using namespace physics;

// Private methods

int Simulation::InitializeSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    window = SDL_CreateWindow("Verlet Sim", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        SDL_DestroyWindow(window);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    if (SDL_GetRendererOutputSize(renderer, &renderer_width, &renderer_height) != 0)
    {
        std::cout << "SDL_GetRendererOutputSize Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    return 0;
}

void Simulation::DestroySDL()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Simulation::CreateWorld(int width, int height)
{
    world_width = width;
    world_height = height;
    world_aspect_ratio = width / height;
    world = new Verlet<float>(width, height);

    // Line segments
    std::vector<math::Vector2d<float> > segment_points = {
        math::Vector2d<float>(0,0), math::Vector2d<float>(20,0),
        math::Vector2d<float>(40,0), math::Vector2d<float>(60,0),
        math::Vector2d<float>(80,0), math::Vector2d<float>(100,0),
        math::Vector2d<float>(120,0), math::Vector2d<float>(140,0),
        math::Vector2d<float>(160,0), math::Vector2d<float>(180,0),
        math::Vector2d<float>(200,0), math::Vector2d<float>(220,0),
        math::Vector2d<float>(240,0), math::Vector2d<float>(260,0),
        math::Vector2d<float>(280,0), math::Vector2d<float>(300,0),
        math::Vector2d<float>(320,0), math::Vector2d<float>(340,0),
        math::Vector2d<float>(360,0), math::Vector2d<float>(380,0),
        math::Vector2d<float>(400,0), math::Vector2d<float>(420,0),
        math::Vector2d<float>(440,0), math::Vector2d<float>(460,0),
        math::Vector2d<float>(480,0), math::Vector2d<float>(500,0)
    };
    math::Vector2d<float> position_offset(140, 30);
    float stiffness = 0.2;
    
    Composite<float>* segment = LineSegments<float>(segment_points, position_offset, stiffness);
    segment->Pin(0);
    segment->Pin(segment_points.size() - 1);
    world->AddComposite(segment);
}

inline math::Vector2d<float> Simulation::ScaleFromWorldToRenderer(math::Vector2d<float> position) const
{
    return math::Vector2d<float>(position.x, position.y);
}


// Public methods

Simulation::Simulation()
{
    InitializeSDL();
    CreateWorld(WORLD_WIDTH, WORLD_HEIGHT);
}

Simulation::~Simulation()
{
    DestroySDL();
    const std::vector<Composite<float>*>* const composites = world->composites;
    for (auto compositeIt = composites->begin(); compositeIt != composites->end(); compositeIt++)
    {
        std::vector<Constraint<float>*>* constraints = &(*compositeIt)->constraints;
        for (auto constraintIt = constraints->begin(); constraintIt != constraints->end(); constraintIt++)
        {
            delete (*constraintIt);
        }

        std::vector<Particle<float>*>* particles = &(*compositeIt)->particles;
        for (auto particleIt = particles->begin(); particleIt != particles->end(); particleIt++)
        {
            delete (*particleIt);
        }

        delete (*compositeIt);
    }
    delete this->world;
}

bool Simulation::HandleInput()
{
    SDL_Event event;

    if (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            return false;
        }
  
        if (event.type == SDL_KEYDOWN)
        {
            SDL_Keycode keyPressed = event.key.keysym.sym;
            switch (keyPressed)
            {
                case SDLK_ESCAPE:
                    return false;
            }
        }
    }
    return true;
}

void Simulation::Update()
{
    world->Update(16);
}

void Simulation::Draw()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const std::vector<Composite<float>*>* const composites = world->composites;
    for (auto compositeIt = composites->begin(); compositeIt != composites->end(); compositeIt++)
    {
        std::vector<Constraint<float>*>* constraints = &(*compositeIt)->constraints;
        for (auto constraintIt = constraints->begin();
             constraintIt != constraints->end(); constraintIt++)
        {
            //PinConstraint
            if ((*constraintIt)->type == PIN)
            {
                PinConstraint<float>* pin_constraint = static_cast<PinConstraint<float>*>(*constraintIt);
                math::Vector2d<float>* position = &pin_constraint->particle->position;
                math::Vector2d<float> scaledPosition = ScaleFromWorldToRenderer(*position);
                filledCircleColor(renderer, scaledPosition.x, scaledPosition.y, 5, VERLET_PIN_COLOR);
            }
            //DistanceConstraint
            else if ((*constraintIt)->type == DISTANCE)
            {
                DistanceConstraint<float>* distance_constraint = static_cast<DistanceConstraint<float>*>(*constraintIt);
                math::Vector2d<float>* position1 = &distance_constraint->p1->position;
                math::Vector2d<float>* position2 = &distance_constraint->p2->position;
                math::Vector2d<float> scaledPosition1 = ScaleFromWorldToRenderer(*position1);
                math::Vector2d<float> scaledPosition2 = ScaleFromWorldToRenderer(*position2);

                lineColor(renderer, scaledPosition1.x, scaledPosition1.y, 
                    scaledPosition2.x, scaledPosition2.y, VERLET_LINE_COLOR);
                filledCircleColor(renderer, scaledPosition1.x, scaledPosition1.y, 3, VERLET_PARTICLE_COLOR);
                filledCircleColor(renderer, scaledPosition2.x, scaledPosition2.y, 3, VERLET_PARTICLE_COLOR);
            }
        }
    }
    SDL_RenderPresent(renderer);
}
