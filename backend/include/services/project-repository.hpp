#pragma once

#include <string>
#include <vector>

#include "model/project.hpp"

/**
 * Stateless utilities for discovering projects on disk.
 *
 * These functions contain no mutable state and do not require a manager
 * instance. They are intended to be used by list/scan operations that do not
 * need a fully loaded project (audio, songs, setlists).
 */
namespace ProjectRepository {

/** Returns the default projects directory: ~/daw/projects/ */
std::string getDefaultProjectsDirectory();

/**
 * Scans a directory for .dawproj folders and returns a lightweight metadata
 * snapshot for each one found.
 *
 * Each returned Project is created via Project::readMetadata(), meaning its
 * SongsManager and SetlistManager are NOT initialised. Only identity fields
 * (id, name, path, lastModified) are populated.
 */
std::vector<Project> scan(const std::string& directory);

}  // namespace ProjectRepository
