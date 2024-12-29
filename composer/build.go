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

package main

import (
	"fmt"
	"os"
	"path/filepath"

	"gopkg.in/yaml.v3"
)

type Level struct {
	GLTF   []string
	Script []string
}

type Manifest struct {
	Name    string
	GameID  string
	Version uint8
	Levels  map[string]Level
}

func readManifest(name string) (manifest *Manifest, err error) {
	_manifest, err := os.ReadFile(name)
	if err != nil {
		return nil, fmt.Errorf(`failed to open manifest (%w)`, err)
	}

	manifest = new(Manifest)
	err = yaml.Unmarshal(_manifest, &manifest)
	if err != nil {
		return nil, fmt.Errorf(`failed to unmarshal manifest (%w)`, err)
	}
	return
}

func makeTempDir(pattern string) (tempDir string, err error) {
	tempDir, err = os.MkdirTemp("", pattern)
	if err != nil {
		// could not use default temp dir, try project directory instead
		pwd, _ := os.Getwd()
		tempDir, err = os.MkdirTemp(pwd, pattern)
		if err != nil {
			return "", fmt.Errorf(`failed to create temporary working directory (%w)`, err)
		}
	}
	return
}

type BuildCmd struct {
	Apploader string `arg:"" help:"Path to a retail-compatible apploader (e.g. ORCA freeloader)" existingfile:""`
	Runtime   string `arg:"dol" help:"Path to the ORCA runtime executable" existingfile:""`
	Dir       string `arg:"project" help:"Path to the project root directory (defaults to the current directory)" optional:"" existingdir:""`
}

func (r *BuildCmd) Run() error {
	manifest, err := readManifest(filepath.Join(r.Dir, "manifest.yaml"))
	if err != nil {
		return err
	}

	id := GameID{}
	if len(manifest.GameID) != 6 {
		return fmt.Errorf(`GameID was %d chars, expected 6`, len(manifest.GameID))
	}
	id.ConsoleID = manifest.GameID[0]
	id.Gamecode[0] = manifest.GameID[1]
	id.Gamecode[1] = manifest.GameID[2]
	id.CountryCode = manifest.GameID[3]
	id.MakerCode[0] = manifest.GameID[4]
	id.MakerCode[1] = manifest.GameID[5]
	id.Version = manifest.Version
	if len(manifest.Name) > 63 {
		return fmt.Errorf(`game name exceeded 63 chars`)
	}
	for i := range len(manifest.Name) {
		id.GameName[i] = manifest.Name[i]
	}

	tempDir, err := makeTempDir("ORCA.*")
	if err != nil {
		return err
	}

	for id, level := range manifest.Levels {
		buf, err := PackLevel(level, r.Dir)
		if err != nil {
			return err
		}

		filename := id + ".PAK"
		err = os.WriteFile(filepath.Join(tempDir, filename), buf, os.ModeAppend)
		if err != nil {
			return fmt.Errorf(`failed to write packed level file "%s" (%w)`, filename, err)
		}
	}

	gcm, err := GCM(id, r.Apploader, r.Runtime, tempDir)
	if err != nil {
		return fmt.Errorf(`failed to pack GCM image (%w)`, err)
	}

	f, err := os.Create(filepath.Join(r.Dir, "game.gcm"))
	if err != nil {
		return fmt.Errorf(`failed to create GCM file (%w)`, err)
	}
	_, err = f.Write(gcm)
	if err != nil {
		return fmt.Errorf(`failed to write packed GCM (%w)`, err)
	}

	return nil
}
