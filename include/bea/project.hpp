#pragma once

#include <bea/core.hpp>

namespace bea {

	/*
	 * Project context
	 */
	class Project {
	public:
		/*
		 * Project type
		 */
		enum class Type {
			Executable = 0,
			Library,

			Count,
		};
		static const char *typeToString(Type type);

		/*
		 * Project target
		 */
		enum class Target {
			Debug = 0,
			Release,

			Count,
		};
		static const char *targetToString(Target target);

		/*
		 * Project source
		 */
		struct Source {
			std::size_t index;
			std::string path;
			std::string outputPath;
			std::string shortPath;
			bool recompile;
		};

		Project(const std::string& path) : m_path(path) {}

		static std::string defaultURL;

		const std::string& getPath() const { return m_path; }
		const std::string& getName() const { return m_name; }
		Type getType() const { return m_type; }

		/*
		 * Load or get project
		 */
		static Result<Project *> getProject(const std::string& name);

		static Result<Project *> loadProject(
			const std::string& path,
			const std::string& url = defaultURL
		);

		/*
		 * Build project
		 */
		Result<None> build();

		/*
		 * Run project
		 */
		Result<None> run();
	private:
		std::string m_path;
		std::string m_name;
		Type m_type = Type::Executable;

		std::vector<std::unique_ptr<Source>> m_sources;

		Target m_target = Target::Debug;

		static std::vector<std::unique_ptr<Project>> s_projects;

		/*
		 * Turn project relative path to global
		 * path
		 */
		std::string localPath(const std::string& path);

		/*
		 * Load configuration
		 */
		Result<None> loadConfiguration();

		/*
		 * Fetch project
		 */
		Result<None> fetch();

		/*
		 * Determine sources
		 */
		Result<None> locateSources(const std::string& path);

		/*
		 * Build source
		 */
		Result<None> buildSource(Source& source);
	};
}
