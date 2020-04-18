/*
 * I'm very much aware all the types here are nearly the same
 * thing...
 *
 * @NOTE jerry:
 * I do not advocate ever programming like this.
 * This is just what happens when I try to go as fast as I can,
 * and it doesn't go so greatly.
 *
 * "Because UNLESS someone like you cares a whole awful lot. It's
 *  not going to get better. It's not..."
 *
 *  Also why did I not use templates...
 *  I'm going to be honest I kind of forgot lmao.
 */
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

static constexpr unsigned int window_width = 1440;
static constexpr unsigned int window_height = 900;

static bool game_running = true;

enum font_sizes{
    FONT_SIZE_SMALL,
    FONT_SIZE_MEDIUM,
    FONT_SIZE_BIG,

    FONT_SIZE_TYPES
};

namespace gfx{
    TTF_Font* fonts[FONT_SIZE_TYPES];
    SDL_Texture* circle_texture;

    SDL_Texture* load_image_to_texture(SDL_Renderer* renderer, const char* path){
        SDL_Surface* surface = IMG_Load(path);
        SDL_Texture* result = 
            SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        return result;
    }

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

    static void render_circle(
            SDL_Renderer* renderer,
            float x, float y,
            float radius,
            const color c){
        SDL_Rect as_sdl_rect = {
            x - radius,
            y - radius,
            radius*2, 
            radius*2,
        };

        SDL_SetTextureColorMod(circle_texture, c.r, c.g, c.b);
        SDL_SetTextureAlphaMod(circle_texture, c.a);
        SDL_RenderCopy(renderer, circle_texture, NULL, &as_sdl_rect);
    }

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
        ENEMY_NULL,

        ENEMY_BASIC_LUMBERJACK,
        ENEMY_SKINNY_LUMBERJACK,
        ENEMY_FAT_LUMBERJACK,
        ENEMY_ANARCHIST,
        ENEMY_LOGGING_MACHINES,
        ENEMY_ANTHROPORMORPHIC_ANT,

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

        float speed;

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

    // Mines are not turrets.
    enum turret_type{
        TURRET_NULL,

        TURRET_SINGLE_SHOOTER,
        TURRET_REPEATER,

        TURRET_FREEZER
    };

    enum projectile_type{
        PROJECTILE_NULL,

        PROJECTILE_BULLET,

        PROJECTILE_EXPLOSIVE,
    };

    struct turret_projectile{
        projectile_type type;

        float lifetime;

        float x;
        float y;
        float w;
        float h;

        float speed;

        // vector.
        float direction_x;
        float direction_y;

        float damage;
        float defense_damage;
    };

    struct turret_unit{
        turret_type type;

        float x;
        float y;
        float w;
        float h;

        float health;
        float max_health;

        float defense;
        float max_defense;

        float targetting_radius;

        float fire_rate_delay;
        float fire_rate_timer;
    };

    static constexpr size_t MAX_ENEMIES_IN_GAME = 512;
    static constexpr size_t MAX_TURRETS_IN_GAME = 512;
    static constexpr size_t MAX_PROJECTILES_IN_GAME = 512;

    // mostly for button presses.
    struct input_state{
        bool escape_down;
        bool w_down;
        bool return_down;
    };

    struct wave_spawn_list_entry{
        enemy_type type;
        float x;
        float y;
    };

    struct state{
        input_state last_input;
        input_state input; 
        game_screen_state screen;

#if 0
        float time_passed;
#endif

        bool wave_started;
        int game_wave;

        int points;

        turret_unit turret_units[MAX_TURRETS_IN_GAME];
        turret_projectile projectiles[MAX_PROJECTILES_IN_GAME];

        int enemies_in_wave;
        enemy_entity enemies[MAX_ENEMIES_IN_GAME];

        float wave_spawn_delay;
        float wave_spawn_timer;
        int wave_spawn_list_entry_count;
        wave_spawn_list_entry wave_spawn_list[MAX_ENEMIES_IN_GAME];

        float time_until_wave_ends;
        float preparation_timer;

        tree_entity tree;
    };

    static void clear_wave_spawn_list(state& game_state){
        for(int wave_entry = 0; 
            wave_entry < MAX_ENEMIES_IN_GAME;
            ++wave_entry){
            game_state.wave_spawn_list[wave_entry].type = ENEMY_NULL;
        }
    }

    static void push_wave_entry(state& game_state,
            enemy_type push_entry_type,
            float x,
            float y){
        unsigned free_index = 0;
        {
            for(unsigned wave_entry = 0;
                wave_entry < MAX_ENEMIES_IN_GAME;
                ++wave_entry){
                if(game_state.wave_spawn_list[wave_entry].type == ENEMY_NULL){
                    free_index = wave_entry;
                    break;
                }
            }
        }

        wave_spawn_list_entry* spawn_list_entry =
            &game_state.wave_spawn_list[free_index]; 

        spawn_list_entry->type = push_entry_type;
        spawn_list_entry->x = x;
        spawn_list_entry->y = y;
        
        game_state.wave_spawn_list_entry_count++;
    }

    static void push_turret_unit(state& game_state,
            const int x,
            const int y,
            turret_unit to_place){
        unsigned free_index = 0;
        {
            for(unsigned turret_entry = 0;
                turret_entry < MAX_TURRETS_IN_GAME;
                ++turret_entry){
                turret_unit* current_turret = 
                    &game_state.turret_units[turret_entry];
                if(current_turret->type == TURRET_NULL){
                    free_index = turret_entry;
                    break;
                }
            }
        }

        turret_unit* free_unit = 
            &game_state.turret_units[free_index];

        (*free_unit) = to_place;

        free_unit->x = x;
        free_unit->y = y;
    }

    static void place_selected_unit(state& game_state,
            const int mouse_x,
            const int mouse_y){
        // for now just plant a dumb single shooter.
        // this can fail btw? 
        turret_unit to_place = {};
        to_place.type = TURRET_SINGLE_SHOOTER;
        to_place.fire_rate_delay = 2.5f;
        to_place.w = 64;
        to_place.h = 64;
        to_place.health = 100;
        to_place.max_health = 100;

        to_place.targetting_radius = 150;

        to_place.defense = 100;
        to_place.max_defense = 100;
        if(game_state.points - 300 >= 0){
            push_turret_unit(game_state, mouse_x - (to_place.w / 2), mouse_y - (to_place.h / 2), to_place);
            game_state.points -= 300;
        }else{

        }
    }

    static enemy_entity generate_enemy_of_type(enemy_type type, float x, float y){
        enemy_entity result = {};

        result.type = type;
        result.x = x;
        result.y = y;

        result.w = 32;
        result.h = 32;

        // stupid
        const float pixel_speed_scale = 60.0f;

        switch(type){
            case ENEMY_BASIC_LUMBERJACK:
            {
                result.max_health = 100;
                result.health = 100;

                result.max_defense = 30;
                result.defense = 30;
                result.speed = 1.45f * pixel_speed_scale;
            }
            break;
            case ENEMY_SKINNY_LUMBERJACK:
            {
                result.max_health = 45;
                result.health = 45;

                result.max_defense = 15;
                result.defense = 15;
                result.speed = 2.85f * pixel_speed_scale;
            }
            break;
            case ENEMY_FAT_LUMBERJACK:
            {
                result.max_health = 250;
                result.health = 250;

                result.max_defense = 85;
                result.defense = 85;
                result.speed = 1.00f * pixel_speed_scale;
                result.w *= 3;
                result.h *= 3;
            }
            break;
            case ENEMY_ANARCHIST:
            {
                result.max_health = 50;
                result.health = 50;

                result.max_defense = 0;
                result.defense = 0;
                result.speed = 3.45f * pixel_speed_scale;
            }
            break;
            case ENEMY_LOGGING_MACHINES:
            {
                result.max_health = 400;
                result.health = 400;

                result.max_defense = 100;
                result.defense = 100;
                result.speed = 0.85f * pixel_speed_scale;

                result.w *= 5;
                result.h *= 5;
            }
            break;
            case ENEMY_ANTHROPORMORPHIC_ANT:
            {
                result.max_health = 50;
                result.health = 50;

                result.max_defense = 30;
                result.defense = 30;
                result.speed = 3.45f * pixel_speed_scale;
            }
            break;
        }

        return result;
    }

    // probably going to take a fixed list of objects to
    // spawn......
    //
    // I could implement a weighted random list really quickly I guess...
    static void generate_wave(state& game_state){
        // Each entry should probably set their own spawn timer
        // so the game looks like it's taking "random" time for
        // generation
        game_state.wave_spawn_delay = 2.45f;
        game_state.wave_spawn_timer = game_state.wave_spawn_delay;

        for(int i = 0; i < 10; ++i){
            const float angle = i * 36;
            const float radians = angle * (M_PI / 180.0f);
            const float radius = 450;

            push_wave_entry(game_state, 
                    ENEMY_BASIC_LUMBERJACK,
                    (cosf(radians)+1) * radius,
                    (sinf(radians)+1) * radius);
        }
    }

    static void setup_game(state& game_state){
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
        // should be enough to build a few units or something like that.
        game_state.points = 3000;

        // null empty every single list...
        {
            for(unsigned turret_entry = 0;
                turret_entry < MAX_TURRETS_IN_GAME;
                ++turret_entry){
                game_state.turret_units[turret_entry].type = TURRET_NULL;
            }

            for(unsigned projectile_entry = 0;
                projectile_entry < MAX_PROJECTILES_IN_GAME;
                ++projectile_entry){
                game_state.projectiles[projectile_entry].type = PROJECTILE_NULL;
            }

            for(unsigned enemy_entry = 0;
                enemy_entry < MAX_ENEMIES_IN_GAME;
                ++enemy_entry){
                game_state.wave_spawn_list[enemy_entry] = wave_spawn_list_entry {};
                game_state.enemies[enemy_entry] = enemy_entity{};
            }

            game_state.wave_spawn_timer = 0;
            game_state.wave_spawn_list_entry_count = 0;
            game_state.enemies_in_wave = 0;
            game_state.time_until_wave_ends = 0;
            game_state.preparation_timer = 0;

            game_state.game_wave = 0;
            game_state.wave_started = false;
        }

        // test wave init
        {
            generate_wave(game_state);
        }
    }

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

                if(game_state.screen == GAME_SCREEN_STATE_GAMEPLAY){
                    if(button_down){
                        if(button == SDL_BUTTON_LEFT){
                            // place selected unit and pay.
                            place_selected_unit(game_state, mouse_x, mouse_y);
                        }else if(button == SDL_BUTTON_MIDDLE){
                        }else if(button == SDL_BUTTON_RIGHT){
                            // destroy selected unit and refund.
                        }
                    }
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

    static void handle_input(state& game_state, const float delta_time){
        if(!game_state.input.w_down &&
           game_state.last_input.w_down){
            game_state.tree.health -= 10;
        }

        if(!game_state.input.escape_down &&
           game_state.last_input.escape_down){
            if(game_state.screen == GAME_SCREEN_STATE_MAIN_MENU){
                game_running = false;
            }else if(game_state.screen == GAME_SCREEN_STATE_GAME_OVER){
                game_state.screen = GAME_SCREEN_STATE_MAIN_MENU;
            }
        }

        if(!game_state.input.return_down &&
           game_state.last_input.return_down){
            if(game_state.screen == GAME_SCREEN_STATE_MAIN_MENU){
                game_state.screen = GAME_SCREEN_STATE_GAMEPLAY;
                setup_game(game_state);
            }else if(game_state.screen == GAME_SCREEN_STATE_GAME_OVER){
                game_state.screen = GAME_SCREEN_STATE_GAMEPLAY;
                setup_game(game_state);
            }
        }
    }

    static void handle_key_input(state& game_state, SDL_Event* event){
        int scancode = event->key.keysym.scancode;
        bool keydown = event->type == SDL_KEYDOWN;

        switch(scancode){
            case SDL_SCANCODE_W:
            {
                game_state.input.w_down = keydown;
            }
            break;
            case SDL_SCANCODE_RETURN:
            {
                game_state.input.return_down = keydown;
            }
            break;
            case SDL_SCANCODE_ESCAPE:
            {
                game_state.input.escape_down = keydown;
            }
            break;
        }
    }
    
    static void push_enemy(state& game_state, const enemy_entity to_spawn){
        unsigned free_index = 0;
        {
            for(unsigned enemy_index = 0;
                enemy_index < MAX_ENEMIES_IN_GAME;
                ++enemy_index){
                enemy_entity* entry = 
                    &game_state.enemies[enemy_index];
                
                bool can_replace = (entry->type == ENEMY_NULL);

                if(can_replace){
                    free_index = enemy_index;
                    break;
                }
            }
        }

        game_state.enemies_in_wave++;

        enemy_entity* free_entry = 
            &game_state.enemies[free_index];

        (*free_entry) = to_spawn;
    }

    static void push_projectile(state& game_state, turret_projectile projectile){
        unsigned free_index = 0;
        {
            for(unsigned projectile_index = 0;
                projectile_index < MAX_PROJECTILES_IN_GAME;
                ++projectile_index){
                turret_projectile* entry =
                    &game_state.projectiles[projectile_index];

                bool can_replace = (entry->type == PROJECTILE_NULL) ||
                                   (entry->lifetime <= 0.0f);

                if(can_replace){
                    free_index = projectile_index;
                    break;
                }
            }
        }

        turret_projectile* free_entry =
            &game_state.projectiles[free_index];

        (*free_entry) = projectile;
    }

    static void turret_fire_projectile_at_point(state& game_state, 
            turret_unit* turret,
            float target_x,
            float target_y){
        if(turret->fire_rate_timer <= 0.0f){
            turret->fire_rate_timer = turret->fire_rate_delay;
            // fire a projectile
            {
                turret_projectile projectile = {};
                projectile.type = PROJECTILE_BULLET;

                float turret_center_x = turret->x + (turret->w / 2);
                float turret_center_y = turret->y + (turret->h / 2);

                projectile.x = turret_center_x;
                projectile.y = turret_center_y;

                projectile.w = 16;
                projectile.h = 16;

                // 2 mins and 30 seconds?
                projectile.lifetime = 150;

                projectile.speed = 200;

                projectile.damage = 35;
                projectile.defense_damage = 15;

                // calculate direction it goes.
                // maybe have a seeking one...
                {
                    float position_delta_x = target_x - turret_center_x;
                    float position_delta_y = target_y - turret_center_y;

                    float magnitude_of_vector = 
                        sqrtf( (position_delta_x * position_delta_x) + 
                               (position_delta_y * position_delta_y) );
                    
                    projectile.direction_x = position_delta_x / magnitude_of_vector;
                    projectile.direction_y = position_delta_y / magnitude_of_vector;
                }

                push_projectile(game_state, projectile);
            }
        }
    }

    static void initialize(state& game_state, SDL_Renderer* renderer){
        static int font_sizes_array[FONT_SIZE_TYPES] = 
        { 16, 32, 64 };
        for(int i = 0; i < FONT_SIZE_TYPES; ++i){
            gfx::fonts[i] = TTF_OpenFont("data/pixel_font.ttf", font_sizes_array[i]);
        }
    }

    static void render_gameover(state& game_state, SDL_Renderer* renderer){
        static float heading_text_scale = 1.0f; 
        heading_text_scale = sinf(SDL_GetTicks() / 500.0f) + 2.5;
        render_centered_dynamically_scaled_text(
                renderer,
                FONT_SIZE_MEDIUM,
                heading_text_scale,
                1024,
                300,
                "Game Over!",
                gfx::yellow);

        render_centered_text(
                renderer,
                FONT_SIZE_MEDIUM,
                1024,
                768,
                "Enter to Restart Game",
                gfx::white);

        render_centered_text(
                renderer,
                FONT_SIZE_MEDIUM,
                1024,
                900,
                "Escape to Return to Menu",
                gfx::white);
    }

    static void render_mainmenu(state& game_state, SDL_Renderer* renderer){
        static float heading_text_scale = 1.0f; 
        heading_text_scale = sinf(SDL_GetTicks() / 500.0f) + 2.5;
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
            for(unsigned enemy_index = 0; enemy_index < MAX_ENEMIES_IN_GAME; ++enemy_index){
                enemy_entity* current_enemy = &game_state.enemies[enemy_index];
                if(current_enemy->type != ENEMY_NULL){
                    gfx::render_rectangle(renderer, 
                            {
                            current_enemy->x,
                            current_enemy->y,
                            current_enemy->w,
                            current_enemy->h
                            },
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
        }

        // draw turrets
        // these do not have the health bar.
        // these will be visually redrawn to look different.
        {
            for(unsigned turret_entry = 0;
                turret_entry < MAX_TURRETS_IN_GAME;
                ++turret_entry){
                turret_unit* current_unit =
                    &game_state.turret_units[turret_entry];

                if(current_unit->type != TURRET_NULL){
                    // everything is still top left aligned....
                    //
                    // I have to actually hack in the calculations
                    // because nothing is centered.... Damn.
#if 1
                    gfx::color circle_color = gfx::blue;
                    circle_color.a = 100;
                    gfx::render_circle(renderer,
                            current_unit->x + (current_unit->w / 2),
                            current_unit->y + (current_unit->h / 2),
                            current_unit->targetting_radius,
                            circle_color);
#endif

                    gfx::render_rectangle(renderer, 
                            {
                            current_unit->x,
                            current_unit->y,
                            current_unit->w,
                            current_unit->h
                            },
                            gfx::green);
                }
            }
        }

        // draw projectiles
        {
            for(unsigned projectile_entry = 0;
                projectile_entry < MAX_PROJECTILES_IN_GAME;
                ++projectile_entry){
                turret_projectile* current_projectile =
                    &game_state.projectiles[projectile_entry];

                if(current_projectile->type != PROJECTILE_NULL){
                    gfx::render_rectangle(renderer, 
                            {
                            current_projectile->x,
                            current_projectile->y,
                            current_projectile->w,
                            current_projectile->h
                            },
                            gfx::white);
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
            char expect_enemies_text[255];
            snprintf(expect_enemies_text, 255, "ENEMIES IN WAVE: %d", game_state.wave_spawn_list_entry_count);
            gfx::render_centered_text(
                    renderer, 
                    FONT_SIZE_BIG,
                    1024,
                    text_y_layout,
                    expect_enemies_text, 
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
        text_y_layout += advance_y;
        {
            char points_to_spend_text[255];
            snprintf(points_to_spend_text, 255, "$%d", game_state.points);
            gfx::render_centered_text(
                    renderer, 
                    FONT_SIZE_BIG,
                    1024,
                    text_y_layout,
                    points_to_spend_text, 
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
        // handle wave spawning
        // and other wave logic.
        {
            game_state.wave_spawn_timer -= delta_time;
            // spawn from the first thing in the list.
            if(game_state.wave_spawn_timer <= 0.0f){
                game_state.wave_spawn_timer = game_state.wave_spawn_delay;
                bool wave_empty = true;
                for(unsigned wave_entry = 0;
                        wave_entry < MAX_ENEMIES_IN_GAME;
                        ++wave_entry){
                    wave_spawn_list_entry* entry =
                        &game_state.wave_spawn_list[wave_entry];

                    if(entry->type != ENEMY_NULL){
                        // actually spawn...
                        {
                            enemy_entity to_spawn =
                                generate_enemy_of_type(entry->type, 
                                        entry->x,
                                        entry->y);

                            push_enemy(game_state, to_spawn);

                            entry->type = ENEMY_NULL;
                            game_state.wave_spawn_list_entry_count--;
                        }
                        wave_empty = false;
                        break;
                    }
                }
            }
        }

        // update turrets.
        {
            for(unsigned turret_index = 0;
                turret_index < MAX_TURRETS_IN_GAME;
                ++turret_index){
                turret_unit* current_turret = 
                    &game_state.turret_units[turret_index];

                if(current_turret->type != TURRET_NULL){
                    current_turret->fire_rate_timer -= delta_time;

                    for(unsigned enemy_index = 0;
                        enemy_index < MAX_ENEMIES_IN_GAME;
                        ++enemy_index){
                        enemy_entity* current_enemy =
                            &game_state.enemies[enemy_index];

                        if(current_enemy->type != ENEMY_NULL &&
                           current_enemy->health > 0){

                            float delta_x = current_enemy->x - current_turret->x;
                            float delta_y = current_enemy->y - current_turret->y;
                            float distance_between_turret_and_enemy =
                                sqrtf((delta_y * delta_y) + (delta_x * delta_x));

                            if(distance_between_turret_and_enemy <= current_turret->targetting_radius){
                                // it will attempt to fire...
                                // if it can...
                                turret_fire_projectile_at_point(game_state, current_turret, 
                                        current_enemy->x + (current_enemy->w / 2), 
                                        current_enemy->y + (current_enemy->h / 2));
                            }
                        }
                    } 
                }
            }
        }

        // update projectiles
        {
            for(unsigned projectile_index = 0;
                projectile_index < MAX_PROJECTILES_IN_GAME;
                ++projectile_index){
                turret_projectile* projectile = 
                    &game_state.projectiles[projectile_index];

                if(projectile->type != PROJECTILE_NULL){
                    projectile->x += (projectile->direction_x * projectile->speed) * delta_time;
                    projectile->y += (projectile->direction_y * projectile->speed) * delta_time;

                    projectile->lifetime -= delta_time;

                    aabb projectile_bounding_box =
                    {
                        projectile->x,
                        projectile->y,
                        projectile->w,
                        projectile->h
                    };

                    for(unsigned enemy_index = 0;
                        enemy_index < MAX_ENEMIES_IN_GAME;
                        ++enemy_index){
                        enemy_entity* current_enemy =
                            &game_state.enemies[enemy_index];

                        if(current_enemy->type != ENEMY_NULL &&
                           current_enemy->health > 0){

                            aabb enemy_bounding_box =
                            {
                                current_enemy->x,
                                current_enemy->y,
                                current_enemy->w,
                                current_enemy->h
                            };

                            if(aabb_intersects(projectile_bounding_box, enemy_bounding_box)){
                                // for now straight up kill the enemy;
                                // @TODO jerry : ^_^
                                projectile->type = PROJECTILE_NULL;
                                projectile->lifetime = 0;

                                current_enemy->health = -1000;

                                break;
                            }
                        }
                    } 
                }
            }
        }

        // update enemies.
        {
            aabb tree_bounding_box = 
            {
                game_state.tree.x, game_state.tree.y,
                game_state.tree.w, game_state.tree.h
            };
            for(unsigned enemy_index = 0;
                enemy_index < MAX_ENEMIES_IN_GAME;
                ++enemy_index){
                enemy_entity* current_enemy = &game_state.enemies[enemy_index];
                // dead or doesn't exist, skip.
                if(current_enemy->type == ENEMY_NULL || current_enemy->health <= 0){ 
                    if(current_enemy->type != ENEMY_NULL){
                        current_enemy->type = ENEMY_NULL;
                        game_state.enemies_in_wave--;
                    }
                    continue;
                }
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

        // check if game lost
        if(game_state.tree.health <= 0){
            game_state.enemies_in_wave = 0;
            game_state.screen = GAME_SCREEN_STATE_GAME_OVER;
        }
    }

    static void update(state& game_state, const float delta_time){
        /* game_state.input = input_state{}; */
        handle_input(game_state, delta_time);
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
        game_state.last_input = game_state.input;
    }
}

int main(int argc, char** argv){
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    Mix_Init(MIX_INIT_OGG);
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048);
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
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    uint32_t current = SDL_GetTicks();
    uint32_t last = current;
    uint32_t difference = 0;

    float delta_time = 1/60.0f;

    game::state state = {};

    game::initialize(state, renderer);

    gfx::circle_texture = 
        gfx::load_image_to_texture(renderer, "data/circle.png");

    // too lazy to implement my
    // own independent resolution.
    //
    // SDL2 pls work good enough.
    SDL_RenderSetLogicalSize(renderer, 
            virtual_window_width,
            virtual_window_height);

    while(game_running){
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
        current = SDL_GetTicks();
        difference = current - last;
        last = current;

        // last thing I'll do lmao
        delta_time = (difference / 1000.0f);
        delta_time = 1.0f / 60.0f;
    }

    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
