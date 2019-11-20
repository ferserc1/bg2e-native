
#include <bg2e/db/drawable.hpp>

namespace bg2e {
namespace db {

	DrawableLoader::ReadDrawablePluginVector DrawableLoader::s_readPlugins;

	void DrawableLoader::RegisterPlugin(ReadDrawablePlugin * plugin) {
		auto it = std::find(s_readPlugins.begin(), s_readPlugins.end(), plugin);
		if (it == s_readPlugins.end()) {
			s_readPlugins.push_back(plugin);
			Plugin::RegisterDependencies(plugin);
		}
	}

	void DrawableLoader::Release() {
		s_readPlugins.clear();
	}

	DrawableLoader::DrawableLoader()
	{

	}

	scene::Drawable * DrawableLoader::load(const std::string & path) {
		base::path p(path);
		ReadDrawablePlugin * plugin = Plugin::FindPlugin<ReadDrawablePlugin>(s_readPlugins, p.extension());
		if (plugin) {
			ptr<scene::Drawable> drw = plugin->load(path);
			return drw.release();
		}
		return nullptr;
	}

	scene::Drawable * loadDrawable(const base::path & path) {
		return DrawableLoader().load(path);
	}
	
	scene::Drawable * loadDrawable(const std::string & path) {
		return DrawableLoader().load(path);
	}

	DrawableWriter::WriteDrawablePluginVector DrawableWriter::s_writePlugins;

	void DrawableWriter::RegisterPlugin(WriteDrawablePlugin * plugin) {
		auto it = std::find(s_writePlugins.begin(), s_writePlugins.end(), plugin);
		if (it == s_writePlugins.end()) {
			s_writePlugins.push_back(plugin);
			Plugin::RegisterDependencies(plugin);
		}
	}

	void DrawableWriter::Release() {
		s_writePlugins.clear();
	}

	DrawableWriter::DrawableWriter()
	{

	}

	void DrawableWriter::save(const std::string & path, scene::Drawable * drw) {
		base::path p(path);
		WriteDrawablePlugin * plugin = Plugin::FindPlugin<WriteDrawablePlugin>(s_writePlugins, p.extension());
		if (plugin) {
			plugin->save(path, drw);
		}
		else {
			throw std::runtime_error("No such plugin found to load " + path);
		}
	}

	void writeDrawable(const base::path & p, scene::Drawable * drw) {
		DrawableWriter().save(p, drw);
	}

	void writeDrawable(const std::string & p, scene::Drawable * drw) {
		DrawableWriter().save(p, drw);
	}

}
}
