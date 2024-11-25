#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 740
#define SCREEN_HEIGHT 580
#define TILE_SIZE 20

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position segments[SCREEN_WIDTH * SCREEN_HEIGHT / (TILE_SIZE * TILE_SIZE)];
    int size;
    Position direction;
} Snake;

SDL_Texture* load_texture(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        fprintf(stderr, "Failed to load image %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void draw_text(SDL_Renderer* renderer, TTF_Font* font, const char* message, SDL_Color color, int x, int y) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, message, color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}
bool game_loop(SDL_Renderer* renderer, TTF_Font* font, Mix_Chunk* eat_sound, Mix_Chunk* collision_sound, SDL_Texture* bg_texture, SDL_Texture* food_texture, SDL_Texture* snake_texture, SDL_Texture* game_over_texture) {
    srand((unsigned int)time(NULL));

    Snake snake = {{{0}}, 4, {1, 0}};
    for (int i = 0; i < snake.size; i++) {
        snake.segments[i].x = snake.size - i - 1;
        snake.segments[i].y = 0;
    }

    Position food = {rand() % (SCREEN_WIDTH / TILE_SIZE), rand() % (SCREEN_HEIGHT / TILE_SIZE)};
    int score = 0;
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: if (snake.direction.y == 0) snake.direction = (Position){0, -1}; break;
                    case SDLK_DOWN: if (snake.direction.y == 0) snake.direction = (Position){0, 1}; break;
                    case SDLK_LEFT: if (snake.direction.x == 0) snake.direction = (Position){-1, 0}; break;
                    case SDLK_RIGHT: if (snake.direction.x == 0) snake.direction = (Position){1, 0}; break;
                }
            }
        }

        for (int i = snake.size - 1; i > 0; i--) {
            snake.segments[i] = snake.segments[i - 1];
        }
        snake.segments[0].x += snake.direction.x;
        snake.segments[0].y += snake.direction.y;

        if (snake.segments[0].x < 0 || snake.segments[0].x >= SCREEN_WIDTH / TILE_SIZE ||
            snake.segments[0].y < 0 || snake.segments[0].y >= SCREEN_HEIGHT / TILE_SIZE) {
            Mix_PlayChannel(-1, collision_sound, 0);
            SDL_Delay(2000);
            break;
        }

        for (int i = 1; i < snake.size; i++) {
            if (snake.segments[0].x == snake.segments[i].x && snake.segments[0].y == snake.segments[i].y) {
                Mix_PlayChannel(-1, collision_sound, 0);
                SDL_Delay(2000);
                running = false;
                break;
            }
        }

        if (snake.segments[0].x == food.x && snake.segments[0].y == food.y) {
            snake.size++;
            score++;
            food = (Position){rand() % (SCREEN_WIDTH / TILE_SIZE), rand() % (SCREEN_HEIGHT / TILE_SIZE)};
            Mix_PlayChannel(-1, eat_sound, 0);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bg_texture, NULL, NULL);

        SDL_Rect food_rect = {food.x * TILE_SIZE, food.y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
        SDL_RenderCopy(renderer, food_texture, NULL, &food_rect);

        for (int i = 0; i < snake.size; i++) {
            SDL_Rect segment_rect = {snake.segments[i].x * TILE_SIZE, snake.segments[i].y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, snake_texture, NULL, &segment_rect);
        }
        

        char score_text[32];
        snprintf(score_text, sizeof(score_text), "Score: %d", score);
        draw_text(renderer, font, score_text, (SDL_Color){255, 255, 255, 255}, 10, 10);

        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, bg_texture, NULL, NULL);

    SDL_Rect game_over_rect = {SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 100, 300, 100};
    SDL_RenderCopy(renderer, game_over_texture, NULL, &game_over_rect);

    char final_score[64];
    snprintf(final_score, sizeof(final_score), "Final Score: %d", score);
    draw_text(renderer, font, final_score, (SDL_Color){255, 255, 255, 255}, SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 20);

    draw_text(renderer, font, "Press R to Retry or Q to Quit", (SDL_Color){255, 255, 255, 255}, SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 60);
    SDL_RenderPresent(renderer);

    while (SDL_WaitEvent(&event)) {
        if (event.type == SDL_QUIT) return false;
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_r) return true;
            if (event.key.keysym.sym == SDLK_q) return false;
        }
    }

    return false;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || TTF_Init() == -1 || Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "Failed to initialize SDL components.\n");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) {
        fprintf(stderr, "Failed to create window or renderer.\n");
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("Super Vibes.ttf", 20);
    Mix_Chunk* eat_sound = Mix_LoadWAV("eat.wav");
    Mix_Chunk* collision_sound = Mix_LoadWAV("crash sound.wav");
    SDL_Texture* bg_texture = load_texture(renderer, "background.png");
    SDL_Texture* food_texture = load_texture(renderer, "fruit.png");
    SDL_Texture* snake_texture = load_texture(renderer, "snake skin.png");
    SDL_Texture* game_over_texture = load_texture(renderer, "game over.jpg");

    if (!font || !eat_sound || !collision_sound || !bg_texture || !food_texture || !snake_texture || !game_over_texture) {
        fprintf(stderr, "Failed to load assets.\n");
        return 1;
    }

    bool retry;
    do {
        retry = game_loop(renderer, font, eat_sound, collision_sound, bg_texture, food_texture, snake_texture, game_over_texture);
    } while (retry);

    SDL_DestroyTexture(bg_texture);
    SDL_DestroyTexture(food_texture);
    SDL_DestroyTexture(snake_texture);
    SDL_DestroyTexture(game_over_texture);
    Mix_FreeChunk(eat_sound);
    Mix_FreeChunk(collision_sound);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
