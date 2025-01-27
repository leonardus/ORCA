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

package main

import (
	"fmt"
	"os"
	"path/filepath"

	"gopkg.in/yaml.v3"
)

type Level struct {
	GLTF   map[string]string
	Script map[string]string
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
	Apploader *string `help:"Path to a retail-compatible apploader (e.g. ORCA freeloader)" optional:"" existingfile:""`
	Runtime   *string `help:"Path to the ORCA runtime executable" optional:"" existingfile:""`
	Dir       *string `help:"Path to the project root directory (defaults to the current directory)" optional:"" existingdir:""`
}

func getOrcaComponent(name string) (string, error) {
	orcaDir, exists := os.LookupEnv("ORCA")
	if !exists {
		return "", fmt.Errorf(`ORCA is not installed`)
	}
	component := filepath.Join(orcaDir, name)
	_, err := os.Stat(component)
	if err != nil {
		return "", fmt.Errorf(`failed to locate ORCA component "%s" (%w)`, name, err)
	}
	return component, nil
}

func (r *BuildCmd) Run() error {
	var loader string
	var dol string
	var projectDir string

	var err error
	if r.Apploader == nil {
		loader, err = getOrcaComponent("FREELOADER.IMG")
	} else {
		loader = *r.Apploader
	}
	if r.Runtime == nil {
		dol, err = getOrcaComponent("ORCA.DOL")
	} else {
		dol = *r.Runtime
	}
	if r.Dir == nil {
		projectDir, err = os.Getwd()
	} else {
		projectDir = *r.Dir
	}
	if err != nil {
		return err
	}

	manifest, err := readManifest(filepath.Join(projectDir, "manifest.yaml"))
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
		if (len(id)) > 63 {
			return fmt.Errorf(`level name exceeded maximum of 63 characters`)
		}

		buf, err := PackLevel(level, projectDir)
		if err != nil {
			return err
		}

		filename := id + ".PAK"
		err = os.WriteFile(filepath.Join(tempDir, filename), buf, os.ModeAppend)
		if err != nil {
			return fmt.Errorf(`failed to write packed level file "%s" (%w)`, filename, err)
		}
	}

	gcm, err := GCM(id, loader, dol, tempDir)
	if err != nil {
		return fmt.Errorf(`failed to pack GCM image (%w)`, err)
	}

	f, err := os.Create(filepath.Join(projectDir, "game.gcm"))
	if err != nil {
		return fmt.Errorf(`failed to create GCM file (%w)`, err)
	}
	_, err = f.Write(gcm)
	if err != nil {
		return fmt.Errorf(`failed to write packed GCM (%w)`, err)
	}

	return nil
}
