#include <cstdint>
#include <chrono>
#include <thread>
#include <string>
#include <cstdio>
#include <Windows.h>

#include <memory/memory.h>
#include <sdk/offsets.h>
#include <sdk/sdk.h>
#include <game/game.h>
#include <cache/cache.h>
#include <render/render.h>
#include <render/notifications.h>
#include <features/silent/silent.h>
#include <features/expl/walkspeed.h>
#include <features/expl/freezeplayer.h>
#include <features/expl/hitbox_expander.h>
#include <features/expl/fly.h>
#include <features/aimbot/aimbot.h>
#include <check/typing_check.h>
#include <game/rescan.h>
#include <auth/keyauth_init.h>
#include <bypass/kill_crash_handler.h>
#include "../protection/protection/antidebug.h"

std::int32_t main()
{
	std::thread(debugger_detection).detach();
	std::thread(rbx::bypass::run).detach();
	SetConsoleCP(65001);
	SetConsoleOutputCP(65001);
	
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX cfi = { sizeof(cfi) };
	GetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
	wcscpy_s(cfi.FaceName, L"NSimSun");
	SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);

	//initialize_keyauth();
	//
	//if (!authenticate_keyauth())
	//{
	//	std::this_thread::sleep_for(std::chrono::seconds(5));
	//	cleanup_keyauth();
	//	return 1;
	//}

	static const char* BINARY_NAME = "RobloxPlayerBeta.exe";

	if (!memory->find_process_id(BINARY_NAME))
	{
		printf("\n");
		print_colored_bot_message("", false);
		printf("Unable to find Roblox!\n");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		ExitProcess(1);
		return 1;
	}

	if (!memory->attach_to_process(BINARY_NAME))
	{
		printf("\n");
		print_colored_bot_message("", false);
		printf("Unable to attach to Roblox!\n");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		ExitProcess(1);
		return 1;
	}

	if (!memory->find_module_address(BINARY_NAME))
	{
		printf("\n");
		print_colored_bot_message("", false);
		printf("Unable to find main module address!\n");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		ExitProcess(1);
		return 1;
	}

	char buffer[256];
	
	sprintf_s(buffer, "base -> 0x%llx", memory->get_module_address());
	print_colored_bot_message(buffer, true);

	std::uint64_t fake_datamodel{ memory->read<std::uint64_t>(memory->get_module_address() + Offsets::FakeDataModel::Pointer) };
	game::datamodel = rbx::instance_t(memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel));
	
	sprintf_s(buffer, "datamodel -> 0x%llx", game::datamodel.address);
	print_colored_bot_message(buffer, true);

	game::visengine = { memory->read<std::uint64_t>(memory->get_module_address() + Offsets::VisualEngine::Pointer) };
	
	sprintf_s(buffer, "visengine -> 0x%llx", game::visengine.address);
	print_colored_bot_message(buffer, true);

	game::players = { game::datamodel.find_first_child_by_class("Players") };
	
	sprintf_s(buffer, "players -> 0x%llx", game::players.address);
	print_colored_bot_message(buffer, true);

	game::local_player = { memory->read<std::uint64_t>(game::players.address + Offsets::Player::LocalPlayer) };
	
	sprintf_s(buffer, "local_player -> 0x%llx", game::local_player.address);
	print_colored_bot_message(buffer, true);

	rbx::player_t local_player_obj = { game::local_player.address };
	game::local_character = { local_player_obj.get_model_instance().address };
	
	sprintf_s(buffer, "local_character -> 0x%llx", game::local_character.address);
	print_colored_bot_message(buffer, true);

	std::thread(cache::run).detach();

	if (!InitializeStorage())
	{
		printf("\n");
		print_colored_bot_message("", false);
		printf("Failed to initialize storage\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	std::thread(AutoRescanHandler).detach();

	rbx::silent::initialize();

	if (!render->create_window())
	{
		printf("\n");
		print_colored_bot_message("", false);
		printf("Failed to create window\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	if (!render->create_device())
	{
		printf("\n");
		print_colored_bot_message("", false);
		printf("Failed to create device\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}
	
	if (!render->create_imgui())
	{
		printf("\n");
		print_colored_bot_message("", false);
		printf("Failed to create imgui\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}


	notifications::add("Rose Loaded Successfully!", notifications::NotificationType::Success, 8.0f);

	std::thread([]() {
		for (;;)
		{
			static const char* BINARY_NAME = "RobloxPlayerBeta.exe";
			if (!memory->find_process_id(BINARY_NAME))
			{
				ExitProcess(0);
			}

			Sleep(500);
		}
		}).detach();

	std::thread(check::run).detach();
	std::thread(walkspeed::run).detach();
	std::thread(freezeplayer::run).detach();
	std::thread(fly::run).detach();
	std::thread(rbx::aimbot::run).detach();
	std::thread(rbx::hitbox_expander::run).detach();

	std::thread([&]()
		{
			for (;;)
			{
				Sleep(200);

				if (!settings::expl::tickrate)
				{
					continue;
				}

				auto gc = memory->read<uintptr_t>(game::workspace.address + Offsets::Workspace::GravityContainer);
				memory->write<float>(gc + 0x660, settings::expl::tickrate_amount * 4);
			}
		}
	).detach();

	game::wnd = FindWindowA(nullptr, "Roblox");

	while (true)
	{
		render->start_render();

		render->render_visuals();

		if (render->running)
		{
			render->render_menu();
		}

		render->render_notifications();

		render->end_render();
	}

	cleanup_keyauth();
	return 0;
}