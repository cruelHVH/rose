#include "fly.h"

#include <memory/memory.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <game/game.h>
#include <settings.h>
#include <check/typing_check.h>
#include <windows.h>
#include <cmath>


static math::vector3 normalize(const math::vector3& vec)
{
    float length = std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    return (length != 0) ? math::vector3{ vec.x / length, vec.y / length, vec.z / length } : vec;
}

static float length(const math::vector3& vec)
{
    return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

void fly_function(const math::cframe& cframe, const math::vector3& velocity)
{
    if (!game::local_player.address)
        return;

    rbx::player_t local_player_obj = { game::local_player.address };
    rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
    if (model_instance.address == 0)
        return;

    rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
    if (hrp.address == 0)
        return;

    rbx::part_t part(hrp.address);
    rbx::primitive_t prim = part.get_primitive();
    if (prim.address == 0)
        return;

    memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, cframe.position);
    memory->write<math::matrix3>(prim.address + Offsets::Primitive::Rotation, cframe.rotation);
    memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, velocity);
}

namespace fly
{
    void run()
    {
        static bool key_was_pressed = false;
        static bool toggle_state = false;
        static bool was_disabled_by_typing = false;
        
        for (;;)
        {
            Sleep(1);
            
            if (check::textchatopen)
            {
                was_disabled_by_typing = true;
                toggle_state = false;
                continue;
            }

            if (was_disabled_by_typing && !check::textchatopen)
            {
                toggle_state = false;
                was_disabled_by_typing = false;
            }

            if (!settings::expl::fly_enabled)
            {
                toggle_state = false;
                continue;
            }

            bool key_pressed = false;
            if (settings::expl::fly_keybind == 0)
            {
                key_pressed = true;
            }
            else if (settings::expl::fly_keybind_mode == 0)
            {
                key_pressed = GetAsyncKeyState(settings::expl::fly_keybind) & 0x8000;
            }
            else if (settings::expl::fly_keybind_mode == 1)
            {
                bool current_key_state = GetAsyncKeyState(settings::expl::fly_keybind) & 0x8000;
                if (current_key_state && !key_was_pressed)
                {
                    toggle_state = !toggle_state;
                    key_was_pressed = true;
                }
                else if (!current_key_state)
                {
                    key_was_pressed = false;
                }
                key_pressed = toggle_state;
            }
            else if (settings::expl::fly_keybind_mode == 2)
            {
                key_pressed = true;
            }

            if (!key_pressed)
                continue;

            if (game::local_player.address == 0)
                continue;

            rbx::player_t local_player_obj = { game::local_player.address };
            rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
            if (model_instance.address == 0)
                continue;

            rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
            if (hrp.address == 0)
                continue;

            rbx::part_t part(hrp.address);
            rbx::primitive_t prim = part.get_primitive();
            if (prim.address == 0)
                continue;

            math::vector3 current_position = prim.get_position();
            math::matrix3 current_rotation = prim.get_rotation();

            rbx::instance_t camera_instance = game::workspace.find_first_child_by_class("Camera");
            if (camera_instance.address == 0)
                continue;

            rbx::camera_t camera{ camera_instance.address };
            math::matrix3 camera_rotation = camera.get_rotation();

            math::vector3 move_direction(0.0f, 0.0f, 0.0f);
            bool is_moving = false;

            if (GetAsyncKeyState('W') & 0x8000)
            {
                move_direction.z -= 1.0f;
                is_moving = true;
            }
            if (GetAsyncKeyState('S') & 0x8000)
            {
                move_direction.z += 1.0f;
                is_moving = true;
            }
            if (GetAsyncKeyState('A') & 0x8000)
            {
                move_direction.x -= 1.0f;
                is_moving = true;
            }
            if (GetAsyncKeyState('D') & 0x8000)
            {
                move_direction.x += 1.0f;
                is_moving = true;
            }
            if (GetAsyncKeyState(VK_SPACE) & 0x8000)
            {
                move_direction.y += 1.0f;
                is_moving = true;
            }
            if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
            {
                move_direction.y -= 1.0f;
                is_moving = true;
            }

            if (length(move_direction) > 0.0f)
            {
                move_direction = normalize(move_direction);
                math::vector3 rotated_direction = camera_rotation * move_direction;
                
                if (settings::expl::fly_mode == 0)
                {
                    math::vector3 new_velocity = rotated_direction * settings::expl::fly_speed;
                    for (int i = 0; i < 10000; i++)
                    {
                        memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, new_velocity);
                    }
                }
                else if (settings::expl::fly_mode == 1)
                {
                    math::vector3 new_position = current_position + (rotated_direction * (settings::expl::fly_speed / 165.0f));
                    math::cframe new_cframe(new_position, current_rotation);
                    for (int i = 0; i < 10000; i++)
                    {
                        memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, new_cframe.position);
                        memory->write<math::matrix3>(prim.address + Offsets::Primitive::Rotation, new_cframe.rotation);
                        memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, math::vector3(0.0f, 0.0f, 0.0f));
                    }
                }
            }
            else
            {
                if (settings::expl::fly_mode == 0)
                {
                    math::vector3 zero_velocity(0.0f, 0.0f, 0.0f);
                    for (int i = 0; i < 10000; i++)
                    {
                        memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, zero_velocity);
                    }
                }
                else if (settings::expl::fly_mode == 1)
                {
                    math::cframe static_cframe(current_position, current_rotation);
                    for (int i = 0; i < 10000; i++)
                    {
                        memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, static_cframe.position);
                        memory->write<math::matrix3>(prim.address + Offsets::Primitive::Rotation, static_cframe.rotation);
                        memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, math::vector3(0.0f, 0.0f, 0.0f));
                    }
                }
            }
        }
    }
}

