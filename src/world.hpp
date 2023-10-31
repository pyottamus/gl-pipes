// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <algorithm>
#include <optional>
#include  <numeric>

#ifndef unreachable
#ifdef __GNUC__ // GCC, Clang, ICC

#define unreachable() (__builtin_unreachable())

#else
#ifdef _MSC_VER // MSVC

#define unreachable() (__assume(false))

#else
#error "No deffinition for unreachable"
#endif
#endif
#endif
enum class Direction {
    North,
    South,
    East,
    West,
    Up,
    Down
};
enum class PipeUpdataType {
    NOP,
    PIPE_STRAIGHT,
    PIPE_BEND,
    FIRST_PIPE,
    NEW
};
struct PipeStraightData {
    glm::uvec3 last_node;
    glm::uvec3 current_node;

    Direction current_dir;

    size_t pipe_id;
};
struct PipeBendData {
    glm::uvec3 last_node;
    glm::uvec3 current_node;

    Direction last_dir;
    Direction current_dir;

    size_t pipe_id;
};


struct NewPipeData {
    glm::uvec3 start_node;
    size_t pipe_id;
};
struct PipeUpdateData {
    PipeUpdataType type;
    union {
        PipeStraightData pipeStraightData;
        PipeBendData pipeBendData;
        NewPipeData newPipeData;
    } data;
};


struct Ocupied {

    int x, y, z;
    std::vector<bool> ocupied_nodes;
    std::vector<uint64_t> free_map0;
    size_t used = 0;

    Ocupied(int x, int y, int z) : x{ x }, y{ y }, z{ z }, ocupied_nodes(x* y* z, false), free_map0( ( 63 + x * y * z) / 64, UINT64_MAX) {
        size_t leftover = (free_map0.size() * 64) - ocupied_nodes.size();
        if (leftover) {
            // mask out upper leftover bits of free_map0[-1]
            uint64_t& last = *(free_map0.end() - 1);
            uint8_t keep_bits = (64 - leftover);
            last &= ((UINT64_C(1) << keep_bits) - 1);
        }
    
    }

    bool operator[](size_t i) {
        return ocupied_nodes[i];
    }
    glm::u64vec3 iTovec(size_t i) {
        size_t z_c = i / x * y;
        i %= x * y;
        size_t y_c = i / x;
        i %= x;
        size_t x_c = i;
        return { x_c, y_c, z_c };
    }
    size_t vecToi(glm::u64vec3 vec) {
        return vec.z * (x * y) + vec.y * x + vec.x;
    }
    bool operator[](glm::u64vec3 i) {
        return ocupied_nodes[vecToi(i)];
    }


    void set(uint64_t i) {
        ocupied_nodes[i] = true;
        free_map0[i / 64] &= ~(uint64_t)(UINT64_C(1) << (i % 64));
        used += 1;
    }

    void set(glm::u64vec3 i) {
        set(vecToi(i));
    }


    std::optional<glm::u64vec3> getRandomFree(auto& rng) {
        std::vector<size_t> choices(free_map0.size());
        std::iota(choices.begin(), choices.end(), 0);
        std::shuffle(choices.begin(), choices.end(), rng);
        
        for (size_t i = 0; i < choices.size(); ++i) {
            if (choices[i] == 0)
                continue;
            uint64_t choice = choices[i];
            uint8_t bitChoice[64];
            std::iota(bitChoice, bitChoice + 64, 0);
            std::shuffle(bitChoice, bitChoice + 64, rng);

            for (uint8_t j = 0; j < 64; ++j) {
                if (choice & (UINT64_C(1) << (bitChoice[j]))) {
                    size_t index = i * 64 + j;
                    set(index);
                    return iTovec(index);
                }
            }
        }

        return {};
        
    }


};
/// Returns a random coordinate that is not occupied by any other pipe (including the pipe itself if
/// somehow the pipe is already on the board)
glm::u64vec3 get_random_start(Ocupied& occupied_nodes, auto& rng) {
    //Double check if somehow there is no more space on the board
    auto coord = occupied_nodes.getRandomFree(rng);
    if (!coord) {
        throw std::runtime_error("No more space on the board!");
    }

    return *coord;
}

Direction choose_random_direction(auto& rng) {
    return (Direction)std::uniform_int_distribution<>{0, 5}(rng);
}


glm::uvec3 step_in_dir(glm::uvec3 coord, Direction dir) {
    
    switch (dir) {
    case Direction::North:
        coord.z -= 1;
        break;
    case Direction::South:
        coord.z += 1;
        break;
    case Direction::East:
        coord.x += 1;
        break;
    case Direction::West:
        coord.x -= 1;
        break;
    case Direction::Up:
        coord.y += 1;
        break;
    case Direction::Down:
        coord.y -= 1;
        break;
    default:
        unreachable();
    }

    return coord;
}

bool is_in_bounds(glm::uvec3 coord, glm::uvec3 bounds) {
    return coord.x < bounds.x && coord.y < bounds.y && coord.z < bounds.z;
}

struct Pipe {
    bool alive;
    glm::uvec3 space_bounds;

    std::vector<glm::uvec3> nodes;
    Direction current_dir;

    size_t len() {
        return nodes.size();
    }

    Pipe(glm::uvec3 space_bounds, Ocupied& ocupied_nodes, auto& rng) : space_bounds{ space_bounds } {
        nodes.emplace_back(get_random_start(ocupied_nodes, rng));
    }

    Direction get_current_dir() {
        return current_dir;
    }
    glm::uvec3 get_current_head() {
        return nodes.back();
    }

    void kill(){}
    void update(Ocupied& ocupied_nodes, auto& rng) {
        if(!alive)
            return;
        
        uint8_t directions_to_try[6] = { 0, 1, 2, 3, 4, 5 };
        bool want_to_turn = std::uniform_int_distribution<>(0, 1)(rng);
        

        if (nodes.size() > 1 && !want_to_turn) {
            std::swap(directions_to_try[(uint8_t)current_dir], directions_to_try[0]);
            std::shuffle(directions_to_try + 1, directions_to_try + 6, rng);
        }
        else {
            std::shuffle(directions_to_try, directions_to_try + 6, rng);

        }

        bool found_valid_direction = false;
        glm::uvec3 new_position{ 0, 0, 0 };
        for(auto dir : directions_to_try){
            new_position = step_in_dir(nodes.back(), (Direction)dir);
            if(!is_in_bounds(new_position, space_bounds)){
                continue;
            }
            if(ocupied_nodes[new_position]) {
                continue;
            }
            found_valid_direction = true;
            current_dir = (Direction)dir;
            break;
        }
        if(!found_valid_direction){
            kill();
            return;
        }

        ocupied_nodes.set(new_position);

        nodes.emplace_back(new_position);
        
    }
};




struct World {
    std::default_random_engine rng;
    std::uniform_int_distribution<> directions{ 0, 5 };
    std::uniform_int_distribution<> xdir;
    std::uniform_int_distribution<> ydir;
    std::uniform_int_distribution<> zdir;
    glm::uvec3 bounds;
    Ocupied ocupied_nodes;
    double new_pipe_chance = .1L;
    int active_pipes = 0;
    bool gen_complete;
    std::vector<glm::vec3> colors;
    std::vector<Pipe> pipes;
    uint8_t max_pipes;
    World(int x_max, int y_max, int z_max, uint8_t max_pipes) :rng(std::random_device{}()), xdir(0, x_max), ydir(0, y_max), zdir(0, z_max), ocupied_nodes{ x_max, y_max, z_max }, max_pipes{ max_pipes }, bounds{ x_max, y_max, z_max } {
        std::uniform_real_distribution<float> color_rng{ 0., 1. };
        for (int i = 0; i < max_pipes; ++i) {
            colors.emplace_back(color_rng(rng), color_rng(rng) ,color_rng(rng));
        }
    }

    size_t pipe_count() {
        return pipes.size();
    }

    bool is_pipe_alive(size_t i) {
        return pipes[i].alive;
    }
    bool chance(double odds) {
        double flip = std::uniform_real_distribution<double>(0., 1.)(rng);
        return odds < flip;
    }
    void new_pipe(PipeUpdateData& data) {
        Pipe& pipe = pipes.emplace_back(bounds, ocupied_nodes, rng);
        //pipe.current_dir;
        size_t pipe_id = pipes.size() - 1;
        active_pipes += 1;

        data.type = PipeUpdataType::NEW;
        data.data.newPipeData = { .start_node = pipe.get_current_head(), .pipe_id = pipe_id
        };

    }
    bool is_gen_complete() {
        return active_pipes == 0;
    }
    void pipe_update(PipeUpdateData& data, size_t pipe_id) {
        size_t color_id = pipe_id;
        glm::uvec3 last_node = pipes[pipe_id].get_current_head();
        Direction last_dir = pipes[pipe_id].get_current_dir();
        pipes[pipe_id].update(ocupied_nodes, rng);

        glm::uvec3 current_node = pipes[pipe_id].get_current_head();
        Direction current_dir = pipes[pipe_id].get_current_dir();
        bool first_pipe = pipes[pipe_id].nodes.size() == 1;
        //Add a random chance post update to kill the pipe
        //increases the more the space is filled
        size_t total_nodes = ocupied_nodes.x * ocupied_nodes.y * ocupied_nodes.z;
        double chance_to_kill = ((double)(ocupied_nodes.used)) / ((double)(total_nodes));

        if (pipes[pipe_id].len() >= (total_nodes * 10 / 100) && chance(chance_to_kill))
        {
            pipes[pipe_id].kill();
            active_pipes -= 1;
        }

        if (first_pipe || current_dir == last_dir) {
            data.type = PipeUpdataType::PIPE_STRAIGHT;
            data.data.pipeStraightData = { .last_node = last_node, .current_node = current_node, .current_dir = current_dir, .pipe_id = pipe_id };
        }
        else {
            data.type = PipeUpdataType::PIPE_BEND;
            data.data.pipeBendData = { .last_node = last_node, .current_node = current_node, .last_dir = last_dir, .current_dir = current_dir, .pipe_id = pipe_id };
        }
        
    
    }
};

class WorldRenderer {
    
};