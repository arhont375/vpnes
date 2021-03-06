/**
 * @file
 *
 * Defines interfaces between gui and core
 */
/*
 NES Emulator
 Copyright (C) 2012-2018  Ivanov Viktor

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

 */

#ifndef INCLUDE_VPNES_CORE_FRONTEND_HPP_
#define INCLUDE_VPNES_CORE_FRONTEND_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vpnes/vpnes.hpp>

namespace vpnes {

namespace core {

/**
 * Front-end
 */
class CFrontEnd {
public:
	/**
	 * Default constructor
	 */
	CFrontEnd() = default;
	/**
	 * Deleted copy constructor
	 *
	 * @param s Copied value
	 */
	CFrontEnd(const CFrontEnd &s) = delete;
	/**
	 * Default destructor
	 */
	virtual ~CFrontEnd() = default;

	/**
	 * Frame-ready callback
	 *
	 * @param frameTime Frame time
	 */
	virtual void handleFrameRender(double frameTime) = 0;
};

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_FRONTEND_HPP_
