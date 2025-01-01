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
		as = "powerpc-eabi-gcc"
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

target("img")
	set_default(true)

	set_toolchains("devkitppc")
	set_plat("dolphin")
	set_arch("ppc")

	add_cflags("-fno-builtin")
	add_cflags("-fno-toplevel-reorder")

	add_ldflags("-nostartfiles", {force=true})
	add_ldflags("-nodefaultlibs", {force=true})
	add_ldflags("-Wl,-Ttext=81200000")
	add_ldflags("-Wl,-T,apploader.ld")

	set_kind("binary")
	set_basename("FREELOADER")
	set_extension(".ELF")

	add_includedirs("include")
	add_files("src/*.c")
	add_files("src/*.S")

	after_build(function(target)
		local function makeAplHeader(raw)
			local header = {}
			table.insert(header, string.rep("\0", 16))
			table.insert(header, string.pack(">I4", 0x81200000))
			table.insert(header, string.pack(">I4", os.filesize(raw)))
			table.insert(header, string.rep("\0", 8))
			return table.concat(header)
		end

		local raw = path.join(os.tmpdir(), target:basename() .. ".BIN")
		os.run(path.join(devkitppc_bindir, "powerpc-eabi-objcopy") .. " --strip-all -O binary %s %s",
			   path.join(target:targetdir(), target:filename()),
			   raw
		)

		local apploader = io.open(path.join(target:targetdir(), target:basename() .. ".IMG"), "wb")
		apploader:write(makeAplHeader(raw))
		local f = io.open(raw, "rb")
		apploader:write(f:read("*a"))
		f:close()
		apploader:close()
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
	set_arch("ppc")
	set_kind("binary")
	add_cflags("-target ppc-unknown-unknown-eabi")
	add_sysincludedirs(devkitpro_dir .. "/devkitPPC/powerpc-eabi/include")
	add_includedirs("include")
	add_files("src/*.c")
	add_files("src/*.S")
target_end()
