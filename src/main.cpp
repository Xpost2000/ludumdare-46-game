/*
 * I know it's 2020. I'm sorry it runs in 1024 x 768.
 *
 * Also this is not purposely obfuscated, I really was a complete
 * idiot and decided to scratch this in one source file :/, protip
 * I shouldn't do that next time.
 *
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
 *
 *  A stupid amount of globals, and I learned I shouldn't
 *  have done this from scratch. Or rather I shouldn't have gone
 *  cowboy mode.
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include <stdint.h>
#include <stdlib.h>
#include <time.h> // I don't appreciate using rand or srand but speed speed speed.
#include <stdio.h> // emergency?

#include <math.h>

static char* game_name = "Treewatch";
static char* title_string = "Treewatch LD46";

#define STUPID_DEBUG
#undef STUPID_DEBUG

static constexpr unsigned int virtual_window_width = 1024;
static constexpr unsigned int virtual_window_height = 768;

static constexpr unsigned int window_width = 1440;
static constexpr unsigned int window_height = 900;

static bool game_running = true;

template<typename numeric_type>
static const numeric_type clamp(
        numeric_type in_value, // used as intermediate storing value as well.
        const numeric_type min_value,
        const numeric_type max_value){
    if(in_value < min_value){
        in_value = min_value;
    }else{
        if(in_value > max_value){
            in_value = max_value;
        }
    }

    return in_value;
}

enum font_sizes{
    FONT_SIZE_SMALL,
    FONT_SIZE_MEDIUM,
    FONT_SIZE_BIG,

    FONT_SIZE_TYPES
};

// I think I'm done giving wtfs.
namespace audio{
    Mix_Chunk* wave_finished_sound;
    Mix_Chunk* menu_move_sound; 
    Mix_Chunk* menu_select_sound; 

    Mix_Music* menu_music;
    Mix_Music* game_music;

    // required to be global.
    static size_t audio_cyclic_counter = 0;

    static constexpr size_t MAX_EXPLOSION_SOUNDS = 4;
    static constexpr size_t MAX_LOGGING_MACHINE_MOVE_SOUNDS = 2;
    static constexpr size_t MAX_PEW_SOUNDS = 5;
    static constexpr size_t MAX_TREE_HURT_SOUNDS = 5;

    Mix_Chunk* explosion_sounds[MAX_EXPLOSION_SOUNDS];
    Mix_Chunk* logging_move_sounds[MAX_LOGGING_MACHINE_MOVE_SOUNDS];
    Mix_Chunk* pew_sounds[MAX_PEW_SOUNDS];
    Mix_Chunk* tree_hurt_sounds[MAX_TREE_HURT_SOUNDS];
}

namespace gfx{
    TTF_Font* fonts[FONT_SIZE_TYPES];

    // yes real long names very good.
    // wait what
    struct texture_with_origin_info{
        SDL_Texture* texture;
        int width;
        int height;

        float at_scale;

        // percentage.
        float pivot_x;
        float pivot_y;
    };

    SDL_Texture* audio_playing_texture;
    SDL_Texture* audio_mute_texture;

    SDL_Texture* circle_texture;
    SDL_Texture* backdrop_texture;
    texture_with_origin_info turret_texture;
    texture_with_origin_info tree_texture;

    // for the selection UI
    // 0 - logger
    // 1 - lumberjack
    // 2 - big lumberjack
    // 3 - anarchist
    SDL_Texture* enemy_cards[4]; 
    // order same as enums.
    texture_with_origin_info enemy_textures[5];
    // matching turret type.
    SDL_Texture* turret_cards[3];
    SDL_Texture* frame_texture;

    SDL_Texture* load_image_to_texture(SDL_Renderer* renderer, const char* path){
        SDL_Surface* surface = IMG_Load(path);
        SDL_Texture* result = 
            SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        return result;
    }

    // set your own origin info afterwards.
    texture_with_origin_info load_image_to_texture_with_origin_info(SDL_Renderer* renderer, const char* path){
        texture_with_origin_info result = {};

        SDL_Surface* surface = IMG_Load(path);

        result.at_scale = 1;
        result.width = surface->w;
        result.height = surface->h;

        result.texture = 
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
    static color dark_gray = {64, 64, 64, 255};
    static color red = {255, 0, 0, 255};
    static color green = {0, 255, 0, 255};
    static color blue = {0, 0, 255, 255};
    static color cyan = {0, 255, 255, 255};
    static color yellow = {255, 255, 0, 255};

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

    static void render_textured_rectangle(
            SDL_Renderer* renderer,
            SDL_Texture* texture,
            const rectangle rect,
            const color c
            ){
        SDL_Rect as_sdl_rect = {
            rect.x,
            rect.y,
            rect.w,
            rect.h
        };

        SDL_SetTextureColorMod(texture, c.r, c.g, c.b);
        SDL_SetTextureAlphaMod(texture, c.a);
        SDL_RenderCopy(renderer, texture, NULL, &as_sdl_rect);
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
    static constexpr size_t unique_wave_count = 15;
    // cosmetic stuff...
    // and also for taunts!!!
    struct floating_message{
        float x;
        float y;
        char* text;

        float max_lifetime;
        float lifetime;
    };
    static bool music_playing = true;

    // COMMIT DIE????
    // like actually.... WTF??????!!!!
    static constexpr size_t tilemap_height = (virtual_window_height / 32);
    static constexpr size_t tilemap_width = (virtual_window_width / 32);
    uint8_t backdrop_tilemap[tilemap_height][tilemap_width] = 
    {
    };

    // Perlin noise distribution so it doesn't look like poop
    static void sprinkle_in_tilemap(size_t amount, uint8_t what){
        // @TODO jerry: Perlin noise. Blue noise if I can get it...
        // @Afternotes jerry: So about that noise huh???
    }

    static void paint_square_in_tilemap(int x, int y, int w, int h, uint8_t what){
        for(int i = 0; i < h; ++i){
            for(int j = 0; j < w; ++j){
                backdrop_tilemap[i+y][j+x] = what;
            }
        }
    }

    enum game_screen_state{
        GAME_SCREEN_STATE_MAIN_MENU,
        GAME_SCREEN_STATE_ATTRACT_MODE,

        GAME_SCREEN_STATE_GAMEPLAY,
        GAME_SCREEN_STATE_INSTRUCTIONS,
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

        float damage_timer_delay;
        float damage_timer;
        float damage;
        float defense_damage;

        float freeze_time;
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
        TURRET_FREEZER,

        TURRET_TYPE_COUNT
    };

    enum projectile_type{
        PROJECTILE_NULL,

        PROJECTILE_BULLET,
        PROJECTILE_EXPLOSIVE,

        PROJECTILE_TYPE_COUNT
    };

    struct turret_projectile{
        projectile_type type;

        float lifetime;

        float x;
        float y;
        float w;
        float h;

        float speed;
        bool freeze_on_hit;

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

        int cost;
    };

    static constexpr size_t MAX_ENEMIES_IN_GAME = 512;
    static constexpr size_t MAX_TURRETS_IN_GAME = 512;
    static constexpr size_t MAX_PROJECTILES_IN_GAME = 512;
    static constexpr size_t MAX_FLOATING_MESSAGES_IN_GAME = 512;

    // mostly for button presses.
    // this entire thing is one big WTF?
    struct input_state{
        int mouse_x;
        int mouse_y;

        // WTF?
        bool m_down;

        // WTF?
        bool escape_down;
        // WTF?
        bool return_down;

        // WTF?
        bool left_down;
        // WTF?
        bool right_down;

        // WTF?
        bool up_down;
        // WTF?
        bool down_down;
        // WTF?
        bool tab_down;
#ifdef STUPID_DEBUG
        // skip round instantly.
        bool q_down;
        // damage tree.
        bool w_down;
#endif
    };

    struct wave_spawn_list_entry{
        enemy_type type;
        float x;
        float y;
    };

    // wtf.
    struct unit_selection_ui{
        int selected_unit;
    };

    // wtf?
    static constexpr short max_reward_flash_times = 20;
    // wtf?
    static constexpr float max_reward_flash_fx_timer = 0.075f;

    // +1
    static constexpr size_t max_instruction_pages = 5;
    struct state{
        // WTF?
        int instruction_page;
        // WTF?
        int main_menu_option_selection_index;

        unit_selection_ui unit_selection;

        bool paused;
        bool show_wave_preview;

        input_state last_input;
        input_state input; 
        game_screen_state screen;

        floating_message floating_messages[MAX_FLOATING_MESSAGES_IN_GAME];
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

        // WTF am I doing.
        bool reward_flashing_started;
        int flash_iterations;
        float reward_flash_fx_timer;
    };

    static void push_floating_message(state& game_state,
            float x,
            float y,
            char* message,
            float lifetime){
        // here I go again
        unsigned free_index = 0;
        {
            for(unsigned floating_message_entry_index = 0;
                floating_message_entry_index < MAX_FLOATING_MESSAGES_IN_GAME;
                ++floating_message_entry_index){
                floating_message* entry = 
                    &game_state.floating_messages[floating_message_entry_index];

                if(entry->lifetime <= 0.0f){
                    free_index = floating_message_entry_index;
                    break;
                }
            }
        }

        floating_message* free_entry = 
            &game_state.floating_messages[free_index];

        free_entry->x = x;
        free_entry->y = y;
        free_entry->text = message;
        free_entry->lifetime = lifetime;
        free_entry->max_lifetime = lifetime;
    }

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
        bool colliding_with_any = false;
        bool violating_distance_rule = false;

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

        // check for collisions...
        // this is probably slow? :/
        float closest_turret_distance = INFINITY;
        {
            for(unsigned turret_entry = 0;
                turret_entry < MAX_TURRETS_IN_GAME;
                ++turret_entry){
                if(turret_entry != free_index){
                    turret_unit* current_turret = 
                        &game_state.turret_units[turret_entry];
                    if(current_turret->type != TURRET_NULL){
                        aabb want_to_place_at_bounding_box =
                        {
                            x, 
                            y,
                            to_place.w,
                            to_place.h
                        };

                        aabb current_turrent_bounding_box =
                        {
                            current_turret->x,
                            current_turret->y,
                            current_turret->w,
                            current_turret->h
                        };

                        if(aabb_intersects(want_to_place_at_bounding_box, current_turrent_bounding_box)){
                            colliding_with_any = true;
                        }

                        float delta_x = current_turret->x - x;
                        float delta_y = current_turret->y - y;

                        float distance = 
                            sqrtf((delta_x * delta_x) + (delta_y * delta_y));
                        if(distance < closest_turret_distance){
                            closest_turret_distance = distance;
                        }
                    }
                }
            }
        }

        // will be too hard?
        if(closest_turret_distance <= 95.0f){
            violating_distance_rule = true;
        }

        if(!colliding_with_any && !violating_distance_rule){
            turret_unit* free_unit = 
                &game_state.turret_units[free_index];

            (*free_unit) = to_place;

            free_unit->x = x;
            free_unit->y = y;
            game_state.points -= to_place.cost;
        }else{
            if(colliding_with_any){
                push_floating_message(game_state, 
                        x, y, 
                        "You cannot place a unit atop another unit!", 1.5f);
            }else if(violating_distance_rule){
                push_floating_message(game_state, 
                        x, y, 
                        "This unit is too close to another unit!", 1.5f);
            }
        }
    }

    static turret_unit generate_turret_of_type(turret_type type){
        turret_unit result = {};

        result.type = type;

        result.w = 32;
        result.h = 32;

        switch(type){
            case TURRET_SINGLE_SHOOTER:
            {
                result.health = 30;
                result.max_health = 30;

                result.defense = 20;
                result.max_defense = 20;

                result.cost = 500;
                result.targetting_radius = 350;
                result.fire_rate_delay = 1.75f;
            }
            break;
            case TURRET_REPEATER:
            {
                result.health = 40;
                result.max_health = 40;

                result.defense = 40;
                result.max_defense = 40;

                result.cost = 750;

                result.targetting_radius = 250;
                result.fire_rate_delay = 0.75f;
            }
            break;
            case TURRET_FREEZER:
            {
                result.health = 60;
                result.max_health = 60;

                result.defense = 40;
                result.max_defense = 40;

                result.cost = 1500;
                result.targetting_radius = 250;
                result.fire_rate_delay = 2.00f;
            }
            break;
        }

        return result;
    }

    static void place_selected_unit(state& game_state,
            const int mouse_x,
            const int mouse_y){
        // for now just plant a dumb single shooter.
        // this can fail btw? 
        turret_unit to_place = 
            generate_turret_of_type((turret_type)(game_state.unit_selection.selected_unit+1));

        if(game_state.points - to_place.cost >= 0){
            push_turret_unit(game_state, mouse_x - (to_place.w / 2), mouse_y - (to_place.h / 2), to_place);
            Mix_PlayChannel(-1, 
                    audio::explosion_sounds[(audio::audio_cyclic_counter++) % audio::MAX_EXPLOSION_SOUNDS],
                    0);
        }else{
            push_floating_message(game_state, 
                    mouse_x, mouse_y, 
                    "You cannot afford this unit!", 1.5f);
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
                result.max_health = 125;
                result.health = 125;

                result.max_defense = 30;
                result.defense = 30;
                result.speed = 1.25f * pixel_speed_scale;

                result.damage_timer_delay = 1.6f;
                result.damage = 20;
                result.defense_damage = 15;
            }
            break;
            case ENEMY_SKINNY_LUMBERJACK:
            {
                result.max_health = 45;
                result.health = 45;

                result.max_defense = 15;
                result.defense = 15;
                result.speed = 2.05f * pixel_speed_scale;

                result.damage_timer_delay = 1.0f;
                result.damage = 10;
                result.defense_damage = 2;
            }
            break;
            case ENEMY_FAT_LUMBERJACK:
            {
                result.max_health = 350;
                result.health = 350;

                result.max_defense = 85;
                result.defense = 85;
                result.speed = 0.90f * pixel_speed_scale;
                result.w *= 3;
                result.h *= 3;

                result.damage_timer_delay = 3.0f;
                result.damage = 30;
                result.defense_damage = 15;
            }
            break;
            case ENEMY_ANARCHIST:
            {
                result.max_health = 50;
                result.health = 50;

                result.max_defense = 0;
                result.defense = 0;
                result.speed = 3.15f * pixel_speed_scale;

                result.damage_timer_delay = 0.65;
                result.damage = 5;
                result.defense_damage = 2;
            }
            break;
            case ENEMY_LOGGING_MACHINES:
            {
                result.max_health = 450;
                result.health = 450;

                result.max_defense = 100;
                result.defense = 100;
                result.speed = 0.65f * pixel_speed_scale;

                result.w *= 5;
                result.h *= 5;

                result.damage_timer_delay = 4.5f;
                result.damage = 40;
                result.defense_damage = 40;
            }
            break;
            /* case ENEMY_ANTHROPORMORPHIC_ANT: */
            /* { */
            /*     result.max_health = 50; */
            /*     result.health = 50; */

            /*     result.max_defense = 30; */
            /*     result.defense = 30; */
            /*     result.speed = 3.25f * pixel_speed_scale; */
            /* } */
            /* break; */
        }

        return result;
    }

    // probably going to take a fixed list of objects to
    // spawn......
    //
    // I could implement a weighted random list really quickly I guess...
    static void generate_wave(state& game_state){
        // weird syntax wtf.
        static struct wave_probabilities{ 
            int weights[ENEMY_TYPE_COUNT-1]; 

            int randomly_pick(void){
                int weights_summed = 0;

                for(unsigned weight_index = 0;
                    weight_index < ENEMY_TYPE_COUNT-1;
                    ++weight_index){
                    weights_summed += weights[weight_index];
                }

                int weight_versus = rand() % weights_summed;

                for(unsigned weight_index = 0;
                    weight_index < ENEMY_TYPE_COUNT-1;
                    ++weight_index){
                    if(weight_versus < weights[weight_index]){
                        return weight_index+1;
                    }

                    weight_versus -= weights[weight_index];
                }

                // should never happen.
                return -1;
            }

        }probabilities[unique_wave_count];
        //wave 1 - 3
        {
            for(unsigned i = 0; i < 3; ++i){
                wave_probabilities* current = &probabilities[i];
                current->weights[ENEMY_BASIC_LUMBERJACK-1] = 70;
                current->weights[ENEMY_SKINNY_LUMBERJACK-1] = 40;
                current->weights[ENEMY_FAT_LUMBERJACK-1] = 15;
                current->weights[ENEMY_ANARCHIST-1] = 20;
                current->weights[ENEMY_LOGGING_MACHINES-1] = 5;
            }
        }


        //wave 3 - 6
        {
            for(unsigned i = 2; i < 6; ++i){
                wave_probabilities* current = &probabilities[i];
                current->weights[ENEMY_BASIC_LUMBERJACK-1] = 70;
                current->weights[ENEMY_SKINNY_LUMBERJACK-1] = 40;
                current->weights[ENEMY_FAT_LUMBERJACK-1] = 20;
                current->weights[ENEMY_ANARCHIST-1] = 25;
                current->weights[ENEMY_LOGGING_MACHINES-1] = 10;
            }
        }

        //wave 6 - 9
        {
            for(unsigned i = 5; i < 9; ++i){
                wave_probabilities* current = &probabilities[i];
                current->weights[ENEMY_BASIC_LUMBERJACK-1] = 70;
                current->weights[ENEMY_SKINNY_LUMBERJACK-1] = 30;
                current->weights[ENEMY_FAT_LUMBERJACK-1] = 35;
                current->weights[ENEMY_ANARCHIST-1] = 35;
                current->weights[ENEMY_LOGGING_MACHINES-1] = 20;
            }
        }

        //wave 9 - 15
        {
            for(unsigned i = 8; i < 15; ++i){
                wave_probabilities* current = &probabilities[i];
                current->weights[ENEMY_BASIC_LUMBERJACK-1] = 80;
                current->weights[ENEMY_SKINNY_LUMBERJACK-1] = 10;
                current->weights[ENEMY_FAT_LUMBERJACK-1] = 50;
                current->weights[ENEMY_ANARCHIST-1] = 45;
                current->weights[ENEMY_LOGGING_MACHINES-1] = 30;
            }
        }
        // 15 unique waves...
        static int wave_enemy_counts[unique_wave_count] = {
            15, 15, 15, 20, 20, 22, 24, 25, 25, 30, 30, 35, 35, 40, 45
        };
        int clamped_index = clamp<int>(game_state.game_wave, 0, unique_wave_count-1);
        // Each entry should probably set their own spawn timer
        // so the game looks like it's taking "random" time for
        // generation
        game_state.wave_spawn_delay = 2.05f;
        game_state.wave_spawn_timer = game_state.wave_spawn_delay;

        game_state.game_wave++;

        for(int i = 0; 
            i < wave_enemy_counts[clamped_index]; 
            ++i){
            const float angle = static_cast<int>(rand() % 360+1);
            const float radians = angle * (M_PI / 180.0f);
            const float radius = 740;

            push_wave_entry(game_state, 
                    // skip ENEMY_NULL
                    static_cast<enemy_type>(probabilities[clamped_index].randomly_pick()),
                    (cosf(radians)+0.5) * radius ,
                    (sinf(radians)+0.5) * radius);
        }
    }

    static void finished_round(state& game_state){
        // clear arrays...
        for(unsigned enemy_entry = 0;
                enemy_entry < MAX_ENEMIES_IN_GAME;
                ++enemy_entry){
            game_state.wave_spawn_list[enemy_entry] = wave_spawn_list_entry {};
            game_state.enemies[enemy_entry] = enemy_entity{};
        }

        for(unsigned projectile_entry = 0;
            projectile_entry < MAX_PROJECTILES_IN_GAME;
            ++projectile_entry){
            game_state.projectiles[projectile_entry].type = PROJECTILE_NULL;
        }

        game_state.wave_spawn_timer = 0;
        game_state.wave_spawn_list_entry_count = 0;
        game_state.enemies_in_wave = 0;
        game_state.time_until_wave_ends = 0;
        game_state.preparation_timer = 0;

        game_state.wave_started = false;
        generate_wave(game_state);
    }

    static void setup_game(state& game_state){
        // tree init
        {
            game_state.tree.health = 350;
            game_state.tree.max_health = 350;

            game_state.tree.defense = 50;
            game_state.tree.max_defense = 50;

            game_state.tree.w = 64;
            game_state.tree.h = 64;

            game_state.tree.x = (1024 / 2) - (game_state.tree.w/2);
            game_state.tree.y = (768 / 2) - (game_state.tree.h/2);
        }
        // should be enough to build a few units or something like that.
        game_state.points = 7500;

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

            game_state.game_wave = 0;
        }
        // gen test list
        {
            finished_round(game_state);
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
                            if(!game_state.wave_started && !game_state.show_wave_preview){
                                place_selected_unit(game_state, mouse_x, mouse_y);
                            }
                        }else if(button == SDL_BUTTON_MIDDLE){
                        }else if(button == SDL_BUTTON_RIGHT){
                            // destroy selected unit and refund.
                            for(unsigned turret_entry = 0;
                                turret_entry < MAX_TURRETS_IN_GAME;
                                ++turret_entry){
                                turret_unit* current_turret = 
                                    &game_state.turret_units[turret_entry];
                                if(current_turret->type != TURRET_NULL){
                                    aabb current_turret_bounding_box =
                                    {
                                        current_turret->x,
                                        current_turret->y,
                                        current_turret->w,
                                        current_turret->h
                                    };

                                    aabb mouse_bounding_box = 
                                    {
                                        mouse_x,
                                        mouse_y,
                                        current_turret->w,
                                        current_turret->h,
                                    };

                                    if(aabb_intersects(current_turret_bounding_box, mouse_bounding_box)){
                                        // removing and refunding.
                                        push_floating_message(game_state, 
                                                mouse_x, mouse_y, 
                                                "Removing and refunding unit.", 1.5f);
                                        current_turret->type = TURRET_NULL;
                                        game_state.points += current_turret->cost;
                                        Mix_PlayChannel(-1, 
                                                audio::explosion_sounds[(audio::audio_cyclic_counter++) % audio::MAX_EXPLOSION_SOUNDS],
                                                0);
                                    }
                                }
                            }
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

        game_state.input.mouse_x = mouse_x;
        game_state.input.mouse_y = mouse_y;
    }

    static void handle_input(state& game_state, const float delta_time){
#ifdef STUPID_DEBUG
        if(!game_state.input.w_down &&
           game_state.last_input.w_down){
            game_state.tree.health -= 10;
        }

        if(!game_state.input.q_down &&
            game_state.last_input.q_down){
            game_state.enemies_in_wave = 0;
            game_state.wave_spawn_list_entry_count = 0;
        }
#endif
        if(!game_state.input.m_down &&
            game_state.last_input.m_down){
            if(music_playing){
                Mix_VolumeMusic(0);
                music_playing = false;
            }else{
                Mix_VolumeMusic(MIX_MAX_VOLUME);
                music_playing = true;
            }
        }

        if(!game_state.input.escape_down &&
           game_state.last_input.escape_down){
            if(game_state.screen == GAME_SCREEN_STATE_MAIN_MENU){
                game_running = false;
            }else if(game_state.screen == GAME_SCREEN_STATE_GAME_OVER){
                game_state.screen = GAME_SCREEN_STATE_MAIN_MENU;
            }else if(game_state.screen == GAME_SCREEN_STATE_GAMEPLAY){
                game_state.screen = GAME_SCREEN_STATE_MAIN_MENU;
                game_state.paused = true;
            }else if(game_state.screen == GAME_SCREEN_STATE_INSTRUCTIONS){
                game_state.screen = GAME_SCREEN_STATE_MAIN_MENU;
            }
        }

        if(!game_state.input.return_down &&
           game_state.last_input.return_down){
            if(game_state.screen == GAME_SCREEN_STATE_MAIN_MENU){
                switch(game_state.main_menu_option_selection_index){
                    case 0:
                    {
                        game_state.screen = GAME_SCREEN_STATE_GAMEPLAY;
                        if(!game_state.paused){
                            setup_game(game_state);
                        }else{
                            game_state.paused = false;
                        }
                    }
                    break;
                    case 1:
                    {
                        game_state.screen = GAME_SCREEN_STATE_INSTRUCTIONS;
                        game_state.instruction_page = 0;
                    }
                    break;
                    case 2:
                    {
                        game_running = false;
                    }
                    break;
                }
                Mix_PlayChannel(-1, audio::menu_select_sound, 0);
            }else if(game_state.screen == GAME_SCREEN_STATE_GAME_OVER){
                game_state.screen = GAME_SCREEN_STATE_GAMEPLAY;
                Mix_PlayChannel(-1, audio::menu_select_sound, 0);
                setup_game(game_state);
            }else if(game_state.screen == GAME_SCREEN_STATE_GAMEPLAY){
                if(!game_state.wave_started){
                    game_state.wave_started = true;
                }
            }
        }

        if(!game_state.input.left_down &&
           game_state.last_input.left_down){
            if(game_state.screen == GAME_SCREEN_STATE_GAMEPLAY){
                if(!game_state.wave_started){
                    game_state.unit_selection.selected_unit--;
                    Mix_PlayChannel(-1, audio::menu_move_sound, 0);
                }
            }else if(game_state.screen == GAME_SCREEN_STATE_INSTRUCTIONS){
                game_state.instruction_page--;
                game_state.instruction_page =
                    clamp<int>(game_state.instruction_page, 0, max_instruction_pages-1);
                Mix_PlayChannel(-1, audio::menu_move_sound, 0);
            }
        }

        if(!game_state.input.right_down &&
           game_state.last_input.right_down){
            if(game_state.screen == GAME_SCREEN_STATE_GAMEPLAY){
                if(!game_state.wave_started){
                    game_state.unit_selection.selected_unit++;
                    Mix_PlayChannel(-1, audio::menu_move_sound, 0);
                }
            }else if(game_state.screen == GAME_SCREEN_STATE_INSTRUCTIONS){
                game_state.instruction_page++;
                game_state.instruction_page =
                    clamp<int>(game_state.instruction_page, 0, max_instruction_pages-1);
                Mix_PlayChannel(-1, audio::menu_move_sound, 0);
            }
        }

        if(!game_state.input.tab_down &&
            game_state.last_input.tab_down){
            if(game_state.screen == GAME_SCREEN_STATE_GAMEPLAY){
                if(!game_state.wave_started){
                    game_state.show_wave_preview ^= 1;
                    Mix_PlayChannel(-1, audio::menu_select_sound, 0);
                }
            }
        }

        if(!game_state.input.up_down &&
            game_state.last_input.up_down){
            if(game_state.screen == GAME_SCREEN_STATE_MAIN_MENU){
                game_state.main_menu_option_selection_index--;
                game_state.main_menu_option_selection_index =
                    clamp<int>(game_state.main_menu_option_selection_index, 0, 2);
                Mix_PlayChannel(-1, audio::menu_move_sound, 0);
            }
        }

        if(!game_state.input.down_down &&
            game_state.last_input.down_down){
            if(game_state.screen == GAME_SCREEN_STATE_MAIN_MENU){
                game_state.main_menu_option_selection_index++;
                game_state.main_menu_option_selection_index =
                    clamp<int>(game_state.main_menu_option_selection_index, 0, 2);
                Mix_PlayChannel(-1, audio::menu_move_sound, 0);
            }
        }
    }

    static void handle_key_input(state& game_state, SDL_Event* event){
        int scancode = event->key.keysym.scancode;
        bool keydown = event->type == SDL_KEYDOWN;

        switch(scancode){
#ifdef STUPID_DEBUG
            case SDL_SCANCODE_Q:
            {
                game_state.input.q_down = keydown;
            }
            break;
            case SDL_SCANCODE_W:
            {
                game_state.input.w_down = keydown;
            }
            break;
#endif
            case SDL_SCANCODE_M:
            {
                game_state.input.m_down = keydown;
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
            case SDL_SCANCODE_UP:
            {
                game_state.input.up_down = keydown;
            }
            break;
            case SDL_SCANCODE_DOWN:
            {
                game_state.input.down_down = keydown;
            }
            break;
            case SDL_SCANCODE_LEFT:
            {
                game_state.input.left_down = keydown;
            }
            break;
            case SDL_SCANCODE_RIGHT:
            {
                game_state.input.right_down = keydown;
            }
            break;
            case SDL_SCANCODE_TAB:
            {
                game_state.input.tab_down = keydown;
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
                Mix_PlayChannel(-1, 
                        audio::pew_sounds[(audio::audio_cyclic_counter++) % audio::MAX_PEW_SOUNDS],
                        0);
                turret_projectile projectile = {};
                projectile.type = PROJECTILE_BULLET;

                if(turret->type == TURRET_FREEZER){
                    projectile.freeze_on_hit = true;
                }

                float turret_center_x = turret->x + (turret->w / 2);
                float turret_center_y = turret->y + (turret->h / 2);

                projectile.x = turret_center_x;
                projectile.y = turret_center_y;

                projectile.w = 16;
                projectile.h = 16;

                // 2 mins and 30 seconds?
                projectile.lifetime = 150;

                projectile.speed = 500;

                projectile.damage = rand() % 20 + 15;
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

        // perhaps this should grow dynamically each round?
        static constexpr int square_size = 6;
        paint_square_in_tilemap(tilemap_width / 2 - (square_size / 2),
                tilemap_height / 2 - (square_size / 2),
                square_size,
                square_size, 1);
    }

    static void distribute_reward_for_round(state& game_state){
        static int wave_rewards_for_rounds[unique_wave_count] = {
            750, 750, 750, 750, 1000, 1000, 1250, 1250, 1250, 1400, 1500, 1600, 1700, 1800, 2000
        };
        int clamped_index = clamp<int>(game_state.game_wave, 0, unique_wave_count - 1);
        game_state.points += (wave_rewards_for_rounds[clamped_index] + 1500);

        game_state.reward_flashing_started = true;
        game_state.reward_flash_fx_timer = max_reward_flash_fx_timer;
        game_state.flash_iterations = 0;
    }

    static void render_instructions(state& game_state, SDL_Renderer* renderer, float delta_time){
        static float heading_text_scale = 1.0f; 
        heading_text_scale = sinf(SDL_GetTicks() / 500.0f) + 2.5;
        render_centered_dynamically_scaled_text(
                renderer,
                FONT_SIZE_MEDIUM,
                heading_text_scale,
                1024,
                300,
                "Instructions / Help",
                gfx::yellow);

        // I don't feel like implementing word wrap....
        //
        // COPY AND PASTE HERE I COME!
        static constexpr float start_layout_x = 100;
        static constexpr float start_layout_y = 250;
        float layout_x = start_layout_x;
        float layout_y = start_layout_y;
        constexpr float layout_advance = 32;
        // no actually. WTF???????
        switch(game_state.instruction_page){
            case 0:
            {
                static constexpr size_t lines_in_page_one = 9;
                static char* page_one_lines[lines_in_page_one] = {
                    "Welcome to _Treewatch_, an ad-hoc tower defense game.",
                    "You play as a group of unseen environmentalists that",
                    "decided using turrets would be the best way to",
                    "defend the last tree on the planet.",
                    "  ",
                    "  ",
                    "The tree-haters are shown on the next page.",
                    "The UI of this game is controlled with the arrow keys.",
                    "However the main game is controlled with the mouse."
                };


                for(int i = 0; i < lines_in_page_one; ++i){
                    gfx::render_text(
                            renderer,
                            FONT_SIZE_MEDIUM,
                            layout_x, 
                            layout_y,
                            page_one_lines[i],
                            gfx::white);
                    layout_y += layout_advance;
                }
            }
            break;
            case 1:
            {
                static constexpr size_t lines_in_page_two = 9;
                static char* page_two_lines[lines_in_page_two] = {
                    "The opposition to your group of tree huggers are:",
                    " ",
                    "Lumberjacks: ",
                    " ",
                    " ",
                    "these guys inexplicably still think it's a good idea",
                    "to chop trees down. They're generally weak and in 3 sizes.",
                    "Small, Medium, and Large. Remarkably unremarkable.",
                    "No trouble for you for sure."
                };


                for(int i = 0; i < lines_in_page_two; ++i){
                    gfx::render_text(
                            renderer,
                            FONT_SIZE_MEDIUM,
                            layout_x, 
                            layout_y,
                            page_two_lines[i],
                            gfx::white);
                    layout_y += layout_advance;
                }

                gfx::render_textured_rectangle( 
                        renderer,
                        gfx::enemy_textures[ENEMY_BASIC_LUMBERJACK - 1].texture,
                        {
                        400, 
                        300,
                        gfx::enemy_textures[ENEMY_BASIC_LUMBERJACK-1].width * gfx::enemy_textures[ENEMY_BASIC_LUMBERJACK-1].at_scale,
                        gfx::enemy_textures[ENEMY_BASIC_LUMBERJACK-1].height * gfx::enemy_textures[ENEMY_BASIC_LUMBERJACK-1].at_scale
                        },
                        gfx::white
                        );

/*                 gfx::render_textured_rectangle( */ 
/*                         renderer, */
/*                         gfx::enemy_textures[4].texture, */
/*                         { */
/*                         400, */ 
/*                         350, */
/*                         gfx::enemy_textures[4].width * gfx::enemy_textures[4].at_scale / 2, */
/*                         gfx::enemy_textures[4].height * gfx::enemy_textures[4].at_scale / 2 */
/*                         }, */
/*                         gfx::white */
/*                         ); */
            }
            break;
            case 2:
            {
                static constexpr size_t lines_in_page_three = 7;
                static char* page_three_lines[lines_in_page_three] = {
                    "Logging Machines: ",
                    " ",
                    " ",
                    "Somehow these things are actually autonomous.",
                    "They decided instead of conquering humanity they would",
                    "destroy the environment to insideously destroy humans.",
                    "These are moderately tanky and tough. Thankfully uncommon."
                };


                for(int i = 0; i < lines_in_page_three; ++i){
                    gfx::render_text(
                            renderer,
                            FONT_SIZE_MEDIUM,
                            layout_x, 
                            layout_y,
                            page_three_lines[i],
                            gfx::white);
                    layout_y += layout_advance;
                }

                gfx::render_textured_rectangle( 
                        renderer,
                        gfx::enemy_textures[4].texture,
                        {
                        400, 
                        220,
                        gfx::enemy_textures[4].width * gfx::enemy_textures[4].at_scale / 2,
                        gfx::enemy_textures[4].height * gfx::enemy_textures[4].at_scale / 2
                        },
                        gfx::white
                        );
            }
            break;
            case 3:
            {
                static constexpr size_t lines_in_page_four = 10;
                static char* page_four_lines[lines_in_page_four] = {
                    "Anarchists: ",
                    " ",
                    " ",
                    "These punks want to cause chaos for no good reason.",
                    "They're the fastest of your opposition and can bolt",
                    "through your defenses, but thankfully they're fragiler",
                    "than glass.",
                    "However these punks can instantly destroy your turrets.",
                    "Somehow they know how to hack your turrets in just the",
                    "right way that things break..."
                };


                for(int i = 0; i < lines_in_page_four; ++i){
                    gfx::render_text(
                            renderer,
                            FONT_SIZE_MEDIUM,
                            layout_x, 
                            layout_y,
                            page_four_lines[i],
                            gfx::white);
                    layout_y += layout_advance;
                }

                gfx::render_textured_rectangle( 
                        renderer,
                        gfx::enemy_textures[3].texture,
                        {
                        400, 
                        220,
                        gfx::enemy_textures[3].width * gfx::enemy_textures[3].at_scale,
                        gfx::enemy_textures[3].height * gfx::enemy_textures[3].at_scale
                        },
                        gfx::white
                        );
            }
            break;
            case 4:
            {
                static constexpr size_t lines_in_page_five = 13;
                static char* page_five_lines[lines_in_page_five] = {
                    " ",
                    " ",
                    "However, you have some low-tech turrets that",
                    "can stop these baddies.",
                    " ",
                    "Singleshooter: $500. Cheap. simple. 'Nuff said.",
                    " ",
                    "Repeater: $750. Little pricier. But has a faster firerate",
                    " ",
                    "Freezer: $1500. Most expensive, but can slow down your enemies.",
                    " ",
                    "How long can you make it with just these?",
                    "Well, let's see. *END OF MANUAL*"
                };


                for(int i = 0; i < lines_in_page_five; ++i){
                    gfx::render_text(
                            renderer,
                            FONT_SIZE_MEDIUM,
                            layout_x, 
                            layout_y,
                            page_five_lines[i],
                            gfx::white);
                    layout_y += layout_advance;
                }

                for(int i = 0; i < 3; ++i){
                    gfx::render_textured_rectangle( 
                            renderer,
                            gfx::turret_cards[i],
                            {
                            350 + (i * 100) + (25 * (i - 1)), 
                            200,
                            100,
                            100
                            },
                            gfx::white
                            );
                }
            }
            break;
        }

        render_centered_text(
                renderer,
                FONT_SIZE_MEDIUM,
                1024,
                768 * 2 - 40,
                "Use arrow keys to navigate <-, ->",
                gfx::cyan
                );
    }

    static void render_gameover(state& game_state, SDL_Renderer* renderer, float delta_time){
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

    enum main_menu_option{
        OPTION_START_GAME,
        OPTION_INSTRUCTIONS,
        OPTION_QUIT,
        OPTION_COUNT
    };
    static char* option_strings[OPTION_COUNT] =
    {
        "Start Game",
        "Instructions / Help",
        "Quit Game"
    };

    static void render_mainmenu(state& game_state, SDL_Renderer* renderer, float delta_time){
        static float heading_text_scale = 1.0f; 
        heading_text_scale = sinf(SDL_GetTicks() / 500.0f) + 2.5;
        render_centered_dynamically_scaled_text(
                renderer,
                FONT_SIZE_MEDIUM,
                heading_text_scale,
                1024,
                300,
                title_string,
                gfx::yellow);

        float layout_options_y = 700;
        const float advance_y = 68;

        for(int option_index = 0;
            option_index < OPTION_COUNT;
            ++option_index){
            gfx::color draw_color = gfx::white;

            if(option_index == game_state.main_menu_option_selection_index){
                draw_color = gfx::yellow;
            }

            render_centered_text(
                    renderer,
                    FONT_SIZE_MEDIUM,
                    1024,
                    layout_options_y,
                    option_strings[option_index],
                    draw_color);

            layout_options_y += advance_y;
        }

/*         render_centered_text( */
/*                 renderer, */
/*                 FONT_SIZE_MEDIUM, */
/*                 1024, */
/*                 700, */
/*                 "Enter to Start Game", */
/*                 gfx::white); */

/*         render_centered_text( */
/*                 renderer, */
/*                 FONT_SIZE_MEDIUM, */
/*                 1024, */
/*                 768, */
/*                 "Escape to Quit Game", */
/*                 gfx::white); */

        render_centered_text(
                renderer,
                FONT_SIZE_SMALL,
                1024,
                1200,
                "Game by Xpost2000 for Ludum Dare 46, navigate with arrow keys.",
                gfx::white);
    }

    // deltatime is provided to update graphical effects...
    static void render_gameplay(state& game_state, SDL_Renderer* renderer, float delta_time){

        // draw the background / backdrop
        {
            /* gfx::render_textured_rectangle( */
            /*         renderer, */
            /*         gfx::backdrop_texture, */
            /*         {0, 0, 1024, 768}, */
            /*         gfx::white */
            /*         ); */
            for(size_t y = 0; y < tilemap_height; ++y){
                for(size_t x = 0; x < tilemap_width; ++x){
                    switch(backdrop_tilemap[y][x]){
                        case 0:
                        {
                            gfx::render_text(
                                    renderer,
                                    FONT_SIZE_MEDIUM,
                                    x * 32, 
                                    y * 32,
                                    ",",
                                    gfx::dark_gray);
                        }
                        break;
                        case 1:
                        {
                            gfx::render_text(
                                    renderer,
                                    FONT_SIZE_MEDIUM,
                                    x * 32, 
                                    y * 32,
                                    ",",
                                    gfx::green);
                        }
                        break;
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
                    gfx::color draw_color = gfx::green;
                    
                    switch(current_unit->type){
                        case TURRET_SINGLE_SHOOTER:
                        {
                            draw_color = gfx::green;
                        }
                        break;
                        case TURRET_REPEATER:
                        {
                            draw_color = gfx::red;
                        }
                        break;
                        case TURRET_FREEZER:
                        {
                            draw_color = gfx::cyan;
                        }
                        break;
                    }

                    gfx::render_textured_rectangle(
                            renderer,
                            gfx::turret_texture.texture,
                            { 
                            current_unit->x + (current_unit->w / 2) -
                            ((gfx::turret_texture.width * gfx::turret_texture.at_scale) * gfx::turret_texture.pivot_x),
                            current_unit->y + (current_unit->h / 2) -
                            ((gfx::turret_texture.height * gfx::turret_texture.at_scale) * gfx::turret_texture.pivot_y),
                            gfx::turret_texture.width * gfx::turret_texture.at_scale,
                            gfx::turret_texture.height * gfx::turret_texture.at_scale
                            },
                            draw_color);

#ifdef STUPID_DEBUG
                    gfx::render_rectangle(renderer, 
                            {
                            current_unit->x,
                            current_unit->y,
                            current_unit->w,
                            current_unit->h
                            },
                            draw_color);
#endif
                }
            }
        }

        // draw enemies
        {
            for(unsigned enemy_index = 0; enemy_index < MAX_ENEMIES_IN_GAME; ++enemy_index){
                enemy_entity* current_enemy = &game_state.enemies[enemy_index];
                if(current_enemy->type != ENEMY_NULL){
                    unsigned enemy_texture_index = (current_enemy->type - 1);

                    gfx::color draw_color = gfx::white;
                    if(current_enemy->frozen){
                        draw_color = gfx::cyan;
                    }

                    gfx::render_textured_rectangle( 
                            renderer,
                            gfx::enemy_textures[enemy_texture_index].texture,
                            {
                            (current_enemy->x + (current_enemy->w / 2)) -
                            ((gfx::enemy_textures[enemy_texture_index].width * gfx::enemy_textures[enemy_texture_index].at_scale) * gfx::enemy_textures[enemy_texture_index].pivot_x),
                            (current_enemy->y + (current_enemy->h / 2)) -
                            ((gfx::enemy_textures[enemy_texture_index].height * gfx::enemy_textures[enemy_texture_index].at_scale) * gfx::enemy_textures[enemy_texture_index].pivot_y),
                            gfx::enemy_textures[enemy_texture_index].width * gfx::enemy_textures[enemy_texture_index].at_scale,
                            gfx::enemy_textures[enemy_texture_index].height * gfx::enemy_textures[enemy_texture_index].at_scale
                            },
                            draw_color
                            );

#ifdef STUPID_DEBUG
                    gfx::render_rectangle(
                            renderer, 
                            {
                            current_enemy->x,
                            current_enemy->y,
                            current_enemy->w,
                            current_enemy->h
                            },
                            gfx::white);
#endif

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

#ifdef STUPID_DEBUG
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
#endif
                }
            }
        }

        // draw preview turret
        if(!game_state.wave_started && !game_state.show_wave_preview){
            turret_unit ghost_placement = 
                generate_turret_of_type((turret_type)(game_state.unit_selection.selected_unit+1));

            gfx::color draw_color =
            {
                0, 0, 128, 128
            };

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            // everything is still top left aligned....
            //
            // I have to actually hack in the calculations
            // because nothing is centered.... Damn.
#if 1
            gfx::color circle_color = gfx::blue;
            circle_color.a = 100;
            gfx::render_circle(renderer,
                    game_state.input.mouse_x + (ghost_placement.w / 2),
                    game_state.input.mouse_y + (ghost_placement.h / 2),
                    ghost_placement.targetting_radius,
                    circle_color);
#endif

            gfx::render_textured_rectangle(
                    renderer,
                    gfx::turret_texture.texture,
                    { 
                    game_state.input.mouse_x -
                    ((gfx::turret_texture.width * gfx::turret_texture.at_scale) * gfx::turret_texture.pivot_x),
                    game_state.input.mouse_y -
                    ((gfx::turret_texture.height * gfx::turret_texture.at_scale) * gfx::turret_texture.pivot_y),
                    gfx::turret_texture.width * gfx::turret_texture.at_scale,
                    gfx::turret_texture.height * gfx::turret_texture.at_scale
                    },
                    draw_color);


#ifdef STUPID_DEBUG
            gfx::render_rectangle(renderer, 
                    {
                    game_state.input.mouse_x - (ghost_placement.w / 2),
                    game_state.input.mouse_y - (ghost_placement.h / 2),
                    ghost_placement.w,
                    ghost_placement.h
                    },
                    draw_color);
#endif
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
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

        // draw the tree
        {
            gfx::render_textured_rectangle(
                    renderer,
                    gfx::tree_texture.texture,
                    { 
                    game_state.tree.x + (game_state.tree.w / 2) -
                    ((gfx::tree_texture.width * gfx::tree_texture.at_scale) * gfx::tree_texture.pivot_x),
                    game_state.tree.y + (game_state.tree.h / 2) -
                    ((gfx::tree_texture.height * gfx::tree_texture.at_scale) * gfx::tree_texture.pivot_y),
                    gfx::tree_texture.width * gfx::tree_texture.at_scale,
                    gfx::tree_texture.height * gfx::tree_texture.at_scale
                    },
                    gfx::white);

#ifdef STUPID_DEBUG
            gfx::render_rectangle(renderer, 
                    {game_state.tree.x,
                    game_state.tree.y,
                    game_state.tree.w,
                    game_state.tree.h},
                    gfx::white);
#endif

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

#ifdef STUPID_DEBUG
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
#endif
        }

        // Unit selection UI.
        // maybe buttons?
        // This is here and splits up the text stuf
        {
            // print the preview cards for all enemies that
            // will show up
            if(!game_state.wave_started){
                if(game_state.show_wave_preview){
                    const float ui_preview_card_box_size = 100;
                    const float ui_preview_card_wrap_width = ui_preview_card_box_size * 8;

                    float ui_preview_card_layout_y = (25);
                    float ui_preview_card_layout_x = virtual_window_width * 0.1;

                    for(unsigned wave_list_entry_index = 0;
                            wave_list_entry_index < game_state.wave_spawn_list_entry_count;
                            ++wave_list_entry_index){
                        wave_spawn_list_entry* entry = 
                            &game_state.wave_spawn_list[wave_list_entry_index];

                        size_t card_texture_index = 0;
                        switch(entry->type){
                            case ENEMY_BASIC_LUMBERJACK: { card_texture_index = 1; } break;
                            case ENEMY_SKINNY_LUMBERJACK: { card_texture_index = 1; } break;
                            case ENEMY_FAT_LUMBERJACK: { card_texture_index = 2; } break;
                            case ENEMY_ANARCHIST: { card_texture_index = 3; } break;
                            case ENEMY_LOGGING_MACHINES: { card_texture_index = 0; } break;
                                                         /* case ENEMY_ANTHROPORMORPHIC_ANT: { draw_color = gfx::color{0, 255, 255}; } break; */
                        }

                        gfx::rectangle draw_rect =
                        {
                            ui_preview_card_layout_x,
                            ui_preview_card_layout_y,
                            ui_preview_card_box_size,
                            ui_preview_card_box_size
                        };

                        ui_preview_card_layout_x += (ui_preview_card_box_size * 1.10);
                        if(ui_preview_card_layout_x >= ui_preview_card_wrap_width){
                            ui_preview_card_layout_x = virtual_window_width * 0.1;
                            ui_preview_card_layout_y += ui_preview_card_box_size * 1.05;
                        }
                        gfx::render_textured_rectangle( 
                                renderer,
                                gfx::enemy_cards[card_texture_index],
                                draw_rect,
                                gfx::white 
                                );
                    }
                }else{
                    // an update? Impossibru!
                    game_state.unit_selection.selected_unit =
                        clamp<int>(game_state.unit_selection.selected_unit, 0, TURRET_TYPE_COUNT-2);
                    const float ui_selection_box_size = 128;
                    const float ui_selection_layout_y = virtual_window_height - (ui_selection_box_size * 1.05);
                    float ui_selection_layout_x = virtual_window_width * 0.1;

                    for(unsigned unit_index = TURRET_SINGLE_SHOOTER;
                            unit_index < TURRET_TYPE_COUNT; // TURRET_NULL doesn't count
                            ++unit_index){
                        gfx::rectangle draw_rect =
                        {
                            ui_selection_layout_x,
                            ui_selection_layout_y,
                            ui_selection_box_size,
                            ui_selection_box_size
                        };

                        ui_selection_layout_x += (ui_selection_box_size * 1.10);
                        gfx::render_textured_rectangle( 
                                renderer,
                                gfx::turret_cards[unit_index-1],
                                draw_rect,
                                gfx::white
                                );

                        if((unit_index) == game_state.unit_selection.selected_unit+1){
                            gfx::render_textured_rectangle( 
                                    renderer,
                                    gfx::frame_texture,
                                    draw_rect,
                                    gfx::green
                                    );
                        }
                    }

                    // Annoying.
                    gfx::render_centered_text(renderer,
                            FONT_SIZE_MEDIUM,
                            virtual_window_width,
                            virtual_window_height,
                            "Press TAB to see the wave preview",
                            gfx::red);
                }
            }
        }

        // UI
        if(!game_state.show_wave_preview){
            const unsigned advance_y = 100;
            float text_y_layout = 100;
            {
                char on_wave_text[255];
                snprintf(on_wave_text, 255, "WAVE: %d", game_state.game_wave);
                gfx::render_centered_text(
                        renderer, 
                        FONT_SIZE_MEDIUM,
                        1024,
                        text_y_layout,
                        on_wave_text, 
                        gfx::white);
            }
            text_y_layout += advance_y;
            if(!game_state.wave_started){
                char expect_enemies_text[255];
                snprintf(expect_enemies_text, 255, "ENEMIES IN WAVE: %d", game_state.wave_spawn_list_entry_count);
                gfx::render_centered_text(
                        renderer, 
                        FONT_SIZE_MEDIUM,
                        1024,
                        text_y_layout,
                        expect_enemies_text, 
                        gfx::white);
            }else{
                char enemies_text[255];
                snprintf(enemies_text, 255, "ENEMIES LEFT: %d", game_state.enemies_in_wave);
                gfx::render_centered_text(
                        renderer, 
                        FONT_SIZE_MEDIUM,
                        1024,
                        text_y_layout,
                        enemies_text, 
                        gfx::white);
            }

            text_y_layout += advance_y;
            {
                gfx::color points_to_spend_text_color = gfx::white;
                char points_to_spend_text[255];

                if(game_state.reward_flashing_started){
                    game_state.reward_flash_fx_timer -= delta_time;

                    if(game_state.reward_flash_fx_timer <= 0.0f){
                        game_state.reward_flash_fx_timer = max_reward_flash_fx_timer;
                        game_state.flash_iterations++;
                    }

                    if(game_state.flash_iterations >= max_reward_flash_times){
                        game_state.reward_flashing_started = false;
                    }

                    if((game_state.flash_iterations % 2) == 0){
                        points_to_spend_text_color = gfx::white;
                    }else if((game_state.flash_iterations % 2) == 1){
                        points_to_spend_text_color = gfx::green;
                    }
                }else{
                    points_to_spend_text_color = gfx::white;
                }

                if(!game_state.wave_started){
                    snprintf(points_to_spend_text, 255, "$%d", game_state.points);
                    gfx::render_centered_text(
                            renderer, 
                            FONT_SIZE_BIG,
                            1024,
                            text_y_layout,
                            points_to_spend_text, 
                            points_to_spend_text_color);
                }
            }
        }

        // floating text
        // weird bug.
        {
            for(unsigned floating_message_index = 0;
                floating_message_index < MAX_FLOATING_MESSAGES_IN_GAME;
                ++floating_message_index){
                floating_message* message = 
                    &game_state.floating_messages[floating_message_index];
                if(message->lifetime > 0.0f){
                    float percent_clamp =
                        clamp<float>(message->lifetime / message->max_lifetime, 0.0f, 1.0f);

                    gfx::color draw_color = gfx::white;
                    draw_color.a = 
                        clamp<uint8_t>(255 * percent_clamp, 0, 255);
#ifdef STUPID_DEBUG
                    fprintf(stderr, "draw_color_a: %d\n", draw_color.a);
#endif
                    gfx::render_text(
                            renderer,
                            FONT_SIZE_MEDIUM,
                            message->x, 
                            message->y,
                            message->text,
                            draw_color);
                }
            }
        }
    }

    static void render(state& game_state, SDL_Renderer* renderer, float delta_time){
        switch(game_state.screen){
            case GAME_SCREEN_STATE_MAIN_MENU:
            {
                render_mainmenu(game_state, renderer, delta_time);
            }
            break;
            case GAME_SCREEN_STATE_ATTRACT_MODE:
            {
            }
            break;
            case GAME_SCREEN_STATE_GAMEPLAY:
            {
                render_gameplay(game_state, renderer, delta_time);
            }
            break;
            case GAME_SCREEN_STATE_GAME_OVER:
            {
                render_gameover(game_state, renderer, delta_time);
            }
            break;
            case GAME_SCREEN_STATE_INSTRUCTIONS:
            {
                render_instructions(game_state, renderer, delta_time);
            }
            break;
            case GAME_SCREEN_STATE_PAUSE:
            {
            }
            break;
        }

        {
            if(music_playing){
                gfx::render_textured_rectangle(
                        renderer,
                        gfx::audio_playing_texture,
                        {
                        0, 0,
                        64, 64
                        },
                        gfx::white
                        );
            }else{

                gfx::render_textured_rectangle(
                        renderer,
                        gfx::audio_mute_texture,
                        {
                        0, 0,
                        64, 64
                        },
                        gfx::white
                        );
            }
        }
    }

    static void update_instructions(state& game_state, const float delta_time){
    }

    static void update_gameover(state& game_state, const float delta_time){
    }

    static void update_mainmenu(state& game_state, const float delta_time){
        if(!Mix_PlayingMusic()){
            Mix_PlayMusic(audio::menu_music, -1);
        }
    }

    static void update_gameplay(state& game_state, const float delta_time){
        if(game_state.wave_started){
            if(game_state.wave_spawn_list_entry_count == 0 &&
               game_state.enemies_in_wave == 0){
                finished_round(game_state);
                distribute_reward_for_round(game_state);
                Mix_PlayChannel(-1, audio::wave_finished_sound, 0);
            }
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

                        enemy_entity* closest_enemy = nullptr;
                        float shortest_distance_between = INFINITY; 

                        for(unsigned enemy_index = 0;
                                enemy_index < MAX_ENEMIES_IN_GAME;
                                ++enemy_index){
                            enemy_entity* current_enemy =
                                &game_state.enemies[enemy_index];

                            if(current_enemy->type != ENEMY_NULL &&
                                    current_enemy->health > 0){
                                {
                                    float delta_x = current_enemy->x - current_turret->x;
                                    float delta_y = current_enemy->y - current_turret->y;

                                    float distance = sqrtf((delta_x * delta_x) + (delta_y * delta_y));
                                    if(distance <= shortest_distance_between){
                                        shortest_distance_between = distance;
                                        closest_enemy = current_enemy;
                                    }
                                }
                            }
                            //???
                        } 
                        //????
                        if(closest_enemy){
                            float delta_x = closest_enemy->x - current_turret->x;
                            float delta_y = closest_enemy->y - current_turret->y;
                            float distance_between_turret_and_enemy =
                                sqrtf((delta_y * delta_y) + (delta_x * delta_x));

                            if(distance_between_turret_and_enemy <= current_turret->targetting_radius){
                                // it will attempt to fire...
                                // if it can...
                                turret_fire_projectile_at_point(game_state, current_turret, 
                                        closest_enemy->x + (closest_enemy->w / 2), 
                                        closest_enemy->y + (closest_enemy->h / 2));
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

                                    current_enemy->health -= projectile->damage;
                                    Mix_PlayChannel(-1, 
                                            audio::tree_hurt_sounds[(audio::audio_cyclic_counter++) % audio::MAX_TREE_HURT_SOUNDS],
                                            0);
                                    if(projectile->freeze_on_hit){
                                        current_enemy->freeze_time = 2.5f;
                                        current_enemy->frozen = true;
                                    }

                                    break;
                                }
                            }
                        } 
                        //??
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

                    current_enemy->freeze_time -= delta_time;
                    if(current_enemy->freeze_time <= 0.0f){
                        current_enemy->frozen = false;
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
                    bool touched_turret = false;

                    current_enemy->damage_timer -= delta_time;

                    if(!touching_tree){
                        // see if I hit a turret
                        for(unsigned turret_index = 0;
                            turret_index < MAX_TURRETS_IN_GAME;
                            ++turret_index){
                            turret_unit* current_turret =
                                &game_state.turret_units[turret_index];

                            if(current_turret->health <= 0.0f){
                                if(current_turret->type != TURRET_NULL){
                                    current_turret->type = TURRET_NULL;
                                    Mix_PlayChannel(-1, 
                                            audio::explosion_sounds[(audio::audio_cyclic_counter++) % audio::MAX_EXPLOSION_SOUNDS],
                                            0);
                                    continue;
                                }
                            }

                            if(current_turret->type != TURRET_NULL){
                                aabb turret_bounding_box =
                                {
                                    current_turret->x,
                                    current_turret->y,
                                    current_turret->w,
                                    current_turret->h
                                };

                                bool touching_turret = aabb_intersects(turret_bounding_box, enemy_bounding_box);

                                if(touching_turret){
                                    touched_turret = touching_turret;
                                    if(current_enemy->damage_timer <= 0.0f){
                                        //anarchists can instantly disable turrets.
                                        if(current_enemy->type == ENEMY_ANARCHIST){
                                            current_turret->health = 0;
                                        }else{
                                            current_enemy->damage_timer = 
                                                current_enemy->damage_timer_delay;
                                            current_turret->health -= current_enemy->damage;
                                        }

                                        Mix_PlayChannel(-1, 
                                                audio::tree_hurt_sounds[(audio::audio_cyclic_counter++) % audio::MAX_TREE_HURT_SOUNDS],
                                                0);
                                    }
                                    break;
                                }
                            }
                        }
                    }else{
                        if(current_enemy->damage_timer <= 0.0f){
                            current_enemy->damage_timer = 
                                current_enemy->damage_timer_delay;
                            game_state.tree.health -= current_enemy->damage;
                            Mix_PlayChannel(-1, 
                                    audio::tree_hurt_sounds[(audio::audio_cyclic_counter++) % audio::MAX_TREE_HURT_SOUNDS],
                                    0);
                        }
                    }

                    if(!touching_tree && !touched_turret){
                        float speed = current_enemy->speed;
                        if(current_enemy->frozen){
                            speed /= 2.5f;
                        }
                        current_enemy->x += direction_to_target_x * speed * delta_time;
                        current_enemy->y += direction_to_target_y * speed * delta_time;
                    }
                }
            }
        }else{
        }

        for(unsigned floating_message_index = 0;
            floating_message_index < MAX_FLOATING_MESSAGES_IN_GAME;
            ++floating_message_index){
            floating_message* message = 
                &game_state.floating_messages[floating_message_index];
            if(message->lifetime > 0.0f){
                message->lifetime -= delta_time;
                message->y -= 25 * delta_time;
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
            case GAME_SCREEN_STATE_INSTRUCTIONS:
            {
                update_instructions(game_state, delta_time);
            }
            break;
            case GAME_SCREEN_STATE_ATTRACT_MODE:
            {
                // Pfft... What attract mode?
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
                // Used a flag instead.
            }
            break;
        }
        game_state.last_input = game_state.input;
    }
}

int main(int argc, char** argv){
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    Mix_Init(MIX_INIT_OGG);
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048);
    TTF_Init();

    srand(time(nullptr));

    SDL_Window* window =
        SDL_CreateWindow(game_name, 
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                window_width,
                window_height,
                SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
                );
    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    uint32_t current = SDL_GetTicks();
    uint32_t last = current;
    uint32_t difference = 0;

    float delta_time = 1/60.0f;

    game::state state = {};

    game::initialize(state, renderer);

    audio::wave_finished_sound =
        Mix_LoadWAV("data/sound/wave_finished.wav");

    audio::menu_move_sound =
        Mix_LoadWAV("data/sound/menu_move.wav");

    audio::menu_select_sound =
        Mix_LoadWAV("data/sound/menu_select.wav");

    gfx::tree_texture =
        gfx::load_image_to_texture_with_origin_info(renderer, "data/evergreen.png");
    gfx::tree_texture.pivot_x = 0.5;
    gfx::tree_texture.pivot_y = 1.0;

    gfx::circle_texture = 
        gfx::load_image_to_texture(renderer, "data/circle.png");

    gfx::backdrop_texture =
        gfx::load_image_to_texture(renderer, "data/emergency-art/backdrop.png");

    gfx::turret_texture = 
        gfx::load_image_to_texture_with_origin_info(renderer, "data/turret.png");
    gfx::turret_texture.at_scale = 0.3f;
    gfx::turret_texture.pivot_x = 0.5f;
    gfx::turret_texture.pivot_y = 0.5f;

    gfx::frame_texture =
        gfx::load_image_to_texture(renderer, "data/frame.png");

    gfx::enemy_cards[0] =
        gfx::load_image_to_texture(renderer, "data/logging_machine_card.png");
    gfx::enemy_cards[1] =
        gfx::load_image_to_texture(renderer, "data/lumberjack_card.png");
    gfx::enemy_cards[2] =
        gfx::load_image_to_texture(renderer, "data/fat_lumberjack_card.png");
    gfx::enemy_cards[3] =
        gfx::load_image_to_texture(renderer, "data/anarchist_card.png");

    gfx::turret_cards[0] =
        gfx::load_image_to_texture(renderer, "data/singleshooter_icon.png");
    gfx::turret_cards[1] =
        gfx::load_image_to_texture(renderer, "data/repeater_icon.png");
    gfx::turret_cards[2] =
        gfx::load_image_to_texture(renderer, "data/freezer_icon.png");

    gfx::enemy_textures[0] =
        gfx::load_image_to_texture_with_origin_info(renderer, "data/lumberjack.png");
    gfx::enemy_textures[0].at_scale = 0.35f;
    gfx::enemy_textures[0].pivot_x = 0.5f;
    gfx::enemy_textures[0].pivot_y = 0.95f;

    gfx::enemy_textures[1] =
        gfx::load_image_to_texture_with_origin_info(renderer, "data/lumberjack.png");
    gfx::enemy_textures[1].at_scale = 0.25f;
    gfx::enemy_textures[1].pivot_x = 0.5f;
    gfx::enemy_textures[1].pivot_y = 0.5f;

    gfx::enemy_textures[2] =
        gfx::load_image_to_texture_with_origin_info(renderer, "data/lumberjack.png");
    gfx::enemy_textures[2].at_scale = 0.60f;
    gfx::enemy_textures[2].pivot_x = 0.5f;
    gfx::enemy_textures[2].pivot_y = 0.5f;

    gfx::enemy_textures[3] =
        gfx::load_image_to_texture_with_origin_info(renderer, "data/anarchist.png");
    gfx::enemy_textures[3].at_scale = 0.35f;
    gfx::enemy_textures[3].pivot_x = 0.5f;
    gfx::enemy_textures[3].pivot_y = 0.7f;

    gfx::enemy_textures[4] =
        gfx::load_image_to_texture_with_origin_info(renderer, "data/logging_machine.png");
    gfx::enemy_textures[4].at_scale = 0.80f;
    gfx::enemy_textures[4].pivot_x = 0.4f;
    gfx::enemy_textures[4].pivot_y = 0.5f;

    {
        gfx::audio_playing_texture = 
            gfx::load_image_to_texture(renderer, "data/audio_playing.png");
        gfx::audio_mute_texture = 
            gfx::load_image_to_texture(renderer, "data/audio_mute.png");
    }

    // initialize all audio...
    // wtf... wtf. 
    {
        using namespace audio;
        Mix_Volume(-1, MIX_MAX_VOLUME / 4);
        {
            menu_music =
                Mix_LoadMUS("data/sound/menuloop.ogg");
        }  
        {
            explosion_sounds[0] =
                Mix_LoadWAV("data/sound/explosion_a.wav");
            explosion_sounds[1] =
                Mix_LoadWAV("data/sound/explosion_b.wav");
            explosion_sounds[2] =
                Mix_LoadWAV("data/sound/explosion_c.wav");
            explosion_sounds[3] =
                Mix_LoadWAV("data/sound/explosion_d.wav");
        }
        {
            logging_move_sounds[0] =
                Mix_LoadWAV("data/sound/logging_machine_move.wav");
            logging_move_sounds[1] =
                Mix_LoadWAV("data/sound/logging_machine_move_b.wav");
        }
        {
            pew_sounds[0] =
                Mix_LoadWAV("data/sound/pew_a.wav");
            pew_sounds[1] =
                Mix_LoadWAV("data/sound/pew_b.wav");
            pew_sounds[2] =
                Mix_LoadWAV("data/sound/pew_c.wav");
            pew_sounds[3] =
                Mix_LoadWAV("data/sound/pew_d.wav");
            pew_sounds[4] =
                Mix_LoadWAV("data/sound/pew_e.wav");
        }
        {
            tree_hurt_sounds[0] =
                Mix_LoadWAV("data/sound/tree_hurt_a.wav");
            tree_hurt_sounds[1] =
                Mix_LoadWAV("data/sound/tree_hurt_b.wav");
            tree_hurt_sounds[2] =
                Mix_LoadWAV("data/sound/tree_hurt_c.wav");
            tree_hurt_sounds[3] =
                Mix_LoadWAV("data/sound/tree_hurt_d.wav");
            tree_hurt_sounds[4] =
                Mix_LoadWAV("data/sound/tree_hurt_e.wav");
        }
    }
     
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

        game::render(state, renderer, delta_time);

        SDL_RenderPresent(renderer);

        // probably really dumb
        current = SDL_GetTicks();
        difference = current - last;
        last = current;

        // last thing I'll do lmao
        delta_time = (difference / 1000.0f);
        /* delta_time = 1.0f / 60.0f; */
    }

    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
