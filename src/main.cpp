#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

namespace {
    int g_stage = 1; // 0 = OFF, 1..4 = presets
    bool g_enabled = true;
    int g_savedTicks = 0;
    std::vector<std::string> g_console = {
        "[InputBuffer] Setting Mode: STAGE 1 (2 All / 2 Wave)",
        "[InputBuffer] Press F6 in-level to cycle: 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF",
        "[InputBuffer] F7 toggles the buffer prototype; F8 opens this console."
    };

    struct StagePreset { int all; int wave; };
    StagePreset presetFor(int stage) {
        switch (stage) {
            case 1: return {2, 2};
            case 2: return {4, 4};
            case 3: return {8, 8};
            case 4: return {16, 8};
            default: return {0, 0};
        }
    }

    void logLine(std::string const& line) {
        g_console.push_back("[InputBuffer] " + line);
        if (g_console.size() > 70) g_console.erase(g_console.begin(), g_console.begin() + 10);
        log::info("[InputBuffer] {}", line);
    }

    void cycleStage() {
        g_stage = (g_stage == 4) ? 0 : g_stage + 1;
        g_enabled = g_stage != 0;
        if (g_stage == 0) {
            logLine("F6 Cycle -> OFF (100% Raw Input)");
        } else {
            auto p = presetFor(g_stage);
            logLine(fmt::format("F6 Cycle -> STAGE {}: {} Ticks All | {} Ticks Wave",
                                g_stage, p.all, p.wave));
        }
    }

    void toggleBuffer() {
        g_enabled = !g_enabled;
        if (g_enabled) {
            if (g_stage == 0) g_stage = 1;
            logLine(fmt::format("Buffer prototype ENABLED (Stage {})", g_stage));
        } else {
            logLine("Buffer prototype DISABLED (100% Raw Input)");
        }
    }

    class BufferPopup : public Popup<> {
    protected:
        bool setup() override {
            this->setTitle("Geode Predictive Input Buffer");
            auto size = this->m_mainLayer->getContentSize();

            auto subtitle = CCLabelBMFont::create(
                "LIVE CONSOLE  |  STANDALONE INPUT-BUFFER PROTOTYPE", "bigFont.fnt");
            subtitle->setScale(0.28f);
            subtitle->setColor({150, 220, 210});
            subtitle->setPosition({size.width / 2, size.height - 38.f});
            this->m_mainLayer->addChild(subtitle);

            auto status = CCLabelBMFont::create("", "bigFont.fnt");
            status->setScale(0.38f);
            status->setColor({235, 235, 235});
            status->setPosition({size.width / 2, size.height - 62.f});
            status->setID("buffer-status");
            this->m_mainLayer->addChild(status);

            auto menu = CCMenu::create();
            menu->setPosition({size.width / 2, size.height - 102.f});
            this->m_mainLayer->addChild(menu);

            addButton(menu, "Save +1 tick", -110.f, 0.f, menu_selector(BufferPopup::onSave1));
            addButton(menu, "Save +2 ticks", 0.f, 0.f, menu_selector(BufferPopup::onSave2));
            addButton(menu, "Save +3 ticks", 110.f, 0.f, menu_selector(BufferPopup::onSave3));

            addButton(menu, "Apply preset", -75.f, -35.f, menu_selector(BufferPopup::onApply));
            addButton(menu, "Clear console", 75.f, -35.f, menu_selector(BufferPopup::onClear));

            auto hint = CCLabelBMFont::create(
                "F6: next stage   F7: toggle   F8: reopen console", "bigFont.fnt");
            hint->setScale(0.32f);
            hint->setColor({170, 170, 170});
            hint->setPosition({size.width / 2, 35.f});
            this->m_mainLayer->addChild(hint);

            refreshStatus();
            refreshConsole();
            return true;
        }

        void addButton(CCMenu* menu, char const* title, float x, float y, SEL_MenuHandler selector) {
            auto sprite = ButtonSprite::create(title, 105, true, "goldFont.fnt", "GJ_button_01.png", 28.f, 0.55f);
            auto item = CCMenuItemSpriteExtra::create(sprite, this, selector);
            item->setPosition({x, y});
            menu->addChild(item);
        }

        void refreshStatus() {
            auto label = typeinfo_cast<CCLabelBMFont*>(this->m_mainLayer->getChildByID("buffer-status"));
            if (!label) return;
            if (!g_enabled || g_stage == 0) {
                label->setString("STATUS: OFF  |  Raw input mode");
            } else {
                auto p = presetFor(g_stage);
                label->setString(fmt::format("STATUS: STAGE {}  |  All: {} ticks  |  Wave: {} ticks",
                                             g_stage, p.all, p.wave).c_str());
            }
        }

        void refreshConsole() {
            if (auto old = this->m_mainLayer->getChildByID("buffer-console")) old->removeFromParent();
            auto size = this->m_mainLayer->getContentSize();
            std::string contents;
            size_t start = g_console.size() > 8 ? g_console.size() - 8 : 0;
            for (size_t i = start; i < g_console.size(); ++i) {
                contents += g_console[i];
                if (i + 1 < g_console.size()) contents += "\n";
            }
            auto label = CCLabelBMFont::create(contents.c_str(), "chatFont.fnt");
            label->setAnchorPoint({0.f, 1.f});
            label->setAlignment(kCCTextAlignmentLeft);
            label->setScale(0.36f);
            label->setColor({210, 210, 210});
            label->setPosition({18.f, size.height - 148.f});
            label->setID("buffer-console");
            this->m_mainLayer->addChild(label);
        }

        void doSave(int amount) {
            g_savedTicks += amount;
            logLine(fmt::format("*** SAVED +{} TICKS ({} total saved) PRESS! (Frame0: 0 -> {})",
                                amount, g_savedTicks, g_enabled ? "Delayed" : "Immediate"));
            refreshStatus();
            refreshConsole();
        }

        void onSave1(CCObject*) { doSave(1); }
        void onSave2(CCObject*) { doSave(2); }
        void onSave3(CCObject*) { doSave(3); }
        void onApply(CCObject*) {
            if (g_stage == 0) g_stage = 1;
            g_enabled = true;
            auto p = presetFor(g_stage);
            logLine(fmt::format("Preset applied: Stage {} ({} All / {} Wave)", g_stage, p.all, p.wave));
            refreshStatus();
            refreshConsole();
        }
        void onClear(CCObject*) {
            g_console.clear();
            logLine("Console cleared");
            refreshConsole();
        }

    public:
        static BufferPopup* create() {
            auto ret = new BufferPopup();
            if (ret->initAnchored(360.f, 290.f)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }
    };

    void openConsole() {
        if (auto popup = BufferPopup::create()) popup->show();
    }
}

$on_game(Loaded) {
    listenForKeybindSettingPresses("cycle-stage", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) cycleStage();
    });
    listenForKeybindSettingPresses("toggle-buffer", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) toggleBuffer();
    });
    listenForKeybindSettingPresses("open-console", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) openConsole();
    });
}
