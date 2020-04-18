#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h> // emergency?

#include <math.h>

static constexpr unsigned int virtual_window_width = 1024;
static constexpr unsigned int virtual_window_height = 768;

static constexpr unsigned int window_width = 1024;
static constexpr unsigned int window_height = 768;

static bool game_running = true;

enum font_sizes{
    FONT_SIZE_SMALL,
    FONT_SIZE_MEDIUM,
    FONT_SIZE_BIG,

    FONT_SIZE_TYPES
};

namespace gfx{
    TTF_Font* fonts[FONT_SIZE_TYPES];

    struct color{
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    static color white = {255, 255, 255, 255};
    static color red = {255, 0, 0, 255};
    static color green = {0, 255, 0, 255};
    static color blue = {0, 0, 255, 255};
    static color yellow = {0, 255, 255, 255};

    struct rectangle{
        float x;
        float y;
        float w;
        float h;
    };

    static void render_rectangle(
            SDL_Renderer* renderer, 
            const rectangle rect,
            const color c){
        SDL_Rect as_sdl_rect = {
            rect.x,
            rect.y,
            rect.w,
            rect.h
        };
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_RenderFillRect(renderer, &as_sdl_rect);
    }

    static void render_text(
            SDL_Renderer* renderer,
            unsigned font_handle,
            float x, 
            float y,
            char* string,
            color c){
        SDL_Surface* temporary_text_surface;

        temporary_text_surface = 
            TTF_RenderText_Blended(
                    fonts[font_handle], 
                    string, 
                    SDL_Color{c.r, c.g, c.b, c.a});

        SDL_Texture* as_texture =
            SDL_CreateTextureFromSurface(renderer, 
                    temporary_text_surface);

        SDL_Rect destination_rect =
        {
            x, y,
            temporary_text_surface->w, 
            temporary_text_surface->h
        };

        SDL_RenderCopy(
                renderer, 
                as_texture,
                NULL,
                &destination_rect);

        SDL_FreeSurface(temporary_text_surface);
        SDL_DestroyTexture(as_texture);
    }

    static void render_centered_text(
            SDL_Renderer* renderer,
            unsigned font_handle,
            float w,
            float h,
            char* string,
            color c){
        SDL_Surface* temporary_text_surface;

        temporary_text_surface = 
            TTF_RenderText_Blended(
                    fonts[font_handle], 
                    string, 
                    SDL_Color{c.r, c.g, c.b, c.a});

        SDL_Texture* as_texture =
            SDL_CreateTextureFromSurface(renderer, 
                    temporary_text_surface);

        SDL_Rect destination_rect =
        {
            ( w / 2 ) - (temporary_text_surface->w / 2), 
            ( h / 2 ) - (temporary_text_surface->h / 2),
            temporary_text_surface->w, 
            temporary_text_surface->h
        };

        SDL_RenderCopy(
                renderer, 
                as_texture,
                NULL,
                &destination_rect);

        SDL_FreeSurface(temporary_text_surface);
        SDL_DestroyTexture(as_texture);
    }

    static void render_centered_dynamically_scaled_text(
            SDL_Renderer* renderer,
            unsigned font_handle,
            float scale,
            float w,
            float h,
            char* string,
            color c){
        SDL_Surface* temporary_text_surface;

        temporary_text_surface = 
            TTF_RenderText_Blended(
                    fonts[font_handle], 
                    string, 
                    SDL_Color{c.r, c.g, c.b, c.a});

        SDL_Texture* as_texture =
            SDL_CreateTextureFromSurface(renderer, 
                    temporary_text_surface);

        SDL_Rect destination_rect =
        {
            ( w / 2 ) - ((temporary_text_surface->w / 2) * scale), 
            ( h / 2 ) - ((temporary_text_surface->h / 2) * scale),
            temporary_text_surface->w * scale, 
            temporary_text_surface->h * scale
        };

        SDL_RenderCopy(
                renderer, 
                as_texture,
                NULL,
                &destination_rect);

        SDL_FreeSurface(temporary_text_surface);
        SDL_DestroyTexture(as_texture);
    }
};

namespace game{
    enum game_screen_state{
        GAME_SCREEN_STATE_MAIN_MENU,
        GAME_SCREEN_STATE_ATTRACT_MODE,

        GAME_SCREEN_STATE_GAMEPLAY,
        GAME_SCREEN_STATE_GAME_OVER,
        GAME_SCREEN_STATE_PAUSE,

        GAME_SCREEN_STATE_VICTORY,

        GAME_SCREEN_STATE_COUNT
    };

    enum enemy_type{
        ENEMY_BASIC_LUMBERJACK,
        ENEMY_SKINNY_LUMBERJACK,
        ENEMY_FAT_LUMBERJACK,
        ENEMY_ANARCHIST,
        ENEMY_LOGGING_MACHINES,
        ENEMY_ANTHROPHORMORPHIC_ANT,

        ENEMY_TYPE_COUNT
    };

    struct aabb{
        float x;
        float y;
        float w;
        float h;
    };

    const bool aabb_intersects(aabb a, aabb b){
        bool x_intersects = (a.x < b.x + b.w) && (a.x + a.w > b.x);
        bool y_intersects = (a.y < b.y + b.h) && (a.y + a.h > b.y);

        return x_intersects && y_intersects;
    }

    struct enemy_entity{
        enemy_type type;

        float x;
        float y;
        float w;
        float h;

        int max_health;
        int health;

        int max_defense;
        int defense;

        int speed;

        bool frozen;
    };

    struct tree_entity{
        float x;
        float y;
        float w;
        float h;

        int max_health;
        int max_defense;

        int health;
        int defense;
    };

    static constexpr size_t MAX_ENEMIES_IN_GAME = 512;

    struct state{
        game_screen_state screen;

#if 0
        float time_passed;
#endif

        bool wave_started;
        int game_wave;

        int enemies_in_wave;
        enemy_entity enemies[MAX_ENEMIES_IN_GAME];

        float time_until_wave_ends;
        float preparation_timer;

        tree_entity tree;
    };

    static void handle_mouse_input(state& game_state, SDL_Event* event){
        int mouse_x = 0;
        int mouse_y = 0;

        switch(event->type){
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                mouse_x = event->button.x;
                mouse_y = event->button.y;

                bool button_down = event->type == SDL_MOUSEBUTTONDOWN;
                uint8_t button = event->button.button;

                if(button == SDL_BUTTON_LEFT){
                }else if(button == SDL_BUTTON_MIDDLE){
                }else if(button == SDL_BUTTON_RIGHT){
                }
            }
            break;
            case SDL_MOUSEMOTION:
            {
                mouse_x = event->motion.x;
                mouse_y = event->motion.y;
            }
            break;
        }
    }

    static void handle_key_input(state& game_state, SDL_Event* event){
        int scancode = event->key.keysym.scancode;
        bool keydown = event->type == SDL_KEYDOWN;
        switch(scancode){
            case SDL_SCANCODE_W:
            {
                game_state.tree.health -= 10;
            }
            break;
            case SDL_SCANCODE_RETURN:
            {
                if(game_state.screen == GAME_SCREEN_STATE_MAIN_MENU){
                    game_state.screen = GAME_SCREEN_STATE_GAMEPLAY;
                }
            }
            break;
            case SDL_SCANCODE_ESCAPE:
            {
                if(game_state.screen == GAME_SCREEN_STATE_MAIN_MENU){
                    game_running = false;
                }
            }
            break;
        }
    }

    /*
     * @TODO jerry:
     *
     * Generate these things in a circle shape / path.
     */
    static void push_enemy(state& game_state, enemy_entity enemy){
        enemy_entity* current = 
            &game_state.enemies[game_state.enemies_in_wave++];

        *current = enemy;
    }

    static void generate_wave(state& game_state){
        push_enemy(game_state, 
                {
                ENEMY_BASIC_LUMBERJACK,
                0, 0, 32, 32,

                100, 100,

                30, 30,

                4,

                false
                });

        push_enemy(game_state, 
                {
                ENEMY_BASIC_LUMBERJACK,
                64, 64, 32, 32,

                100, 100,

                30, 30,

                4,

                false
                });
    }

    static void initialize(state& game_state, SDL_Renderer* renderer){
        static int font_sizes_array[FONT_SIZE_TYPES] = 
        { 16, 32, 64 };
        for(int i = 0; i < FONT_SIZE_TYPES; ++i){
            gfx::fonts[i] = TTF_OpenFont("data/pixel_font.ttf", font_sizes_array[i]);
        }

        // tree init
        {
            game_state.tree.health = 150;
            game_state.tree.max_health = 150;

            game_state.tree.defense = 50;
            game_state.tree.max_defense = 50;

            game_state.tree.w = 128;
            game_state.tree.h = 128;

            game_state.tree.x = (1024 / 2) - (game_state.tree.w/2);
            game_state.tree.y = (768 / 2) - (game_state.tree.h/2);
        }
        // test wave init
        {
            generate_wave(game_state);
        }
    }

    static void render_gameover(state& game_state, SDL_Renderer* renderer){
    }

    static void render_mainmenu(state& game_state, SDL_Renderer* renderer){
        static float heading_text_scale = 1.0f; 
        heading_text_scale = sinf(SDL_GetTicks() / 500.0f) + 2.5;
        printf("%3.3f\n", heading_text_scale);
        render_centered_dynamically_scaled_text(
                renderer,
                FONT_SIZE_MEDIUM,
                heading_text_scale,
                1024,
                300,
                "Ludum Dare 46 Game",
                gfx::yellow);

        render_centered_text(
                renderer,
                FONT_SIZE_MEDIUM,
                1024,
                700,
                "Enter to Start Game",
                gfx::white);

        render_centered_text(
                renderer,
                FONT_SIZE_MEDIUM,
                1024,
                768,
                "Escape to Quit Game",
                gfx::white);

        render_centered_text(
                renderer,
                FONT_SIZE_SMALL,
                1024,
                1200,
                "Game by Xpost2000 for Ludum Dare 46",
                gfx::white);
    }

    static void render_gameplay(state& game_state, SDL_Renderer* renderer){
        // draw the tree
        {
            gfx::render_rectangle(renderer, 
                    {game_state.tree.x,
                    game_state.tree.y,
                    game_state.tree.w,
                    game_state.tree.h},
                    gfx::white);

            const float bars_height = 8;
            const float bars_start_y_offset = 
                game_state.tree.y + game_state.tree.h + 10;

            // health bar
            {
                gfx::render_rectangle(renderer, 
                        {game_state.tree.x,
                        bars_start_y_offset,
                        game_state.tree.w,
                        bars_height},
                        gfx::red);

                const float health_percentage =
                    (float)game_state.tree.health / (float)game_state.tree.max_health;

                gfx::render_rectangle(renderer, 
                        {game_state.tree.x,
                        bars_start_y_offset,
                        game_state.tree.w * health_percentage,
                        bars_height},
                        gfx::green);
            }

            // defense bar
            {
                gfx::render_rectangle(renderer, 
                        {game_state.tree.x,
                        bars_start_y_offset + bars_height + 5,
                        game_state.tree.w,
                        bars_height},
                        gfx::yellow);

                const float defense_percentage =
                    (float)game_state.tree.defense / (float)game_state.tree.max_defense;

                gfx::render_rectangle(renderer, 
                        {game_state.tree.x,
                        bars_start_y_offset + bars_height + 5,
                        game_state.tree.w,
                        bars_height},
                        gfx::blue);
            }
        }

        // draw enemies
        {
            for(unsigned enemy_index = 0; enemy_index < game_state.enemies_in_wave; ++enemy_index){
                enemy_entity* current_enemy = &game_state.enemies[enemy_index];
                gfx::render_rectangle(renderer, 
                        {current_enemy->x,
                         current_enemy->y,
                         current_enemy->w,
                         current_enemy->h},
                        gfx::white);

                const float bars_height = 4;
                const float bars_start_y_offset = 
                    current_enemy->y + current_enemy->h + 10;

                // health bar
                {
                    gfx::render_rectangle(renderer, 
                            {current_enemy->x,
                            bars_start_y_offset,
                            current_enemy->w,
                            bars_height},
                            gfx::red);

                    const float health_percentage =
                        (float)current_enemy->health / (float)current_enemy->max_health;

                    gfx::render_rectangle(renderer, 
                            {current_enemy->x,
                            bars_start_y_offset,
                            current_enemy->w * health_percentage,
                            bars_height},
                            gfx::green);
                }

                // defense bar
                {
                    gfx::render_rectangle(renderer, 
                            {current_enemy->x,
                            bars_start_y_offset + bars_height + 5,
                            current_enemy->w,
                            bars_height},
                            gfx::yellow);

                    const float defense_percentage =
                        (float)game_state.tree.defense / (float)game_state.tree.max_defense;

                    gfx::render_rectangle(renderer, 
                            {current_enemy->x,
                            bars_start_y_offset + bars_height + 5,
                            current_enemy->w,
                            bars_height},
                            gfx::blue);
                }
            }
        }

        // UI
        const unsigned advance_y = 100;
        float text_y_layout = 100;
        {
            char on_wave_text[255];
            snprintf(on_wave_text, 255, "WAVE: %d", game_state.game_wave);
            gfx::render_centered_text(
                    renderer, 
                    FONT_SIZE_BIG,
                    1024,
                    text_y_layout,
                    on_wave_text, 
                    gfx::white);
        }
        text_y_layout += advance_y;
        {
            char enemies_text[255];
            snprintf(enemies_text, 255, "ENEMIES LEFT: %d", game_state.enemies_in_wave);
            gfx::render_centered_text(
                    renderer, 
                    FONT_SIZE_BIG,
                    1024,
                    text_y_layout,
                    enemies_text, 
                    gfx::white);
        }
    }

    static void render(state& game_state, SDL_Renderer* renderer){
        switch(game_state.screen){
            case GAME_SCREEN_STATE_MAIN_MENU:
            {
                render_mainmenu(game_state, renderer);
            }
            break;
            case GAME_SCREEN_STATE_ATTRACT_MODE:
            {
            }
            break;
            case GAME_SCREEN_STATE_GAMEPLAY:
            {
                render_gameplay(game_state, renderer);
            }
            break;
            case GAME_SCREEN_STATE_GAME_OVER:
            {
                render_gameover(game_state, renderer);
            }
            break;
            case GAME_SCREEN_STATE_PAUSE:
            {
            }
            break;
        }
    }

    static void update_gameover(state& game_state, const float delta_time){
    }

    static void update_mainmenu(state& game_state, const float delta_time){
    }

    static void update_gameplay(state& game_state, const float delta_time){
        // update enemies.
        {
            aabb tree_bounding_box = 
            {
                game_state.tree.x, game_state.tree.y,
                game_state.tree.w, game_state.tree.h
            };
            for(unsigned enemy_index = 0; enemy_index < game_state.enemies_in_wave; ++enemy_index){
                enemy_entity* current_enemy = &game_state.enemies[enemy_index];
                tree_entity* target = &game_state.tree;

                float direction_to_target_x = 0;
                float direction_to_target_y = 0;

                {
                    float delta_x = (target->x + (target->w/2)) - current_enemy->x;
                    float delta_y = (target->y + (target->h/2)) - current_enemy->y;

                    float magnitude_of_vector = 
                        sqrtf( (delta_x * delta_x) + (delta_y * delta_y) );
                    
                    direction_to_target_x = delta_x / magnitude_of_vector;
                    direction_to_target_y = delta_y / magnitude_of_vector;
                }
                aabb enemy_bounding_box = 
                {
                    current_enemy->x, current_enemy->y,
                    current_enemy->w, current_enemy->h,
                };

                bool touching_tree = aabb_intersects(enemy_bounding_box, tree_bounding_box);

                if(!touching_tree){
                    current_enemy->x += direction_to_target_x * current_enemy->speed * delta_time;
                    current_enemy->y += direction_to_target_y * current_enemy->speed * delta_time;
                }
            }
        }
    }

    static void update(state& game_state, const float delta_time){
        switch(game_state.screen){
            case GAME_SCREEN_STATE_MAIN_MENU:
            {
                update_mainmenu(game_state, delta_time);
            }
            break;
            case GAME_SCREEN_STATE_ATTRACT_MODE:
            {
            }
            break;
            case GAME_SCREEN_STATE_GAMEPLAY:
            {
                update_gameplay(game_state, delta_time);
            }
            break;
            case GAME_SCREEN_STATE_GAME_OVER:
            {
                update_gameover(game_state, delta_time);
            }
            break;
            case GAME_SCREEN_STATE_PAUSE:
            {
            }
            break;
        }
    }
}

int main(int argc, char** argv){
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    SDL_Window* window =
        SDL_CreateWindow("LD46: Keep it Alive!", 
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                window_width,
                window_height,
                SDL_WINDOW_SHOWN
                );
    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    uint32_t current = SDL_GetTicks();
    uint32_t last = current;
    uint32_t difference = 0;

    float delta_time = 1/60.0f;

    game::state state = {};

    game::initialize(state, renderer);

    while(game_running){
        current = SDL_GetTicks();

        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                {
                    game_running = false;
                }
                break;
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEMOTION:
                {
                    game::handle_mouse_input(state, &event);
                }
                break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                {
                    game::handle_key_input(state, &event);
                }
                break;
            }
        }
        game::update(state, delta_time);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        game::render(state, renderer);

        SDL_RenderPresent(renderer);

        // probably really dumb
        last = SDL_GetTicks();
        difference = last - current;

        delta_time = 1/60.0f;//(difference / 1000.0f);
    }

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
