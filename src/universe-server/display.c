#include "display.h"

#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

void _DrawPlanets(SDL_Renderer* renderer, GameState* game_state) {

    //load font for planet names
    TTF_Font* font = TTF_OpenFont("arial.ttf", 12);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        exit(1);
    }

    //planets will be drawn as just circles with their names on the top right
    for (int i = 0; i < game_state->n_planets; i++) {
        //draw planet i as a filled circle (blue)
        filledCircleRGBA(renderer, (int)game_state->planets[i].position.x, (int)game_state->planets[i].position.y, (int)game_state->planets[i].radius, 0, 0, 255, 255);

        //draw planet name at top-right of planet
        //name is a single char + the amount of trash inside the planet
        char name_text[10];
        snprintf(name_text, sizeof(name_text), "%c%d", game_state->planets[i].name, game_state->planets[i].trash_amount);
        SDL_Color textColor = {0, 0, 0, 255}; //black color
        
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, name_text, textColor);
        if (textSurface == NULL) {
            printf("Failed to create text surface: %s\n", TTF_GetError());
            exit(1);
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture == NULL) {
            printf("Failed to create text texture: %s\n", SDL_GetError());
            SDL_FreeSurface(textSurface);
            exit(1);
        }

        SDL_Rect textRect;
        textRect.x = (int)(game_state->planets[i].position.x + game_state->planets[i].radius);
        textRect.y = (int)(game_state->planets[i].position.y - game_state->planets[i].radius);
        textRect.w = textSurface->w * 2;  
        textRect.h = textSurface->h * 2;
        SDL_FreeSurface(textSurface);
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);
    }

    TTF_CloseFont(font);
}

void _DrawTrash(SDL_Renderer* renderer, GameState* game_state) {
    
    //trash will be drawn as small red circles
    for (int i = 0; i < game_state->n_trashes; i++) {
        filledCircleRGBA(renderer, (int)game_state->trashes[i].position.x, (int)game_state->trashes[i].position.y, (int)game_state->trashes[i].radius, 255, 0, 0, 255);
    }
}

void _DrawGameOver(SDL_Renderer* renderer, GameState* game_state) {

    if(game_state->is_game_over == 0) {
        return;
    }

    //load font for game over text
    TTF_Font* font = TTF_OpenFont("arial.ttf", 48);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        exit(1);
    }

    //create "Game Over" text surface
    SDL_Color textColor = {255, 0, 0, 255}; //red color
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, "GAME OVER", textColor);
    if (textSurface == NULL) {
        printf("Failed to create text surface: %s\n", TTF_GetError());
        exit(1);
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == NULL) {
        printf("Failed to create text texture: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        exit(1);
    }

    SDL_Rect textRect;
    textRect.x = (game_state->universe_size - textSurface->w) / 2;
    textRect.y = (game_state->universe_size - textSurface->h) / 2;
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;
    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);

    TTF_CloseFont(font);
}

void Draw(SDL_Renderer* renderer, GameState* game_state) {

    //set background color to whiteish gray
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderClear(renderer);

    //for each of the GameStates object vectors, we make a draw loop.
    _DrawPlanets(renderer, game_state);
    _DrawTrash(renderer, game_state);
    _DrawGameOver(renderer, game_state);

    //present the rendered frame
    SDL_RenderPresent(renderer);
}
