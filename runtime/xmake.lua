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

target("runtime")
	add_rules("ORCAEnv")
	set_plat("dolphin")
	set_arch("ppc")
	set_toolchains("devkitppc")
	set_warnings("allextra")

	add_cxflags("-mogc")
	add_asflags("-mogc")
	add_ldflags("-mogc")
	add_linkdirs(path.join("$(env DEVKITPRO)", "libogc-orca", "lib", "cube"))
	add_sysincludedirs(path.join("$(env DEVKITPRO)", "libogc-orca", "include"))
	add_syslinks("ogc")
	add_defines("OGC")

	set_kind("binary")
	set_basename("ORCA")
	set_extension(".ELF")

	set_languages("c11")
	set_optimize("faster")

	add_includedirs("include")
	add_files("src/*.c")

	if is_mode("debug") then
		add_defines("DEBUG")
		set_symbols("debug")
		set_optimize("none")
		add_ldflags("-Wl,-Map=ORCA.MAP")
	end

	after_build(function(target)
		os.run("%s %s %s", 
		       path.join("$(env DEVKITPRO)", "tools", "bin", "elf2dol"),
		       target:targetfile(),
			   path.join(target:targetdir(), target:basename() .. ".DOL")
		)
		
		os.rm(target:targetfile())
	end)

	on_install(function(target)
		os.cp(path.join(target:targetdir(), target:basename() .. ".DOL"), "$(env ORCA)")
	end)

	on_clean(function(target)
		os.rm("ORCA.MAP")
	end)
target_end()
