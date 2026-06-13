#include "freezeplayer.h"

#include <memory/memory.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <game/game.h>
#include <settings.h>
#include <check/typing_check.h>
#include <Windows.h>
#include <thread>
#include <chrono>
#include <iostream>

namespace freezeplayer
{
	static bool playerfreeze_thread_running = true;
	static bool waSEN2 = false;

	void freezeplayer_loop()
	{
		while (playerfreeze_thread_running)
		{
			bool key_pressed = false;
			static bool key_was_pressed = false;
			static bool toggle_state = false;
			static bool was_disabled_by_typing = false;

			if (check::textchatopen)
			{
				was_disabled_by_typing = true;
				toggle_state = false;
				if (waSEN2)
				{
					//memory->write<int>(memory->get_module_address() + Offsets::FFlags::TargetTimeDelayFacctorTenths, 20);
					waSEN2 = false;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}

			if (was_disabled_by_typing && !check::textchatopen)
			{
				toggle_state = false;
				was_disabled_by_typing = false;
			}

			if (settings::expl::freeze_players_keybind_mode == 0)
			{
				key_pressed = GetAsyncKeyState(settings::expl::freeze_players_keybind) & 0x8000;
			}
			else if (settings::expl::freeze_players_keybind_mode == 1)
			{
				bool current_key_state = GetAsyncKeyState(settings::expl::freeze_players_keybind) & 0x8000;
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
			else if (settings::expl::freeze_players_keybind_mode == 2)
			{
				key_pressed = true;
			}

			if (settings::expl::freeze_players && key_pressed)
			{
				waSEN2 = true;
				std::cout << "sdfsdfsdf" << std::endl;
				//memory->write<int>(memory->get_module_address() + Offsets::FFlags::TargetTimeDelayFacctorTenths, -999999999999999999);
			}
			else
			{
				if (waSEN2)
				{
				//	memory->write<int>(memory->get_module_address() + Offsets::FFlags::TargetTimeDelayFacctorTenths, 20);
					waSEN2 = false;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	void run()
	{
		playerfreeze_thread_running = true;
		freezeplayer_loop();
	}
}

