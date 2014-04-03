/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

\****************************************************************************/

#include "gui.h"

#include "../types.h"

#include "../nes/ines.h"
#include "../nes/nes.h"
#include "../nes/neslib.h"

#include <exception>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

namespace vpnes_gui {

/* CNESGUI */

CNESGUI::CNESGUI(const char *FileName) {
	if (::SDL_Init(SDL_INIT_EVERYTHING) < 0)
		throw CGenericException("SDL initilaization failure");
	Audio = new CAudio();
	Input = new CInput();
	Window = new CWindow(FileName, Audio, Input);
	Video = new CVideo(Window);
#if !defined(VPNES_DISABLE_SYNC)
	Window->AppendSyncManager(Video);
#endif
	AudioFrontend = Audio;
	VideoFrontend = Video;
	Input1Frontend = Input->GetInput1Frontend();
	Input2Frontend = Input->GetInput2Frontend();
}

CNESGUI::~CNESGUI() {
	delete Video;
	delete Window;
	delete Input;
	delete Audio;
	::SDL_Quit();
}

/* Запустить NES */
void CNESGUI::Start(bool ForceDendyMode) {
	vpnes::ines::NES_ROM_Data Data;
	vpnes::CNESConfig *NESConfig;
	vpnes::CBasicNES *NES;
	std::ifstream ROM;
	std::fstream State;
	std::basic_string<char> Name;
	std::basic_string<char> InfoString;
	const char *RomName;
	bool Quit;
	int Flag;
	int SaveState = 0;
#if defined(VPNES_USE_TTF)
	bool FirstRun;
	const char *RomTitle;
	const char *TempStr;
#endif

#if defined(VPNES_INTERACTIVE)
	do {
#endif
		NESConfig = NULL;
		NES = NULL;
		memset(&Data, 0, sizeof(vpnes::ines::NES_ROM_Data));
		try {
			Window->ClearWindow();
			/* Открываем файл */
			RomName = Window->GetFileName();
#if defined(VPNES_INTERACTIVE)
			if (RomName == NULL)
				break;
#endif
			ROM.open(RomName, std::ios_base::in | std::ios_base::binary);
			if (ROM.fail())
				throw CGenericException("Couldn't open ROM file");
			NESConfig = vpnes::OpenROM(ROM, &Data, ForceDendyMode ? vpnes::ines::NES_Dendy :
				vpnes::ines::NES_Auto);
			ROM.close();
			if (NESConfig == NULL)
				throw CGenericException("Couldn't find any compatible NES configuration "
					"(unsupported file)");
			std::clog << "NES config: " << NESConfig->GetName() << std::endl;
			Window->UpdateSizes(NESConfig->GetWidth(), NESConfig->GetHeight());
			Video->UpdateSizes(NESConfig->GetWidth(), NESConfig->GetHeight());
			Audio->UpdateDevice(NESConfig->GetFrameLength());
#if defined(VPNES_USE_TTF)
			FirstRun = true;
#endif
#if defined(VPNES_INTERACTIVE)
			Window->GetInfoText() = InfoString.c_str();
#endif
			do {
				Audio->Reset();
				NES = NESConfig->GetNES(this);
				if (Data.Header.BatterySize > 0) {
					Name = RomName;
					Name += ".vpram";
					State.open(Name.c_str(), std::ios_base::in | std::ios_base::binary);
					if (!State.fail()) {
						NES->LoadButteryBackedMemory(State);
						State.close();
					}
				}
#if defined(VPNES_USE_TTF)
				if (FirstRun) {
					FirstRun = false;
					RomTitle = RomName;
					for (TempStr = RomName; *TempStr; TempStr++)
						if (*TempStr == '/' || *TempStr == '\\')
							RomTitle = TempStr + 1;
					Window->SetText(RomTitle);
				} else
					Window->SetText("Hard reset");
#endif
				NES->Reset();
				Quit = false;
				do {
					NES->PowerOn();
					switch (Window->GetWindowState()) {
						case CWindow::wsQuit:
						case CWindow::wsHardReset:
							Quit = true;
							break;
						case CWindow::wsSoftReset:
#if defined(VPNES_USE_TTF)
							Window->SetText("Soft reset");
#endif
							NES->Reset();
							break;
						case CWindow::wsSaveState:
							Name = RomName;
							Name += '.';
							Name += '0' + SaveState;
							State.open(Name.c_str(), std::ios_base::out |
								std::ios_base::binary | std::ios_base::trunc);
							if (!State.fail()) {
								NES->SaveState(State);
								State.close();
							}
#if defined(VPNES_USE_TTF)
							if (State.fail())
								Window->SetText("Save state error");
							else
								Window->SetText("Save state");
#endif
							break;
						case CWindow::wsLoadState:
							Name = RomName;
							Name += '.';
							Name += '0' + SaveState;
							State.open(Name.c_str(), std::ios_base::in |
								std::ios_base::binary);
							if (!State.fail()) {
								Flag = NES->LoadState(State);
								State.close();
							}
#if defined(VPNES_USE_TTF)
							if (State.fail() || Flag)
								Window->SetText("Load state error");
							else
								Window->SetText("Load state");
#endif
							break;
						default:
							break;
					}
				} while (!Quit);
				if (Data.Header.BatterySize > 0) {
					Name = RomName;
					Name += ".vpram";
					State.open(Name.c_str(), std::ios_base::out |
						std::ios_base::binary | std::ios_base::trunc);
					if (!State.fail()) {
						NES->SaveButteryBackedMemory(State);
						State.close();
					}
				}
				delete NES;
				NES = NULL;
			} while (Window->GetWindowState() == CWindow::wsHardReset);
		} catch (CGenericException &e) {
			Window->PrintErrorMsg(e.what());
			delete NES;
		}
		delete NESConfig;
		vpnes::FreeROMData(&Data);
#if defined(VPNES_INTERACTIVE)
	} while (Window->CanOpenFile());
#endif
}

/* Отладочная информация ЦПУ */
void CNESGUI::CPUDebug(uint16 PC, uint8 A, uint8 X, uint8 Y, uint8 S, uint8 P) {
	std::clog << "PC: " << std::setw(6) << std::setfill('0') << PC << std::endl;
	std::clog << "A: " << std::setw(4) << std::setfill('0') << (int) A << std::endl;
	std::clog << "X: " << std::setw(4) << std::setfill('0') << (int) X << std::endl;
	std::clog << "Y: " << std::setw(4) << std::setfill('0') << (int) Y << std::endl;
	std::clog << "S: " << std::setw(4) << std::setfill('0') << (int) S << std::endl;
	std::clog << "P: " << std::setw(4) << std::setfill('0') << (int) P << std::endl;
}

/* Отладочная информация ГПУ */
void CNESGUI::PPUDebug(int Frame, int Scanline, int Cycle, uint16 Reg1, uint16 Reg2,
	uint16 Reg1Buf) {
}

/* Критическая ошибка */
void CNESGUI::Panic() {
#if defined(VPNES_USE_TTF)
	Window->SetText("NES CPU Panic");
#endif
	std::clog << "Fatal error: CPU Panic" << std::endl;
}

}
