#ifndef _bg2e_db_drawable_hpp_
#define _bg2e_db_drawable_hpp_

#include <bg2e/db/plugin.hpp>
#include <bg2e/scene/drawable.hpp>

namespace bg2e {
namespace db {

	typedef ReadPlugin<scene::Drawable> ReadDrawablePlugin;
	typedef WritePlugin<scene::Drawable> WriteDrawablePlugin;

	class DrawableLoader {
	public:
		typedef std::vector<ptr<ReadDrawablePlugin>> ReadDrawablePluginVector;

		static void RegisterPlugin(ReadDrawablePlugin *);
		static void Release();

		DrawableLoader();

		inline scene::Drawable * load(const base::path & p) { return load(p.toString()); }
		scene::Drawable * load(const std::string &);

	protected:
		static ReadDrawablePluginVector s_readPlugins;
	};


	extern scene::Drawable * loadDrawable(const base::path &);
	extern scene::Drawable * loadDrawable(const std::string &);

	class DrawableWriter {
	public:
		typedef std::vector<ptr<WriteDrawablePlugin>> WriteDrawablePluginVector;

		static void RegisterPlugin(WriteDrawablePlugin *);
		static void Release();

		DrawableWriter();

		inline void save(const base::path & p, scene::Drawable * drw) { save(p.toString(), drw); }
		void save(const std::string & p, scene::Drawable * drw);

	protected:
		static WriteDrawablePluginVector s_writePlugins;
	};

	extern void writeDrawable(const base::path & p, scene::Drawable * drw);
	extern void writeDrawable(const std::string & p, scene::Drawable * drw);

}
}

#endif
