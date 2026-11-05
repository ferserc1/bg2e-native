/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "EditorUi.hpp"
#include "command/component/TextCommands.hpp"
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Numeric.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <map>
#include <cstdio>
#include <type_traits>

namespace bg2e_composer {
namespace {
using namespace command::component;
struct TextEditRequest {
    std::weak_ptr<bg2e::scene::Component> target;
    TextAuthoringValue value;
};

struct ScopedId {
    explicit ScopedId(int value)
    {
        bg2e::ui::Group::pushId(value);
    }

    ~ScopedId()
    {
        bg2e::ui::Group::popId();
    }
};

void collectFonts(bg2e::scene::Node& node, std::map<std::string, unsigned>& fonts)
{
    if (auto* declarations = node.getComponent<bg2e::scene::TextFontComponent>())
    {
        for (const auto& font : declarations->declarations())
        {
            ++fonts[font.identifier];
        }
    }
    for (const auto& child : node.children())
    {
        collectFonts(*child, fonts);
    }
}
}


void EditorUi::drawTextEditors(const std::shared_ptr<AppDelegate>& delegate, bg2e::scene::Node& node)
try
{
    using namespace bg2e;
    using namespace command::component;
    const auto generation = delegate->document()->generation;
    if (!_textEdit || _textEditGeneration != generation)
    {
        if (_textEdit)
        {
            _textEdit->cancel();
            if (_textEdit->active())
            {
                return;
            }
        }
        _textEditGeneration = generation;
        _textUiWarning = false;
        _textEdit = std::make_unique<ui::EditSession>(
            [weak = _delegate, generation](auto phase, uint64_t interaction, const auto&, const std::any& value)
            {
                try
                {
                    auto owner = weak.lock();
                    if (!owner || !owner->document() || owner->document()->generation != generation)
                    {
                        return phase == ui::EditPhase::Cancel;
                    }
                    const auto& request = std::any_cast<const TextEditRequest&>(value);
                    return owner->textEditIntent(request.target.lock(), phase, interaction, request.value);
                }
                catch (...)
                {
                    return false;
                }
            });
        _textEdit->beginFrame();
    }
    std::map<std::string, unsigned> registered;
    collectFonts(*delegate->document()->root, registered);
    const auto componentKey = [](scene::Component* component, const std::string& field)
    {
        return "text:" + std::to_string(reinterpret_cast<uintptr_t>(component)) + ":" + field;
    };
    // Each widget has a transient draft in the ordinary EditSession. Full CPU
    // values travel through the shared value-edit queue and one history gesture.
    const auto field = [&](scene::Component& component, const std::string& name, auto controls)
    {
        const auto key = componentKey(&component, name);
        auto request = std::any_cast<TextEditRequest>(_textEdit->value(key,
            TextEditRequest{component.shared_from_this(), EditText::capture(component)}));
        bool changed = false;
        const auto interaction = ui::EditInteraction::draw([&]
        {
            changed = controls(request.value);
        });
        _textEdit->submit(key, std::move(request), changed, interaction);
    };
    if (auto* text = node.getComponent<scene::TextComponent>())
    {
        ui::Text::separator("Text paragraph");
        ui::Text::text("Logical units control layout; bitmap pixels control quality; world size controls the quad.");
        field(*text, "content", [](auto& value)
        {
            return ui::Value::multilineText("Content", std::get<TextValues>(value).paragraph.content);
        });
        field(*text, "fontIdentifier", [](auto& value)
        {
            return ui::Value::text("Font identifier", std::get<TextValues>(value).paragraph.fontIdentifier, 256);
        });
        std::vector<std::string> names, identifiers;
        for (const auto& [identifier, count] : registered)
        {
            const auto font = node.scene() ? node.scene()->textFont(identifier) : nullptr;
            identifiers.push_back(identifier);
            names.push_back(identifier + (count > 1 ? " (duplicate: disabled)" : !font ? " (unavailable)" : ""));
        }
        if (!names.empty())
        {
            field(*text, "fontChoice", [&](auto& value)
            {
                auto& paragraph = std::get<TextValues>(value).paragraph;
                uint32_t selected = 0;
                for (uint32_t i = 0; i < identifiers.size(); ++i)
                {
                    if (identifiers[i] == paragraph.fontIdentifier)
                    {
                        selected = i;
                    }
                }
                if (ui::Value::comboBox("Declared fonts", names, selected))
                {
                    paragraph.fontIdentifier = identifiers[selected];
                    return true;
                }
                return false;
            });
        }
        if (!registered.contains(text->text().fontIdentifier()))
        {
            ui::Text::text("Missing identifier: add Text Fonts on the root (recommended) or any scene node.");
        }
        else if (!node.scene() || !node.scene()->textFont(text->text().fontIdentifier()))
        {
            ui::Text::text("Font unavailable: fix duplicate identifiers/source path, then Reload fonts. See console warnings.");
        }
        const auto number = [&](const char* label, auto member)
        {
            field(*text, label, [&](auto& value)
            {
                return ui::Numeric::number(label, &member(std::get<TextValues>(value)));
            });
        };
        number("Font size (logical)", [](auto& v) -> float&
        {
            return v.paragraph.fontSize;
        });
        number("Line spacing multiplier", [](auto& v) -> float&
        {
            return v.paragraph.lineSpacing;
        });
        number("Character spacing (logical)", [](auto& v) -> float&
        {
            return v.paragraph.characterSpacing;
        });
        number("Paragraph width", [](auto& v) -> float&
        {
            return v.paragraph.paragraphSize.width;
        });
        number("Paragraph height", [](auto& v) -> float&
        {
            return v.paragraph.paragraphSize.height;
        });
        field(*text, "resolution", [](auto& value)
        {
            auto& resolution = std::get<TextValues>(value).paragraph.bitmapResolution;
            bool changed = ui::Numeric::drag("Bitmap width (pixels)", &resolution.width);
            changed = ui::Numeric::drag("Bitmap height (pixels)", &resolution.height) || changed;
            return changed;
        });
        number("Left margin", [](auto& v) -> float&
        {
            return v.paragraph.margins.left;
        });
        number("Top margin", [](auto& v) -> float&
        {
            return v.paragraph.margins.top;
        });
        number("Right margin", [](auto& v) -> float&
        {
            return v.paragraph.margins.right;
        });
        number("Bottom margin", [](auto& v) -> float&
        {
            return v.paragraph.margins.bottom;
        });
        const auto choice = [&](const char* label, const std::vector<std::string>& options, auto member)
        {
            field(*text, label, [&](auto& value)
            {
                auto& selected = member(std::get<TextValues>(value).paragraph);
                auto index = static_cast<uint32_t>(selected);
                const bool changed = ui::Value::comboBox(label, options, index);
                selected = static_cast<std::remove_reference_t<decltype(selected)>>(index);
                return changed;
            });
        };
        choice("Wrapping", {"None", "Word", "Character"}, [](auto& p) -> auto&
        {
            return p.wrapMode;
        });
        choice("Horizontal alignment", {"Left", "Center", "Right"}, [](auto& p) -> auto&
        {
            return p.alignment;
        });
        choice("Vertical alignment", {"Top", "Center", "Bottom"}, [](auto& p) -> auto&
        {
            return p.verticalAlignment;
        });
        choice("Overflow", {"Clip with warning", "Hide with warning"}, [](auto& p) -> auto&
        {
            return p.overflowPolicy;
        });
        ui::Text::separator("Text rectangle");
        field(*text, "preserveAspect", [](auto& value)
        {
            return ui::Button::checkBox("Preserve logical paragraph aspect", &std::get<TextValues>(value).preserveAspect);
        });
        number("World height", [](auto& v) -> float&
        {
            return v.size.y;
        });
        if (!text->preserveAspect())
        {
            number("World width", [](auto& v) -> float&
            {
                return v.size.x;
            });
        }
        number("Pivot X (0..1)", [](auto& v) -> float&
        {
            return v.pivot.x;
        });
        number("Pivot Y (0..1)", [](auto& v) -> float&
        {
            return v.pivot.y;
        });
        ui::Text::text("Appearance: Model Properties > Text submesh > Material (including generated albedo preview).");
        if (ui::Button::button("Select text material"))
        {
            if (_textEdit->commit())
            {
                delegate->selectNode(node.shared_from_this(), 0, false);
            }
        }
    }
    if (auto* fonts = node.getComponent<scene::TextFontComponent>())
    {
        ui::Text::separator("Font declarations");
        ui::Text::text("Scene-wide identifiers; root placement recommended. Trusted project fonts only.");
        ui::Text::text("Duplicate identifiers across nodes are disabled. Missing files omit dependent text, not the scene.");
        ui::Text::text("Incomplete identifiers/paths or duplicates within this collection retain the last valid declarations.");
        const auto fontField = [&](size_t index, const char* name, auto controls)
        {
            field(*fonts, "font:" + std::to_string(index) + ":" + name, [&](auto& value)
            {
                auto& declarations = std::get<FontDeclarations>(value);
                return index < declarations.size() && controls(declarations[index]);
            });
        };
        for (size_t index = 0; index < fonts->declarations().size(); ++index)
        {
            ScopedId fontId(static_cast<int>(index));
            fontField(index, "identifier", [](auto& font)
            {
                return ui::Value::text("Identifier", font.identifier, 256);
            });
            fontField(index, "path", [](auto& font)
            {
                auto path = font.path.string();
                bool changed = ui::Value::text("Font file", path, static_cast<int>(path.size() + 16385));
                if (ui::Button::button("Choose font file..."))
                {
                    const auto selected = app::FileDialog::getOpenFilePath({{"TrueType/OpenType fonts", "ttf,otf,ttc"}});
                    if (!selected.empty())
                    {
                        path = selected.string();
                        changed = true;
                    }
                }
                if (changed)
                {
                    font.path = path;
                }
                return changed;
            });
            fontField(index, "face", [](auto& font)
            {
                return ui::Numeric::drag("Face index", &font.faceIndex);
            });
            for (size_t notice = 0; notice < fonts->declarations()[index].notices.size(); ++notice)
            {
                ui::Text::text(fonts->declarations()[index].notices[notice].string());
                ScopedId noticeId(static_cast<int>(notice));
                fontField(index, ("notice:" + std::to_string(notice)).c_str(), [&](auto& font)
                {
                    if (ui::Button::button("Remove notice") && notice < font.notices.size())
                    {
                        font.notices.erase(font.notices.begin() + notice);
                        return true;
                    }
                    return false;
                });
            }
            fontField(index, "addNotice", [](auto& font)
            {
                if (ui::Button::button("Add license/provenance notice..."))
                {
                    auto path = app::FileDialog::getOpenFilePath({{"License/provenance", "txt,md"}});
                    if (!path.empty() && font.notices.size() < 32)
                    {
                        font.notices.push_back(std::move(path));
                        return true;
                    }
                }
                return false;
            });
            const auto& declaration = fonts->declarations()[index];
            const auto available = node.scene() ? node.scene()->textFont(declaration.identifier) : nullptr;
            ui::Text::text(registered[declaration.identifier] > 1 ? "Duplicate identifier: rename one declaration." :
                available ? "Font loaded" : "Font unavailable: check path/face and console, then Reload fonts.");
            field(*fonts, "remove:" + std::to_string(index), [&](auto& value)
            {
                if (ui::Button::button("Remove font"))
                {
                    auto& declarations = std::get<FontDeclarations>(value);
                    if (index < declarations.size())
                    {
                        declarations.erase(declarations.begin() + index);
                        return true;
                    }
                }
                return false;
            });
        }
        field(*fonts, "add", [&](auto& value)
        {
            if (ui::Button::button("Add bundled Didact Gothic font"))
            {
                auto& declarations = std::get<FontDeclarations>(value);
                std::string identifier = "default";
                for (unsigned suffix = 1; registered.contains(identifier); ++suffix)
                {
                    identifier = "font" + std::to_string(suffix);
                }
                declarations.push_back({identifier, base::PlatformTools::assetPath() /
                    "fonts/didact-gothic/DidactGothic-Regular.ttf", 0, {}});
                return true;
            }
            return false;
        });
        ui::Text::text("Add multiple entries, then choose other font files and unique identifiers as needed.");
        ui::Text::text("Verify font redistribution rights. Supplied notices are retained when changing the font file.");
    }
    if (node.getComponent<scene::TextComponent>() || node.getComponent<scene::TextFontComponent>())
    {
        if (ui::Button::button("Reload fonts / retry text"))
        {
            delegate->reloadTextFonts();
        }
    }
}
catch (...)
{
    if (!_textUiWarning)
    {
        std::fprintf(stderr, "Text/font inspector unavailable; authored scene data retained.\n");
        _textUiWarning = true;
    }
}

}
