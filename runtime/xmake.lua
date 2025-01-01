--[[
/*
ORCA
Copyright (C) 2024 leonardus

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

local devkitpro_dir = os.getenv("DEVKITPRO")
if not devkitpro_dir then
	raise("Please set DEVKITPRO in your environment.")
end

local devkitppc_bindir = os.getenv("DEVKITPPC_BINDIR")
if not devkitppc_bindir then
	raise("Please set DEVKITPPC_BINDIR in your environment.")
end

toolchain("devkitppc")
	set_kind("standalone")

	set_bindir(devkitppc_bindir)

	for k, v in pairs({
		cc = "powerpc-eabi-gcc",
		cxx = "powerpc-eabi-g++",
		ld = "powerpc-eabi-gcc",
		ar = "powerpc-eabi-ar",
		strip = "powerpc-eabi-strip",
		as = "powerpc-eabi-as"
	}) do
		set_toolset(k, v)
	end
toolchain_end()

set_allowedplats("dolphin")
set_allowedarchs("ppc")
set_defaultplat("dolphin")
set_defaultarchs("ppc")

set_languages("c11")
set_optimize("faster")

target("dol")
	set_default(true)

	set_toolchains("devkitppc")
	set_plat("dolphin")
	set_arch("ppc")
	local march = "-mogc -mcpu=750 -meabi -mhard-float"
	add_cxflags(march)
	add_asflags(march)
	add_ldflags(march)
	add_linkdirs(devkitpro_dir .. "/libogc2/lib/cube")
	add_sysincludedirs(devkitpro_dir .. "/libogc2/include")
	add_syslinks("ogc", "m")
	add_defines("GEKKO", "DOLPHIN", "OGC")
	add_defines("DEBUG")

	set_kind("binary")
	set_basename("ORCA")
	set_extension(".ELF")

	add_includedirs("include")
	add_files("src/*.c")

	after_build(function(target)
		os.cd(target:targetdir())
		os.run("elf2dol %s %s", target:filename(), target:basename() .. ".DOL")
	end)
target_end()

--[[
This target should not be used directly, but is necessary to
describe clang-compabile build commands for generation of
compile_commands.json (for clangd) with `xmake project -k compile_commands`
]]
target("_do_not_use")
	set_default(false)

	set_toolchains("clang")
	set_kind("binary")
	add_cflags("-nostdlibinc")
	add_cflags("-target ppc-unknown-unknown-eabi")
	add_sysincludedirs(devkitpro_dir .. "/devkitPPC/powerpc-eabi/include")
	add_sysincludedirs(devkitpro_dir .. "/libogc2/include")
	add_includedirs("include")
	add_defines("GEKKO", "DOLPHIN", "OGC")
	add_defines("DEBUG")
	add_files("src/*.c")
target_end()
