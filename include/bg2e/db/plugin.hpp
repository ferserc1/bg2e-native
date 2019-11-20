#ifndef _bg2e_db_plugin_hpp_
#define _bg2e_db_plugin_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/path.hpp>

namespace bg2e {
namespace db {

	class Plugin : public base::ReferencedPointer {
	public:

		virtual bool supportFileType(const std::string & extension) = 0;
		virtual void registerDependencies() {}

		inline bool supportFileType(const base::path & path) { return supportFileType(path.extension()); }

		template <class PluginClass>
		static PluginClass * FindPlugin(std::vector<ptr<PluginClass>> & plugins, const std::string & fileType) {
			for (auto item : plugins) {
				if (item->supportFileType(fileType)) {
					return item.getPtr();
				}
			}
			return nullptr;
		}

		static void RegisterDependencies(Plugin * plugin) {
			plugin->registerDependencies();
		}
	};

	template <class T>
	class ReadPlugin : public Plugin {
	public:
		virtual T * load(const base::path &) = 0;
	};

	template <class T>
	class WritePlugin : public Plugin {
	public:
		virtual void save(const base::path &, T *) = 0;
	};

}
}

#endif
