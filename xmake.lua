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

local devkitpro_dir = os.getenv("DEVKITPRO")
local devkitppc_bindir = path.join(devkitpro_dir, "devkitPPC", "bin")

toolchain("devkitppc")
	set_kind("standalone")

	on_check(function(toolchain)
		return devkitpro_dir
	end)

	set_bindir(devkitppc_bindir)
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
	add_syslinks("m")
	add_defines("GEKKO")
toolchain_end()

option("PatchIPL")
	set_default(false)
	set_description("Skip IPL animation")
	add_defines("PATCH_IPL")
option_end()

target("freeloader")
	set_plat("dolphin")
	set_arch("ppc")
	set_toolchains("devkitppc")
	set_warnings("allextra")

	add_options("PatchIPL")

	add_cflags("-fno-builtin")
	add_cflags("-fno-toplevel-reorder")

	add_ldflags("-nostartfiles", {force=true})
	add_ldflags("-nodefaultlibs", {force=true})
	add_ldflags("-Wl,-Ttext=81200000")

	set_kind("binary")
	set_basename("FREELOADER")
	set_extension(".ELF")

	add_includedirs("freeloader/include")
	add_files("freeloader/src/*.c")
	add_files("freeloader/src/*.S")
	add_files("freeloader/apploader.ld")

	after_build(function(target)
		local function makeAplHeader(bin)
			local header = {}
			table.insert(header, string.rep("\0", 16))
			table.insert(header, string.pack(">I4", 0x81200000))
			table.insert(header, string.pack(">I4", os.filesize(bin)))
			table.insert(header, string.rep("\0", 8))
			return table.concat(header)
		end

		local function readBinaryFile(path)
			local f = io.open(path, "rb")
			local contents = f:read("*a")
			f:close()
			return contents
		end

		local bin = path.join(os.tmpdir(), target:basename() .. ".BIN")
		os.run("%s --strip-all -O binary %s %s",
		       path.join(devkitppc_bindir, "powerpc-eabi-objcopy"),
			   target:targetfile(),
			   bin
		)

		local img = io.open(path.join(target:targetdir(), target:basename()) .. ".IMG", "wb")
		img:write(makeAplHeader(bin))
		img:write(readBinaryFile(bin))
		img:close()

		os.rm(target:targetfile())
	end)

	on_install(function(target)
		if os.getenv("ORCA") then
			os.cp(path.join(target:targetdir(), target:basename()) .. ".IMG", os.getenv("ORCA"))
		else
			raise "please set ORCA in your environment."
		end
	end)
target_end()

target("runtime")
	set_plat("dolphin")
	set_arch("ppc")
	set_toolchains("devkitppc")
	set_warnings("allextra")

	add_cxflags("-mogc")
	add_asflags("-mogc")
	add_ldflags("-mogc")
	add_linkdirs(path.join(devkitpro_dir, "libogc2", "lib", "cube"))
	add_sysincludedirs(path.join(devkitpro_dir, "libogc2", "include"))
	add_syslinks("ogc")
	add_defines("OGC")

	set_kind("binary")
	set_basename("ORCA")
	set_extension(".ELF")

	set_languages("c11")
	set_optimize("faster")

	add_includedirs("runtime/include")
	add_files("runtime/src/*.c")

	if is_mode("debug") then
		add_defines("DEBUG")
	end

	after_build(function(target)
		os.run("%s %s %s", 
		       path.join(devkitpro_dir, "tools", "bin", "elf2dol"),
		       target:targetfile(),
			   path.join(target:targetdir(), target:basename() .. ".DOL")
		)
		
		os.rm(target:targetfile())
	end)

	on_install(function(target)
		if os.getenv("ORCA") then
			os.cp(path.join(target:targetdir(), target:basename() .. ".DOL"), os.getenv("ORCA"))
		else
			raise "please set ORCA in your environment."
		end
	end)
target_end()
