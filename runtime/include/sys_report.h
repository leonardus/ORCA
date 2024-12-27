/*-------------------------------------------------------------

system.h -- OS functions and initialization

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

Copyright (C) 2024 leonardus

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.


-------------------------------------------------------------*/

/* This is an altered version of the original system.h. */

/*
ORCA
Copyright (C) 2024 leonardus

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdbool.h>

/* \fn void SYS_STDIO_Report(bool use_stdout)
\brief redirect stderr to Dolphin OSReport uart
\param[in] use_stdout also redirect stdout for use of printf

*/
void SYS_STDIO_Report(bool use_stdout);

/*! \fn void SYS_Report (char const *const fmt_, ...)
\brief write formatted string to Dolphin OSReport uart

*/

void SYS_Report (char* fmt_, ...);
