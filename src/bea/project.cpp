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
#include <bea/project.hpp>

namespace bea {

	std::string Project::defaultURL = "";
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

		if (name.type != TOML_STRING) {

			toml_free(result);
			return Result<None>(
				"'project.toml' must have project name"
			);
		}
		m_name = name.u.str.ptr;

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
				m_sources.emplace_back(std::make_unique<Source>(Source{
					.path = std::move(entryPathString),
					.outputPath = std::move(outputPath),
					.shortPath = std::move(shortPath),
					.recompile = true,
				}));
				Source& source = *m_sources[m_sources.size()-1];
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

		Parser parser(lexer.getTokens());
		std::unique_ptr<Node> node = BEA_OK(None, parser.parse());

		/* Minor optimizations */
		PostParseSimplifier postParseSimplifier;
		BEA_OK(None, postParseSimplifier.visit(*node));

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

		source.context = std::make_unique<Context>(source.path);
		GlobalResolver globalResolver(
			*source.context, serializer
		);

		BEA_OK(None, globalResolver.analyze(*node));
		grStream.close();

		std::cout << std::endl;
		return Result<None>();
	}

	/* Build source stage 2 */
	Result<None> Project::buildSourceStage2(Source& source) {

		if (!source.recompile)
			return Result<None>();

		source.dependencyDelta = 1;
		while (source.dependencyDelta)
			BEA_OK(None, readSourceDependencies(source));

		std::cout << std::endl;
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
					reference.node.m_position,
					std::format(
						"Unresolved reference '{}'",
						reference.name
					)
				);

			SourceDependency *dependency =
				BEA_OK(None, getSourceDependency(
					source, reference.name
				));
			dependency->read = true;
		}
		return Result<None>();
	}

	/* Get source dependency for name */
	Result<Project::SourceDependency *> Project::getSourceDependency(
		Source& source,
		const Name& name
	) {
		Project::SourceDependency *dependency =
			&source.dependencies[name];

		if (!dependency->outputPath.length()) {

			std::size_t start = 0;

			/* Determine dependency paths */
			Result<Project *> result = getProject(name.components[0]);
			Project *project = this;

			if (result.okay) {

				start++;
				project = result.value;
			}
			std::ostringstream shortPathStream;
			shortPathStream << "src/";

			for (std::size_t i = start; i < name.components.size()-1; i++) {

				if (i > start) shortPathStream << "/";
				shortPathStream << name.components[i];
			}
			shortPathStream << ".bea";
			dependency->shortPath = shortPathStream.str();

			dependency->outputPath = std::format(
				"{}/build/{}/cache/{}",
				project->m_path,
				targetToString(project->m_target),
				Hash(dependency->shortPath).toString()
			);
			std::cout <<
				name << ", " <<
				dependency->shortPath << ", " <<
				dependency->outputPath << std::endl;
		}

		if (!dependency->read)
			source.dependencyDelta++;

		return Result<Project::SourceDependency *>(dependency);
	}
}
