/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2013  Ivanov Viktor

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

#include "video.h"

#include "../types.h"

#ifdef _WIN32
#include "win32-res/win32-res.h"
#endif

namespace vpnes_gui {

/* CVideo */

CVideo::CVideo(CWindow *Window) {
	pWindow = Window;
	InternalSurface = NULL;
	Pal = NULL;
	UpdatePalette();
#if defined(VPNES_USE_TTF)
	Font = NULL;
	if (TTF_Init() < 0)
		throw CGenericException("TTF initialization failure");
#ifdef _WIN32
	void *FontData = NULL;
	SDL_RWops *FontRW = NULL;

	/* Используем ресурсы для получения шрифта */
	ResourceInfo = FindResource(pWindow->GetInstance(), MAKEINTRESOURCE(IDR_MAINFONT),
		RT_RCDATA);
	ResourceHandle = LoadResource(pWindow->GetInstance(), ResourceInfo);
	FontData = LockResource(ResourceHandle);
	FontRW = SDL_RWFromConstMem(FontData, SizeofResource(pWindow->GetInstance(), ResourceInfo));
	if (FontRW != NULL)
		Font = TTF_OpenFontRW(FontRW, -1, 22);
#else
#ifndef VPNES_TTF_PATH
#define VPNES_TTF_PATH "text.otf"
#endif
	Font = TTF_OpenFont(VPNES_TTF_PATH, 22);
#endif
	if (Font == NULL)
		throw CGenericException("Couldn't open a font file");
	TextSurface = NULL;
#if defined(VPNES_DISABLE_SYNC)
	TextTimer = SDL_GetTicks();
#endif
#endif
}

CVideo::~CVideo() {
#if defined(VPNES_USE_TTF)
	if (Font != NULL)
		TTF_CloseFont(Font);
#ifdef _WIN32
	UnlockResource(ResourceHandle);
	FreeResource(ResourceHandle);
#endif
	if (TextSurface != NULL)
		SDL_FreeSurface(TextSurface);
#endif
	if (InternalSurface != NULL)
		SDL_FreeSurface(InternalSurface);
	delete [] Pal;
}

/* Обновить палитру */
void CVideo::UpdatePalette() {
	const Uint8 NES_Palette[64][3] = {
		{124, 124, 124}, {0,   0,   252}, {0,   0,   188}, {68,  40,  188}, {148, 0,   132},
		{168, 0,  32  }, {168, 16,  0  }, {136, 20,  0  }, {80,  48,  0  }, {0,   120, 0  },
		{0,   104, 0  }, {0,   88,  0  }, {0,   64,  88 }, {0,   0,   0  }, {0,   0,   0  },
		{0,   0,   0  }, {188, 188, 188}, {0,   120, 248}, {0,   88,  248}, {104, 68,  252},
		{216, 0,   204}, {228, 0,   88 }, {248, 56,  0  }, {228, 92,  16 }, {172, 124, 0  },
		{0,   184, 0  }, {0,   168, 0  }, {0,   168, 68 }, {0,   136, 136}, {0,   0,   0  },
		{0,   0,   0  }, {0,   0,   0  }, {248, 248, 248}, {60,  188, 252}, {104, 136, 252},
		{152, 120, 248}, {248, 120, 248}, {248, 88,  152}, {248, 120, 88 }, {252, 160, 68 },
		{248, 184, 0  }, {184, 248, 24 }, {88,  216, 84 }, {88,  248, 152}, {0,   232, 216},
		{120, 120, 120}, {0,   0,   0  }, {0,   0,   0  }, {252, 252, 252}, {164, 228, 252},
		{184, 184, 248}, {216, 184, 248}, {248, 184, 248}, {248, 164, 192}, {240, 208, 176},
		{252, 224, 168}, {248, 216, 120}, {216, 248, 120}, {184, 248, 184}, {184, 248, 216},
		{0,   252, 252}, {216, 216, 216}, {0,   0,   0  }, {0,   0,   0  }
	};
	int i, j;

	delete [] Pal;
	Pal = new Uint32[64 * 8];
#define C_R(comp) ((comp) & 1)
#define C_G(comp) (((comp) & 2) >> 1)
#define C_B(comp) (((comp) & 4) >> 2)
	for (i = 0; i < 64; i++)
		for (j = 0; j < 8; j++)
			Pal[i + 64 * j] = SDL_MapRGB(pWindow->GetSurface()->format,
				(int) (NES_Palette[i][0] * (6 - C_G(j) - C_B(j)) / 6.0),
				(int) (NES_Palette[i][1] * (6 - C_R(j) - C_B(j)) / 6.0),
				(int) (NES_Palette[i][2] * (6 - C_R(j) - C_G(j)) / 6.0));
#undef C_R
#undef C_G
#undef C_B
}

/* Обновить поверхность */
SDL_Surface *CVideo::UpdateSurface() {
	SDL_Surface *NewSurface, *Screen;

	Screen = pWindow->GetSurface();
	NewSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, _Width, _Height, 32,
		Screen->format->Rmask, Screen->format->Gmask, Screen->format->Bmask,
		Screen->format->Amask);
	return NewSurface;
}

/* Обновить размер */
void CVideo::UpdateSizes(int Width, int Height) {
#if defined(VPNES_USE_TTF)
	if (TextSurface != NULL)
		SDL_FreeSurface(TextSurface);
#endif
	if (InternalSurface != NULL)
		SDL_FreeSurface(InternalSurface);
	_Width = Width;
	_Height = Height;
	InternalSurface = UpdateSurface();
	Buf = (uint32 *) InternalSurface->pixels;
}

/* Обновить кадр */
bool CVideo::UpdateFrame(double FrameTime) {
	SDL_Surface *Screen;

#if !defined(VPNES_DISABLE_SYNC)
	Delta += FrameTime;
#endif
	while (pWindow->ProcessMessages() == CWindow::wsUpdateBuffer) {
		SDL_Surface *NewSurface;

		/* Обновляем параметры экрана, если это необходимо */
		UpdatePalette();
		NewSurface = SDL_ConvertSurface(InternalSurface, pWindow->GetSurface()->format,
			SDL_SWSURFACE);
		SDL_FreeSurface(InternalSurface);
		InternalSurface = NewSurface;
		Buf = (uint32 *) InternalSurface->pixels;
	}
	Screen = pWindow->GetSurface();
#if defined(VPNES_USE_TTF)
#if defined(VPNES_DISABLE_SYNC)
	if ((TextSurface != NULL) && ((SDL_GetTicks() - TextTimer) >= 4000)) {
		SDL_FreeSurface(TextSurface);
		TextSurface = NULL;
	}
#endif
	if (pWindow->GetUpdateTextFlag()) {
		SDL_Surface *TempSurface;
		SDL_Rect TextInRect = {7, 7};
		SDL_Rect BorderRect = {1, 1};
		const SDL_Color TextColour = {224, 224, 224};
		const SDL_Color BorderColour = {56, 56, 56};
		const SDL_Color BGColour = {40, 40, 40};

		if (TextSurface != NULL)
			SDL_FreeSurface(TextSurface);
		TempSurface = TTF_RenderUTF8_Shaded(Font, pWindow->GetWindowText(), TextColour,
			BGColour);
		TextSurface = SDL_CreateRGBSurface(SDL_HWSURFACE,
			TempSurface->w + TextInRect.x * 2,
			TempSurface->h + TextInRect.y * 2,
			Screen->format->BitsPerPixel, Screen->format->Rmask,
			Screen->format->Gmask, Screen->format->Bmask, Screen->format->Amask);
		SDL_FillRect(TextSurface, NULL, SDL_MapRGB(TextSurface->format,
			BorderColour.r, BorderColour.g, BorderColour.b));
		BorderRect.w = TextSurface->w - BorderRect.x * 2;
		BorderRect.h = TextSurface->h - BorderRect.y * 2;
		SDL_FillRect(TextSurface, &BorderRect, SDL_MapRGB(TextSurface->format,
			BGColour.r, BGColour.g, BGColour.b));
		SDL_BlitSurface(TempSurface, NULL, TextSurface, &TextInRect);
		SDL_FreeSurface(TempSurface);
		pWindow->GetUpdateTextFlag() = false;
#if defined(VPNES_DISABLE_SYNC)
		TextTimer = SDL_GetTicks();
#endif
	}
#endif
#if !defined(VPNES_DISABLE_SYNC) && !defined(VPNES_DISABLE_FSKIP)
	if (!SkipFrame) {
#endif
	/* Обновляем экран */
	if (SDL_MUSTLOCK(Screen))
		SDL_LockSurface(Screen);
	SDL_SoftStretch(InternalSurface, NULL, Screen, NULL);
#if defined(VPNES_USE_TTF)
	SDL_Rect TextRect = {10, 10};

	if (TextSurface != NULL)
		SDL_BlitSurface(TextSurface, NULL, Screen, &TextRect);
#endif
	if (SDL_MUSTLOCK(Screen))
		SDL_UnlockSurface(Screen);
	SDL_Flip(Screen);
#if !defined(VPNES_DISABLE_SYNC) && !defined(VPNES_DISABLE_FSKIP)
	}
#endif
	return pWindow->GetWindowState() != CWindow::wsNormal;
}

#if !defined(VPNES_DISABLE_SYNC)

/* Остановить синхронизацию */
void CVideo::SyncStop() {
	StopTime = SDL_GetTicks();
}

/* Продолжить синхронизацию */
Uint32 CVideo::SyncResume() {
	Uint32 TimeDiff;

	SyncReset();
	TimeDiff = FrameStart - StopTime;
#if defined(VPNES_USE_TTF)
	TextTimer += TimeDiff;
#endif
	return TimeDiff;
}

/* Сбросить синхронизацию */
void CVideo::SyncReset() {
	FrameStart = SDL_GetTicks();
	FrameTimeCheck = FrameStart;
	Jitter = 0;
#if !defined(VPNES_DISABLE_FSKIP)
	SkipFrame = false;
#endif
}

/* Синхронизировать время */
void CVideo::Sync(double PlayRate) {
	Uint32 DelayTime, FrameTime;

	/* Синхронизация */
	FrameTime = (Uint32) (Delta / PlayRate);
	DelayTime = FrameTime - (SDL_GetTicks() - FrameStart);
	Delta -= (Uint32) Delta;
#if !defined(VPNES_DISABLE_FSKIP)
	if (!SkipFrame) {
#endif
	if (Jitter < DelayTime)
		SDL_Delay(DelayTime - Jitter);
#if !defined(VPNES_DISABLE_FSKIP)
	}
#endif
	FrameStart = SDL_GetTicks();
	Jitter += FrameStart - FrameTimeCheck - FrameTime;
	FrameTimeCheck = FrameStart;
#if !defined(VPNES_DISABLE_FSKIP)
	if (SkipFrame) {
		SkippedTime += FrameStart - FrameTimeCheck;
		if (SkippedTime > 50) {
			Jitter = 0;
			/* log */
		}
		FramesSkipped++;
		if (Jitter < FrameTime) {
			SkipFrame = false;
			/* log */
		}
	} else if (Jitter > FrameTime) {
		SkipFrame = true;
		SkippedTime = 0;
		FramesSkipped = 0;
		/* log */
	}
#endif
#if defined(VPNES_USE_TTF)
	if ((TextSurface != NULL) && ((FrameStart - TextTimer) >= 4000)) {
		SDL_FreeSurface(TextSurface);
		TextSurface = NULL;
	}
	if (pWindow->GetUpdateTextFlag()) {
		TextTimer = FrameStart;
	}
#endif
}

#endif

}