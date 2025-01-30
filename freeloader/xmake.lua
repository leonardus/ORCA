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

option("PatchIPL")
	set_default(false)
	set_description("Skip IPL animation")
	add_defines("PATCH_IPL")
option_end()

target("freeloader")
	add_rules("ORCAEnv")
	set_plat("dolphin")
	set_arch("ppc")
	set_toolchains("devkitppc")
	set_warnings("allextra")

	add_options("PatchIPL")

	add_cflags("-fno-builtin")
	add_cflags("-fno-toplevel-reorder")

	add_ldflags("-nostartfiles", {force=true})
	add_ldflags("-nodefaultlibs", {force=true})
	add_ldflags("-Wl,-Ttext=81200000", {force=true})

	set_kind("binary")
	set_basename("FREELOADER")
	set_extension(".ELF")

	add_includedirs("include")
	add_files("src/*.c")
	add_files("src/*.S")
	add_files("apploader.ld")

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
		       path.join("$(env DEVKITPRO)", "devkitPPC", "bin", "powerpc-eabi-objcopy"),
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
		os.cp(path.join(target:targetdir(), target:basename() .. ".IMG"), "$(env ORCA)")
	end)
target_end()
