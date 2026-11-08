#pragma once

#include <bea/core.hpp>
#include <bea/token.hpp>
#include <bea/context.hpp>

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
		 * Project source dependency
		 */
		struct SourceDependency {
			std::string outputPath{};
			std::string shortPath{};
			bool read = false;
		};

		/*
		 * Project source
		 */
		struct Source {
			std::size_t index = 0;
			std::string path{};
			std::string outputPath{};
			std::string shortPath{};
			bool recompile = false;

			std::vector<std::unique_ptr<Token>> tokens;
			std::unique_ptr<Node> node = nullptr;

			std::unique_ptr<Context> context = nullptr;
			OrderedMap<Name, SourceDependency> dependencies;
			std::size_t dependencyDelta = 0;
		};

		/*
		 * Project
		 */
		Project(
			const std::string& path,
			Target target = Target::Debug
		)
			: m_path(path),
			  m_target(target) {}

		static std::string s_defaultURL;
		static std::string s_globalPath;

		const std::string& getPath() const { return m_path; }
		const std::string& getName() const { return m_name; }
		Type getType() const { return m_type; }

		/*
		 * Load or get project
		 */
		static Result<Project *> getProject(const std::string& name);

		static Result<Project *> loadProject(
			const std::string& path,
			const std::string& url = s_defaultURL,
			Target target = Target::Debug
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

		Target m_target;

		struct {
			bool noStdlib = false;
		} m_options;

		bool m_rebuild = false;
		std::vector<Project *> m_dependencies;

		static std::vector<std::unique_ptr<Project>> s_projects;

		/*
		 * Turn project relative path to global
		 * path
		 */
		std::string localPath(const std::string& path);

		/*
		 * Print progress bar
		 */
		void printProgressBar(
			std::size_t index,
			std::size_t count,
			std::size_t width,
			const std::string& text
		);

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
		Result<None> buildSourceStage1(Source& source);
		Result<None> buildSourceStage2(Source& source);

		/*
		 * Read source dependencies
		 */
		Result<None> readSourceDependencies(Source& source);

		/*
		 * Get source dependency for name
		 */
		Result<SourceDependency *> getSourceDependency(
			Source& source,
			const Name& name
		);
	};
}
