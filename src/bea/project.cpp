/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <iostream>
#include <format>
#include <filesystem>
#include <fstream>
#include <tomlc17.h>
#include <bea/core.hpp>
#include <bea/process.hpp>
#include <bea/hash.hpp>
#include <bea/lexer.hpp>
#include <bea/parser.hpp>
#include <bea/optimizer/postparsesimplifier.hpp>
#include <bea/optimizer/statementsimplifier.hpp>
#include <bea/optimizer/implicitexpander.hpp>
#include <bea/optimizer/constantfolder.hpp>
#include <bea/analyzer/globalresolver.hpp>
#include <bea/analyzer/localresolver.hpp>
#include <bea/analyzer/documenter.hpp>
#include <bea/analyzer/compiler.hpp>
#include <bea/project.hpp>

namespace bea {

	std::string Project::s_defaultURL = "";
	std::string Project::s_globalPath;
	bool Project::s_verbose = false;
	bool Project::s_buildAll = false;

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

		s_projects.insert(
			s_projects.begin(),
			std::make_unique<Project>(
				path, url, target
			)
		);
		Project& project = *s_projects[0];

		/* Load project information */
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

		/* Check dependencies */
		for (Project *dependency: project.m_dependencies) {

			if (dependency->m_rebuild)
				project.m_rebuild = true;
		}
		return Result<Project *>(&project);
	}

	/* Build all projects */
	Result<None> Project::buildAll() {

		for (std::unique_ptr<Project>& project: s_projects)
			BEA_OK(None, project->build());
		return Result<None>();
	}

	/* Build project */
	Result<None> Project::build() {

		if (!m_rebuild) return Result<None>();
		m_rebuild = false;

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
			if (source->kind == SourceKind::Bea)
				BEA_OK(None, buildSourceStage1(*source));

			/* Compile C file */
			else if (source->kind == SourceKind::C) {

				std::string objPath = std::format(
					"{}/output.o",
					source->outputPath
				);
				std::vector<const char *> ccArguments = {
					"cc", "-c", "-o",
					objPath.c_str(),
					source->path.c_str()
				};
				for (const std::string& argument: m_options.cflags)
					ccArguments.push_back(argument.c_str());

				BEA_OK(None, runProcess(
					std::move(ccArguments),
					s_verbose
				));
				if (s_verbose) std::cout << std::endl;
			}
		}

		/* Second stage: Analysis and compilation */
		for (std::unique_ptr<Source>& source: m_sources) {

			if (source->kind != SourceKind::Bea)
				continue;

			printProgressBar(
				source->index + m_sources.size(),
				stages * m_sources.size(), 40,
				source->shortPath
			);
			BEA_OK(None, buildSourceStage2(*source));
		}

		/* Link project */
		const char *prefix = "";
		const char *extension = "";

		if (m_type == Type::Library) {

			prefix = "lib";
			extension = ".a";
		}
		std::string buildOutput = std::format(
			"{}/build/{}/{}{}{}",
			m_path,
			targetToString(m_target),
			prefix,
			m_name,
			extension
		);
		printProgressBar(
			40, 40, 40,
			buildOutput
		);
		std::vector<std::string> inputs{};

		/* Add objects to list */
		for (std::unique_ptr<Source>& source: m_sources)
			inputs.emplace_back(std::format(
				"{}/output.o",
				source->outputPath
			));

		/* Add libraries to list */
		if (m_type == Type::Executable) {

			for (std::unique_ptr<Project>& project: s_projects) {

				if (project.get() == this ||
				    project->m_type != Type::Library)
					continue;
				inputs.emplace_back(std::format(
					"{}/build/{}/lib{}.a",
					project->m_path,
					targetToString(project->m_target),
					project->m_name
				));
			}
		}

		/* Generate initialization table */
		std::string initQbePath = std::format(
			"{}/build/{}/cache/inittab.qbe",
			m_path, targetToString(m_target)
		);
		std::string initAsmPath = std::format(
			"{}/build/{}/cache/inittab.s",
			m_path, targetToString(m_target)
		);
		std::string initObjPath = std::format(
			"{}/build/{}/cache/inittab.o",
			m_path, targetToString(m_target)
		);
		std::ofstream initQbeStream(initQbePath, std::ios::binary);
		if (!initQbeStream.is_open())
			return Result<None>(std::format(
				"Can't open file '{}': {}",
				initQbePath,
				std::strerror(errno)
			));
		initQbeStream << "export function l $";

		if (m_type == Type::Library)
			initQbeStream << m_name << "____init";
		else
			initQbeStream << "____beainit";

		initQbeStream << "() {\n@start\n";

		/* Add library initializers */
		if (m_type == Type::Executable) {

			for (std::unique_ptr<Project>& project: s_projects) {

				if (this == project.get() ||
				    project->m_type != Type::Library)
					continue;
				initQbeStream <<
					"call $" << project->m_name <<
					"____init()\n";
			}
		}

		/* Add module initializers */
		for (std::unique_ptr<Source>& source: m_sources) {

			if (source->kind != SourceKind::Bea)
				continue;

			initQbeStream << "call $";
			for (const std::string component: source->moduleName)
				initQbeStream << component << "__";
			initQbeStream << "__init()\n";
		}

		/* Add call to main */
		if (m_type == Type::Executable)
			initQbeStream <<
				"%r0 =l call $__std__main(l $" <<
				m_name << "__main__main____0)\n";
		else
			initQbeStream << "%r0 =l copy 0\n";
		initQbeStream << "ret %r0\n}\n";
		initQbeStream.close();

		const char *beaPath = getenv("BEA_PATH");
		std::string qbePath = std::format(
			"{}/vendor/bin/qbe", beaPath
		);

		/* QBE to assembly, assembly to object */
		BEA_OK(None, runProcess({
			qbePath.c_str(), "-o",
			initAsmPath.c_str(),
			initQbePath.c_str()
		}, s_verbose));

		BEA_OK(None, runProcess({
			"cc", "-c", "-o",
			initObjPath.c_str(),
			initAsmPath.c_str()
		}, s_verbose));

		/* Run linker */
		if (m_type == Type::Executable) {

			std::vector<const char *> ccldArguments = {
				"cc", "-o", buildOutput.c_str(),
				initObjPath.c_str()
			};
			for (const std::string& input: inputs)
				ccldArguments.push_back(input.c_str());

			for (const std::string& argument: m_options.ldflags)
				ccldArguments.push_back(argument.c_str());

			BEA_OK(None, runProcess(
				std::move(ccldArguments),
				s_verbose
			));
		}

		/* Run archiver */
		else if (m_type == Type::Library) {

			std::vector<const char *> arArguments = {
				"ar", "cr", buildOutput.c_str(),
				initObjPath.c_str()
			};
			for (const std::string& input: inputs)
				arArguments.push_back(input.c_str());

			BEA_OK(None, runProcess(
				std::move(arArguments),
				s_verbose
			));
		}
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

		BEA_OK(None, runProcess({
			path.c_str(),
		}, s_verbose));
		return Result<None>();
	}

	/* Turn project relative path to global path */
	std::string Project::localPath(const std::string& path) {

		return std::format("{}/{}", m_path, path);
	}

	/* Substiute configuration parameters */
	std::string Project::substituteConfigParameters(const char *source) {

		std::ostringstream stream;
		for (; *source; source++) {

			if (*source != '%') {

				stream << *source;
				continue;
			}
			source++;
			if (*source != '(') {

				stream << *source;
				continue;
			}
			source++;
			std::ostringstream nameStream;

			for (; *source && *source != ')'; source++)
				nameStream << *source;
			if (*source != ')') source--;

			std::string name = nameStream.str();
			if (name == "path")
				stream << m_path;
			else if (name == "url")
				stream << m_url;
			else if (name == "target")
				stream << targetToString(m_target);
		}
		return stream.str();
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
		toml_datum_t genDocs = toml_seek(result.toptab, "project.options.genDocs");
		toml_datum_t cflags = toml_seek(result.toptab, "project.options.cflags");
		toml_datum_t ldflags = toml_seek(result.toptab, "project.options.ldflags");
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

		if (genDocs.type == TOML_BOOLEAN && genDocs.u.boolean)
			m_options.genDocs = m_path + "/doc/json";
		else if (genDocs.type == TOML_STRING)
			m_options.genDocs = std::format("{}/{}", m_path, genDocs.u.str.ptr);

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

		/* Parse cflags and ldflags */
		if (cflags.type == TOML_ARRAY) {

			for (int32_t i = 0; i < cflags.u.arr.size; i++) {

				toml_datum_t *datum = cflags.u.arr.elem+i;
				if (datum->type != TOML_STRING)
					continue;
				m_options.cflags.emplace_back(
					substituteConfigParameters(datum->u.str.ptr)
				);
			}
		}
		if (ldflags.type == TOML_ARRAY) {

			for (int32_t i = 0; i < ldflags.u.arr.size; i++) {

				toml_datum_t *datum = ldflags.u.arr.elem+i;
				if (datum->type != TOML_STRING)
					continue;
				m_options.ldflags.emplace_back(
					substituteConfigParameters(datum->u.str.ptr)
				);
			}
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
					csUrl = url.u.str.ptr;
					csPath = path.u.str.ptr;
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

		std::cout << "Fetching '" << m_url << "'..." << std::endl;

		const char *argv[] = {
			"git", "clone",
			m_url.c_str(),
			m_path.c_str(),
			"--quiet",
			"--recursive",
			NULL,
		};
		BEA_OK(None, runProcess({
			"git", "clone",
			m_url.c_str(),
			m_path.c_str(),
			"--quiet",
			"--recursive",
		}));
		return Result<None>();
	}

	/* Locate sources */
	Result<None> Project::locateSources(const std::string& path) {

		for (const auto& entry: std::filesystem::directory_iterator{
			localPath(path)
		}) {
			std::filesystem::path entryPath = entry.path();

			/* Search another directory */
			if (entry.is_directory()) {

				BEA_OK(None, locateSources(std::format(
					"{}/{}", path,
					entryPath.filename().string()
				)));
				continue;
			}
			SourceKind kind;

			std::string extension = entryPath.extension().string();
			if (extension == ".bea")
				kind = SourceKind::Bea;
			else if (extension == ".c")
				kind = SourceKind::C;
			else
				continue;

			/* Source file */
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

			/* Determine if source should be recompiled */
			bool recompile = false;

			std::string objPath = std::format(
				"{}/output.o", outputPath
			);
			if (s_buildAll)
				recompile = true;
			else if (fileNewerThan(entryPathString, objPath))
				recompile = true;
			else {
				/* Open dependency list */
				std::ifstream depStream(std::format(
					"{}/dep.list", outputPath
				), std::ios::binary);

				std::vector<char> stringData;
				stringData.resize(1024);

				if (depStream.is_open()) {
					while (!depStream.eof()) {

						/* Read line */
						depStream.getline(
							stringData.data(),
							stringData.capacity()
						);
						std::size_t len = std::strlen(
							stringData.data()
						);
						if (len && stringData[len-1] == '\n')
							stringData[--len] = 0;
						if (!len) continue;

						/* Check dependency */
						std::string path(stringData.data());
						if (fileNewerThan(path, objPath)) {

							recompile = true;
							break;
						}
					}
					depStream.close();
				}
			}

			/* Add source */
			std::unique_ptr<Source> source =
				std::make_unique<Source>();

			source->kind = kind;
			source->path = std::move(entryPathString);
			source->outputPath = std::move(outputPath);
			source->shortPath = std::move(shortPath);
			source->moduleName = std::move(moduleName);
			source->recompile = recompile;

			if (recompile) m_rebuild = true;

			m_sources.emplace_back(std::move(source));
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

		/* Resolve dependencies */
		std::string depPath = std::format(
			"{}/dep.list", source.outputPath
		);
		std::ofstream depStream(depPath, std::ios::binary);
		if (!depStream.is_open())
			return Result<None>(std::format(
				"Can't open file '{}': {}",
				depPath, std::strerror(errno)
			));

		source.dependencyDelta = 1;
		while (source.dependencyDelta)
			BEA_OK(None, readSourceDependencies(source, depStream));

		depStream.close();

		/* Error if there is at least one unresolved reference left */
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
		source.context->resolveStrayTypeIDs();
		source.context->clearDeadFrames();

		/* Local resolver analysis */
		LocalResolver localResolver(*source.context);
		BEA_OK(None, localResolver.analyze(*source.node));

		/* Documentation generation */
		if (!m_options.genDocs.empty()) {

			std::ostringstream stream;
			stream << m_options.genDocs;

			std::ostringstream dirStream;
			dirStream << m_options.genDocs;

			for (
				std::size_t i = 1;
				i < source.moduleName.components.size();
				i++
			) {
				if (i < source.moduleName.components.size()-1)
					dirStream << "/" << source.moduleName.components[i];
				stream << "/" << source.moduleName.components[i];
			}

			stream << ".json";
			std::string jsonPath = stream.str();

			std::filesystem::create_directories(dirStream.str());
			dirStream.clear();

			/* Open output file */
			std::ofstream docStream(jsonPath);
			if (!docStream.is_open())
				return Result<None>(
					std::format(
						"Can't open file '{}': {}",
						jsonPath, std::strerror(errno)
					)
				);
			Documenter documenter(*source.context, docStream);
			BEA_OK(None, documenter.analyze(*source.node));

			docStream.close();
		}

		/* Optimization passes */
		StatementSimplifier statementSimplifier(*source.context);
		BEA_OK(None, statementSimplifier.visit(*source.node));

		ImplicitExpander implicitExpander(*source.context);
		BEA_OK(None, implicitExpander.visit(*source.node));

		ConstantFolder constantFolder(*source.context);
		BEA_OK(None, constantFolder.visit(*source.node));

		/* Compile to slkwvr */
		std::string swPath = std::format(
			"{}/output.sw", source.outputPath
		);
		std::ofstream swStream(swPath);
		if (!swStream.is_open())
			return Result<None>(
				std::format(
					"Can't open file '{}': {}",
					swPath, std::strerror(errno)
				)
			);
		Compiler compiler(*source.context, swStream);
		BEA_OK(None, compiler.analyze(*source.node));

		swStream.close();

		/* Slkwvr to QBE */
		const char *beaPath = std::getenv("BEA_PATH");

		std::string swBinPath = std::format(
			"{}/vendor/bin/slkwvr",
			beaPath
		);
		std::string qbeBinPath = std::format(
			"{}/vendor/bin/qbe",
			beaPath
		);
		std::string qbePath = std::format(
			"{}/output.qbe",
			source.outputPath
		);
		BEA_OK(None, runProcess({
			swBinPath.c_str(),
			swPath.c_str(),
			qbePath.c_str()
		}, s_verbose));

		/* QBE to assembly */
		std::string asmPath = std::format(
			"{}/output.s",
			source.outputPath
		);
		BEA_OK(None, runProcess({
			qbeBinPath.c_str(),
			"-o",
			asmPath.c_str(),
			qbePath.c_str()
		}, s_verbose));

		/* Assembly to object */
		std::string objPath = std::format(
			"{}/output.o",
			source.outputPath
		);
		BEA_OK(None, runProcess({
			"cc", "-c", "-o",
			objPath.c_str(),
			asmPath.c_str()
		}, s_verbose));
		if (s_verbose) std::cout << std::endl;

		return Result<None>();
	}

	/* Read source dependencies */
	Result<None> Project::readSourceDependencies(
		Source& source,
		std::ostream& depStream
	) {
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
				depStream,
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
		std::ostream& depStream,
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

			depStream <<
				project->m_path << "/" <<
				dependency->shortPath << ".bea\n";
		}
		return Result<SourceDependency *>(dependency);
	}

	/* Check if file is newer than other */
	bool Project::fileNewerThan(
		const std::string& a,
		const std::string& b
	) {
		if (!std::filesystem::exists(b))
			return true;

		std::filesystem::file_time_type aType =
			std::filesystem::last_write_time(a);
		std::filesystem::file_time_type bType =
			std::filesystem::last_write_time(b);

		return aType >= bType;
	}
}
