--[[
/*
ORCA
Copyright (C) 2024,2025 leonardus

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
]]

set_project("ORCA")
set_defaultmode("debug")
set_allowedplats("dolphin")
set_allowedarchs("ppc")
set_defaultplat("dolphin")
set_defaultarchs("ppc")

toolchain("devkitppc")
	set_kind("standalone")

	on_check(function(toolchain)
		return os.getenv("DEVKITPPC")
	end)

	set_bindir(path.join("$(env DEVKITPRO)", "devkitPPC", "bin"))
	for k, v in pairs({
		cc = "powerpc-eabi-gcc",
		cxx = "powerpc-eabi-g++",
		ld = "powerpc-eabi-gcc",
		ar = "powerpc-eabi-ar",
		strip = "powerpc-eabi-strip",
		as = "powerpc-eabi-gcc"
	}) do
		set_toolset(k, v)
	end

	local march = "-mcpu=750 -meabi -mhard-float"
	add_cxflags(march)
	add_asflags(march)
	add_ldflags(march)
	add_sysincludedirs(path.join(devkitpro_dir, "devkitPPC", "powerpc-eabi", "include")) -- needed for clangd compile commands
	add_syslinks("m")
	add_defines("GEKKO")
toolchain_end()

rule("ORCAEnv")
	on_load(function(target)
		if os.getenv("ORCA") == nil then raise "Please set ORCA in your environment." end
	end)
rule_end()

includes("runtime/xmake.lua")
includes("freeloader/xmake.lua")
