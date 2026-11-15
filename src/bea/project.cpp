#include <iostream>
#include <format>
#include <filesystem>
#include <fstream>
#include <tomlc17.h>
#include <reproc/run.h>
#include <bea/core.hpp>
#include <bea/hash.hpp>
#include <bea/lexer.hpp>
#include <bea/parser.hpp>
#include <bea/optimizer/postparsesimplifier.hpp>
#include <bea/analyzer/globalresolver.hpp>
#include <bea/analyzer/localresolver.hpp>
#include <bea/project.hpp>

namespace bea {

	std::string Project::s_defaultURL = "";
	std::string Project::s_globalPath;

	std::vector<std::unique_ptr<Project>> Project::s_projects;

	/* Get strings for enums */
	const char *Project::typeToString(Project::Type type) {

		switch (type) {
			case Type::Executable: return "Executable";
			case Type::Library: return "Library";
		}
		return nullptr;
	}

	const char *Project::targetToString(Project::Target target) {

		switch (target) {
			case Target::Debug: return "debug";
			case Target::Release: return "release";
		}
		return nullptr;
	}

	/* Get project */
	Result<Project *> Project::getProject(const std::string& name) {

		for (std::unique_ptr<Project>& project: s_projects) {

			if (project->getName() == name)
				return Result<Project *>(project.get());
		}
		return Result<Project *>("Unknown project");
	}

	/* Load project */
	Result<Project *> Project::loadProject(
		const std::string& path,
		const std::string& url,
		Target target
	) {
		if (s_projects.empty())
			s_globalPath = path;

		s_projects.emplace_back(std::make_unique<Project>(path, target));
		Project& project = *s_projects[s_projects.size()-1];

		if (!std::filesystem::exists(path)) {

			if (!url.empty()) BEA_OK(Project *, project.fetch());

			else return Result<Project *>(std::format(
				"Invalid project path '{}'", path
			));
		}
		BEA_OK(Project *, project.loadConfiguration());
		BEA_OK(Project *, project.locateSources("src"));

		/* Sort sources */
		std::sort(
			project.m_sources.begin(),
			project.m_sources.end(),
			[](auto& a, auto& b){ return a->shortPath < b->shortPath; }
		);
		for (std::size_t i = 0; i < project.m_sources.size(); i++)
			project.m_sources[i]->index = i;

		return Result<Project *>(&project);
	}

	/* Build project */
	Result<None> Project::build() {

		if (!m_rebuild) return Result<None>();
		m_rebuild = false;

		/* Build dependencies */
		for (Project *project: m_dependencies)
			BEA_OK(None, project->build());

		std::cout <<
			"Building " << m_name <<
			" (" << targetToString(m_target) <<
			")...\n" << std::endl;

		std::size_t stages = 2;

		/* First stage: Parsing and declaring globals */
		for (std::unique_ptr<Source>& source: m_sources) {

			printProgressBar(
				source->index, stages * m_sources.size(), 40,
				source->shortPath
			);
			BEA_OK(None, buildSourceStage1(*source));
		}

		/* Second stage: Analysis and compilation */
		for (std::unique_ptr<Source>& source: m_sources) {

			printProgressBar(
				source->index + m_sources.size(),
				stages * m_sources.size(), 40,
				source->shortPath
			);
			BEA_OK(None, buildSourceStage2(*source));
		}

		/* Link project */
		std::string buildOutput = std::format(
			"build/{}/{}", targetToString(m_target), m_name
		);
		printProgressBar(
			40, 40, 40,
			buildOutput
		);

		return Result<None>();
	}

	/* Run project */
	Result<None> Project::run() {

		if (m_type != Type::Executable)
			return Result<None>(std::format(
				"Can't run '{}': Not an executable",
				m_name
			));

		std::cout <<
			"Running " << m_name <<
			" (" << targetToString(m_target) <<
			")..." << std::endl;

		/* Run process */
		std::string path = std::format(
			"{}/build/{}/{}",
			m_path, targetToString(m_target),
			m_name
		);

		const char *argv[] = {
			path.c_str(),
			NULL,
		};
		int code = reproc_run(argv, (reproc_options){0});

		if (code < 0) return Result<None>(std::format(
			"Can't run '{}': {}", m_name,
			reproc_strerror(code)
		));
		else if (code) return Result<None>(std::format(
			"Execution failed (code %d)", code
		));
		return Result<None>();
	}

	/* Turn project relative path to global path */
	std::string Project::localPath(const std::string& path) {

		return std::format("{}/{}", m_path, path);
	}

	/* Print progress bar */
	void Project::printProgressBar(
		std::size_t index,
		std::size_t count,
		std::size_t width,
		const std::string& text
	) {
		std::size_t size = 40;

		std::size_t position = index * width / count;
		std::size_t percentage = index * 100 / count;

		if (position > width) position = width;
		if (percentage > 100) percentage = 100;

		std::cout << "\e[A\r\e[0K[\e[7m";

		for (std::size_t i = 0; i < position; i++) {

			if (i-1 < text.length()) {

				if (i >= width - 4 && i < width - 1)
					std::cout << '.';
				else if (i >= width - 1)
					std::cout << ' ';
				else if (i >= 1)
					std::cout << text[i-1];
			}
			else std::cout << ' ';
		}
		std::cout << "\e[0m";
		for (std::size_t i = position; i < width; i++) {

			if (i-1 < text.length()) {

				if (i >= width - 4 && i < width - 1)
					std::cout << '.';
				else if (i >= width - 1)
					std::cout << ' ';
				else if (i >= 1)
					std::cout << text[i-1];
			}
			else std::cout << ' ';
		}
		std::cout << "] " << percentage << "%" << std::endl;
	}

	/* Load project configuration */
	Result<None> Project::loadConfiguration() {

		std::string path = localPath("project.toml");
		toml_result_t result = toml_parse_file_ex(path.c_str());

		if (!result.ok) return Result<None>(result.errmsg);

		/* Get elements */
		toml_datum_t name = toml_seek(result.toptab, "project.name");
		toml_datum_t type = toml_seek(result.toptab, "project.type");
		toml_datum_t noStdlib = toml_seek(result.toptab, "project.options.noStdlib");
		toml_datum_t dependencies = toml_seek(result.toptab, "project.dependencies");

		if (name.type != TOML_STRING) {

			toml_free(result);
			return Result<None>(
				"'project.toml' must have project name"
			);
		}
		m_name = name.u.str.ptr;

		if (noStdlib.type == TOML_BOOLEAN)
			m_options.noStdlib = noStdlib.u.boolean;

		/* Parse type */
		if (type.type == TOML_STRING) {
			if (!strcmp(type.u.str.ptr, "exec"))
				m_type = Type::Executable;
			else if (!strcmp(type.u.str.ptr, "lib"))
				m_type = Type::Library;
			else
				return Result<None>(std::format(
					"Unknown project type '{}'",
					type.u.str.ptr
				));
		}

		/* Standard library dependency */
		const char *beaPath = getenv("BEA_PATH");
		if (!beaPath || !*beaPath)
			return Result<None>(
				"No 'BEA_PATH' environment variable set"
			);
		if (!m_options.noStdlib) {

			std::string path = std::format(
				"{}/stdlib", beaPath
			);
			Project *stdlibProject = nullptr;
			Result<Project *> result = getProject("std");

			if (!stdlibProject)
				stdlibProject = BEA_OK(None, loadProject(path));
			else
				stdlibProject = result.value;

			m_dependencies.push_back(stdlibProject);
		}

		/* Search dependencies */
		if (dependencies.type == TOML_TABLE) {

			for (std::size_t i = 0; i < dependencies.u.tab.size; i++) {

				const char *csName = nullptr;
				const char *csUrl = nullptr;
				const char *csPath = nullptr;

				csName = dependencies.u.tab.key[i];
				toml_datum_t *datum = dependencies.u.tab.value+i;

				/* Determine path and URL */
				if (datum->type == TOML_STRING) {

					if (std::strstr(datum->u.str.ptr, "://"))
						csUrl = datum->u.str.ptr;
					else
						csPath = datum->u.str.ptr;
				}
				else if (datum->type == TOML_TABLE) {

					toml_datum_t url = toml_get(*datum, "url");
					toml_datum_t path = toml_get(*datum, "path");

					if (url.type != TOML_STRING &&
					    path.type != TOML_STRING)
						return Result<None>(
							std::format(
								"Dependency '{}' must have "
								"url or local path",
								csName
							)
						);
				}

				/* Get or create project */
				std::string name = csName;
				Result<Project *> result = getProject(name);
				Project *project = nullptr;

				if (!result.okay) {

					std::string path;
					if (csPath) path = std::format(
						"{}/{}", m_path, csPath
					);
					else path = std::format(
						"{}/build/vendor/{}",
						s_globalPath, name
					);

					std::string url;
					if (csUrl) url = csUrl;

					project = BEA_OK(None, loadProject(
						path, url, m_target
					));
				}
				else project = result.value;
				m_dependencies.push_back(project);
			}
		}

		toml_free(result);
		return Result<None>();
	}

	/* Fetch project */
	Result<None> Project::fetch() {

		return Result<None>("Unimplemented");
	}

	/* Locate sources */
	Result<None> Project::locateSources(const std::string& path) {

		for (const auto& entry: std::filesystem::directory_iterator{
			localPath(path)
		}) {
			std::filesystem::path entryPath = entry.path();

			/* Search another directory */
			if (entry.is_directory())
				BEA_OK(None, locateSources(std::format(
					"{}/{}", path,
					entryPath.filename().string()
				)));

			/* Source file */
			else if (entryPath.extension().string() == ".bea") {

				std::string shortPath = std::format(
					"{}/{}", path, entryPath.filename().string()
				);
				Name moduleName = {
					m_name,
					entryPath.stem()
				};
				Hash hash(
					(const std::uint8_t *)shortPath.c_str(),
					shortPath.length()
				);

				/* Determine paths */
				std::string entryPathString = entryPath.string();
				std::string outputPath = std::format(
					"{}/build/{}/cache/{}",
					m_path, targetToString(m_target),
					hash.toString()
				);
				std::filesystem::create_directories(outputPath);

				/* Add source */
				std::unique_ptr<Source> source =
					std::make_unique<Source>();

				source->path = std::move(entryPathString);
				source->outputPath = std::move(outputPath);
				source->shortPath = std::move(shortPath);
				source->moduleName = std::move(moduleName);
				source->recompile = true;

				if (source->recompile && m_name != "std") // TODO: remove second check
					m_rebuild = true;

				m_sources.emplace_back(std::move(source));
			}
		}
		return Result<None>();
	}

	/* Build source stage 1 */
	Result<None> Project::buildSourceStage1(Source& source) {

		if (!source.recompile)
			return Result<None>();

		/* Parse source */
		std::ifstream stream(source.path, std::ios::binary);
		if (!stream.is_open())
			return Result<None>(std::format(
				"Can't open file '{}': {}",
				source.path, std::strerror(errno)
			));

		Lexer lexer(source.path.c_str(), stream);
		BEA_OK(None, lexer.makeTokens());

		stream.close();

		Parser parser(lexer.m_tokens);

		source.node = BEA_OK(None, parser.parse());
		source.tokens = std::move(lexer.m_tokens);

		/* Minor optimizations */
		PostParseSimplifier postParseSimplifier;
		BEA_OK(None, postParseSimplifier.visit(*source.node));

		/* Global resolver analysis */
		std::string grPath = std::format(
			"{}/grdata.bin", source.outputPath
		);
		std::ofstream grStream(grPath, std::ios::binary);
		if (!grStream.is_open())
			return Result<None>(std::format(
				"Can't open file '{}': {}",
				grPath, std::strerror(errno)
			));
		GRCodeSerializer serializer(grStream);

		source.context = std::make_unique<Context>(
			source.path, source.moduleName
		);
		GlobalResolver globalResolver(
			*source.context, serializer
		);

		BEA_OK(None, globalResolver.analyze(*source.node));
		grStream.close();

		return Result<None>();
	}

	/* Build source stage 2 */
	Result<None> Project::buildSourceStage2(Source& source) {

		if (!source.recompile)
			return Result<None>();

		source.dependencyDelta = 1;
		while (source.dependencyDelta)
			BEA_OK(None, readSourceDependencies(source));

		for (
			const Context::Reference& reference:
			source.context->m_externalReferences
		) {
			if (reference.resolved)
				continue;
			return Result<None>(
				reference.position,
				std::format(
					"Unresolved reference '{}'",
					reference.name
				)
			);
		}
		source.context->clearDeadFrames();

		/* Local resolver analysis */
		//LocalResolver localResolver(*source.context);
		//BEA_OK(None, localResolver.analyze(*source.node));

		return Result<None>();
	}

	/* Read source dependencies */
	Result<None> Project::readSourceDependencies(Source& source) {

		source.dependencyDelta = 0;

		for (
			Context::Reference& reference:
			source.context->m_externalReferences
		) {
			if (reference.resolved)
				continue;
			if (reference.name.components.size() <= 1)
				return Result<None>(
					reference.position,
					std::format(
						"Unresolved reference '{}'",
						reference.name
					)
				);
			BEA_OK(None, getSourceDependency(
				source,
				reference.name,
				reference.position
			));
		}

		/* Resolve dependencies */
		for (
			std::pair<Name, SourceDependency>& pair:
			source.dependencies
		) {
			if (!pair.second.read) {

				std::string sourcePath =
					pair.second.project->m_path +
					pair.second.shortPath;

				BEA_OK(None, source.context->resolveModule(
					pair.second.outputPath + "/grdata.bin",
					pair.first,
					sourcePath
				));
				source.dependencyDelta++;
			}
			pair.second.read = true;
		}

		return Result<None>();
	}

	/* Get source dependency for name */
	Result<Project::SourceDependency *> Project::getSourceDependency(
		Source& source,
		const Name& name,
		const Position& position
	) {
		Result<Project *> result = getProject(name.components[0]);
		Project *project = this;

		std::size_t start = 0;
		if (result.okay) {

			start++;
			project = result.value;
		}

		/* Determine module if already loaded */
		Name moduleName{};
		std::ostringstream shortPathStream{};
		std::string outputPath{};

		SourceDependency *dependency = nullptr;

		for (std::size_t i = 0; i < name.components.size(); i++) {

			const std::string& component =
				name.components[i];

			moduleName.components.push_back(component);

			Result<SourceDependency *> result =
				source.dependencies.get(moduleName);
			if (result.okay) {
				
				dependency = result.value;
				break;
			}
		}

		/* Determine module if not loaded */
		if (!dependency) {

			shortPathStream << "src";

			moduleName.components.clear();
			if (start) moduleName.components.push_back(
				name.components[0]
			);

			for (std::size_t i = start; i < name.components.size(); i++) {

				const std::string& component =
					name.components[i];

				moduleName.components.push_back(component);

				shortPathStream << "/" << component;

				outputPath = std::format(
					"{}/build/{}/cache/{}",
					project->m_path,
					targetToString(project->m_target),
					Hash(shortPathStream.str() + ".bea").toString()
				);
				if (std::filesystem::exists(outputPath)) {

					dependency = &source.dependencies[moduleName];
					break;
				}
			}
		}

		/* No dependency */
		if (!dependency)
			return Result<SourceDependency *>(
				position,
				std::format(
					"Unresolved reference '{}'",
					name
				)
			);

		/* Fill out dependency info */
		if (!dependency->outputPath.length()) {

			dependency->project = project;
			dependency->shortPath = std::move(shortPathStream.str());
			dependency->outputPath = std::move(outputPath);
		}
		return Result<SourceDependency *>(dependency);
	}
}
