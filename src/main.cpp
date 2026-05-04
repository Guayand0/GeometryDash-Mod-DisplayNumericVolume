#include <Geode/Bindings.hpp>
#include <Geode/modify/GameOptionsLayer.hpp>
#include <Geode/modify/OptionsLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace geode::prelude;

namespace {
    constexpr char const* kMusicLabelID = "guayand0.display-numeric-volume/music-label";
    constexpr char const* kSfxLabelID = "guayand0.display-numeric-volume/sfx-label";

    int getDecimalPlaces() {
        return std::clamp(Mod::get()->getSettingValue<int64_t>("decimal-places"), 0ll, 3ll);
    }

    std::string formatSliderValue(Slider* slider) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(getDecimalPlaces())
               << (slider ? slider->getValue() * 100.f : 0.f);
        return stream.str();
    }

    void collectSliders(cocos2d::CCNode* node, std::vector<Slider*>& out) {
        if (!node) {
            return;
        }

        if (auto* slider = typeinfo_cast<Slider*>(node)) {
            out.push_back(slider);
        }

        auto* children = node->getChildren();
        if (!children) {
            return;
        }

        for (auto* child : geode::cocos::CCArrayExt<cocos2d::CCNode*>(children)) {
            collectSliders(child, out);
        }
    }

    void collectLabels(cocos2d::CCNode* node, std::vector<CCLabelBMFont*>& out) {
        if (!node) {
            return;
        }

        if (auto* label = typeinfo_cast<CCLabelBMFont*>(node)) {
            out.push_back(label);
        }

        auto* children = node->getChildren();
        if (!children) {
            return;
        }

        for (auto* child : geode::cocos::CCArrayExt<cocos2d::CCNode*>(children)) {
            collectLabels(child, out);
        }
    }

    std::string labelText(CCLabelBMFont* label) {
        if (!label || !label->getString()) {
            return "";
        }

        return label->getString();
    }

    bool startsWith(std::string const& value, std::string_view prefix) {
        return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix.data()) == 0;
    }

    CCLabelBMFont* findAudioLabel(cocos2d::CCNode* root, char const* id, std::string_view baseText) {
        if (!root) {
            return nullptr;
        }

        if (auto* byID = typeinfo_cast<CCLabelBMFont*>(root->getChildByID(id))) {
            return byID;
        }

        std::vector<CCLabelBMFont*> labels;
        collectLabels(root, labels);

        for (auto* label : labels) {
            auto const text = labelText(label);
            if (text == baseText || startsWith(text, std::string(baseText) + ":")) {
                label->setID(id);
                return label;
            }
        }

        return nullptr;
    }

    void sortTopToBottom(std::vector<Slider*>& sliders) {
        std::sort(sliders.begin(), sliders.end(), [](Slider* a, Slider* b) {
            return a->getParent()->convertToWorldSpace(a->getPosition()).y >
                b->getParent()->convertToWorldSpace(b->getPosition()).y;
        });
    }

    template <class Fields>
    void setupAudioDisplay(cocos2d::CCNode* root, Fields* fields) {
        if (!root || !fields || !Mod::get()->getSettingValue<bool>("mod-enabled")) {
            return;
        }

        std::vector<Slider*> sliders;
        collectSliders(root, sliders);
        if (sliders.size() < 2) {
            log::warn("Couldn't find both audio sliders, found {}", sliders.size());
            return;
        }

        sortTopToBottom(sliders);
        fields->musicSlider = sliders[0];
        fields->sfxSlider = sliders[1];
        fields->musicLabel = findAudioLabel(root, kMusicLabelID, "Music");
        fields->sfxLabel = findAudioLabel(root, kSfxLabelID, "SFX");
    }

    template <class Fields>
    void refreshAudioDisplay(Fields* fields) {
        if (!fields || !Mod::get()->getSettingValue<bool>("mod-enabled")) {
            return;
        }

        if (fields->musicLabel) {
            fields->musicLabel->setString(fmt::format("Music: {}", formatSliderValue(fields->musicSlider)).c_str());
        }

        if (fields->sfxLabel) {
            fields->sfxLabel->setString(fmt::format("SFX: {}", formatSliderValue(fields->sfxSlider)).c_str());
        }
    }

    template <class Fields>
    void disableAudioDisplay(Fields* fields) {
        if (!fields) {
            return;
        }

        if (fields->musicLabel) {
            fields->musicLabel->setString("Music");
        }

        if (fields->sfxLabel) {
            fields->sfxLabel->setString("SFX");
        }
    }
}

class $modify(VolumeNumberingPauseLayer, PauseLayer) {
    struct Fields {
        Slider* musicSlider = nullptr;
        Slider* sfxSlider = nullptr;
        CCLabelBMFont* musicLabel = nullptr;
        CCLabelBMFont* sfxLabel = nullptr;
    };

    void customSetup() {
        PauseLayer::customSetup();
        auto* fields = m_fields.operator->();
        setupAudioDisplay(this, fields);
        refreshAudioDisplay(fields);
        this->schedule(schedule_selector(VolumeNumberingPauseLayer::updateValueLabels));
    }

    void musicSliderChanged(CCObject* sender) {
        PauseLayer::musicSliderChanged(sender);
        refreshAudioDisplay(m_fields.operator->());
    }

    void sfxSliderChanged(CCObject* sender) {
        PauseLayer::sfxSliderChanged(sender);
        refreshAudioDisplay(m_fields.operator->());
    }

    void updateValueLabels(float) {
        auto* fields = m_fields.operator->();
        if (Mod::get()->getSettingValue<bool>("mod-enabled")) {
            refreshAudioDisplay(fields);
        }
        else {
            disableAudioDisplay(fields);
        }
    }
};

class $modify(VolumeNumberingGameOptionsLayer, GameOptionsLayer) {
    struct Fields {
        Slider* musicSlider = nullptr;
        Slider* sfxSlider = nullptr;
        CCLabelBMFont* musicLabel = nullptr;
        CCLabelBMFont* sfxLabel = nullptr;
    };

    void setupOptions() {
        GameOptionsLayer::setupOptions();
        auto* fields = m_fields.operator->();
        setupAudioDisplay(this, fields);
        refreshAudioDisplay(fields);
        this->schedule(schedule_selector(VolumeNumberingGameOptionsLayer::updateValueLabels));
    }

    void updateValueLabels(float) {
        auto* fields = m_fields.operator->();
        if (Mod::get()->getSettingValue<bool>("mod-enabled")) {
            refreshAudioDisplay(fields);
        }
        else {
            disableAudioDisplay(fields);
        }
    }
};

class $modify(VolumeNumberingOptionsLayer, OptionsLayer) {
    struct Fields {
        CCLabelBMFont* musicLabel = nullptr;
        CCLabelBMFont* sfxLabel = nullptr;
    };

    void customSetup() {
        OptionsLayer::customSetup();

        auto* fields = m_fields.operator->();
        fields->musicLabel = findAudioLabel(this, kMusicLabelID, "Music");
        fields->sfxLabel = findAudioLabel(this, kSfxLabelID, "SFX");

        if (Mod::get()->getSettingValue<bool>("mod-enabled")) {
            if (fields->musicLabel) {
                fields->musicLabel->setString(fmt::format("Music: {}", formatSliderValue(m_musicSlider)).c_str());
            }
            if (fields->sfxLabel) {
                fields->sfxLabel->setString(fmt::format("SFX: {}", formatSliderValue(m_sfxSlider)).c_str());
            }
        }

        this->schedule(schedule_selector(VolumeNumberingOptionsLayer::updateValueLabels));
    }

    void musicSliderChanged(CCObject* sender) {
        OptionsLayer::musicSliderChanged(sender);
        this->updateValueLabels(0.f);
    }

    void sfxSliderChanged(CCObject* sender) {
        OptionsLayer::sfxSliderChanged(sender);
        this->updateValueLabels(0.f);
    }

    void updateValueLabels(float) {
        auto* fields = m_fields.operator->();
        if (Mod::get()->getSettingValue<bool>("mod-enabled")) {
            if (fields->musicLabel) {
                fields->musicLabel->setString(fmt::format("Music: {}", formatSliderValue(m_musicSlider)).c_str());
            }
            if (fields->sfxLabel) {
                fields->sfxLabel->setString(fmt::format("SFX: {}", formatSliderValue(m_sfxSlider)).c_str());
            }
        }
        else {
            if (fields->musicLabel) {
                fields->musicLabel->setString("Music");
            }
            if (fields->sfxLabel) {
                fields->sfxLabel->setString("SFX");
            }
        }
    }
};
