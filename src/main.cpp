#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <prevter.imageplus/include/api.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
using namespace geode::prelude;


// make button


class CustomStartupsLayer;

auto preTestVolume = 1.0f;
auto cRunning = false;

class ButtonSettingV3 : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json) {
        auto res = std::make_shared<ButtonSettingV3>();
        auto root = checkJson(json, "buttonsettingv3");

        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);
        
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const& json) override {
        return true;
    }
    bool save(matjson::Value& json) const override {
        return true;
    }

    bool isDefaultValue() const override {
        return true;
    }
    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};

class ButtonSettingNodeV3 : public SettingNodeV3 {
protected:
    ButtonSprite* m_buttonSprite;
    CCMenuItemSpriteExtra* m_button;

    bool init(std::shared_ptr<ButtonSettingV3> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;
        
        m_buttonSprite = ButtonSprite::create("Test", "goldFont.fnt", "GJ_button_01.png", .8f);
        m_buttonSprite->setScale(.5f);

        m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite, this, menu_selector(ButtonSettingNodeV3::onButton)
        );

        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->setContentWidth(60);
        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);
        
        return true;
    }
    
    void updateState(CCNode* invoker) override {
        SettingNodeV3::updateState(invoker);
        auto shouldEnable = this->getSetting()->shouldEnable();
        m_button->setEnabled(shouldEnable);
        m_buttonSprite->setCascadeColorEnabled(true);
        m_buttonSprite->setCascadeOpacityEnabled(true);
        m_buttonSprite->setOpacity(shouldEnable ? 255 : 155);
        m_buttonSprite->setColor(shouldEnable ? ccWHITE : ccGRAY);
    }

    void onButton(CCObject*) {
        auto path = Mod::get()->getSettingValue<std::filesystem::path>("startup-image");
        std::string pathStr = utils::string::pathToString(path);

        if (pathStr.empty() || !std::filesystem::exists(path)) {
            log::warn("startup image path is empty or file does not exist!");
            return; 
        }
        if (cRunning) return;
        cRunning = true;

        auto animVideo = imgp::AnimatedSprite::create(pathStr.c_str());
        if (!animVideo) {
            cRunning = false;
            return;
        }
        

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        animVideo->setPosition(winSize / 2);

        auto contentSize = animVideo->getContentSize();

        bool stretchToFit = Mod::get()->getSettingValue<bool>("stretch-to-fit");

        if (stretchToFit) {
            float scaleX = winSize.width / contentSize.width;
            float scaleY = winSize.height / contentSize.height;
            
            animVideo->setScaleX(scaleX);
            animVideo->setScaleY(scaleY);
        } else {
            float customScale = Mod::get()->getSettingValue<double>("vid-scale");
            animVideo->setScale(customScale);
        }
        auto uploadedSound = Mod::get()->getSettingValue<std::filesystem::path>("startup-sound");
        
        animVideo->setID("dominodev.custom-startups/AnimVideo");
        
        OverlayManager::get()->addChild(animVideo);

        if (!uploadedSound.empty()) {
            FMODAudioEngine::get()->playEffect(geode::utils::string::pathToString(uploadedSound).c_str());
        }


        OverlayManager::get()->schedule(schedule_selector(ButtonSettingNodeV3::checkIfDoneForSettings)); 

        auto audioEngine = FMODAudioEngine::get();

        preTestVolume = audioEngine->m_musicVolume;

        audioEngine->m_musicVolume = 0.0f;
        if (audioEngine->m_backgroundMusicChannel) {
            audioEngine->m_backgroundMusicChannel->setVolume(0.0f);
        }
    }

    void checkIfDoneForSettings(float dt) {
        auto overlayManager = OverlayManager::get();
        auto animVideo = static_cast<imgp::AnimatedSprite*>(overlayManager->getChildByID("dominodev.custom-startups/AnimVideo"));

        if (!animVideo) {
            overlayManager->unschedule(schedule_selector(ButtonSettingNodeV3::checkIfDoneForSettings));
            return;
        }

        auto earlyFrames = 0;
        if (Mod::get()->getSettingValue<bool>("fade-out-early")) {
            
            earlyFrames = Mod::get()->getSettingValue<int>("fade-out-early-frames");
        }

        if (animVideo->getCurrentFrame() >= animVideo->getFrameCount() - (1 + earlyFrames)) {
            animVideo->stop();
            overlayManager->unschedule(schedule_selector(ButtonSettingNodeV3::checkIfDoneForSettings));

            auto fDuration = Mod::get()->getSettingValue<float>("fade-out-duration");
            auto fadeOut = Mod::get()->getSettingValue<bool>("fade-out");

            auto restoreVolumeAndCleanup = CallFuncExt::create([overlayManager, animVideo]() {
                auto audioEngine = FMODAudioEngine::sharedEngine();
                
                audioEngine->m_musicVolume = preTestVolume;
                if (audioEngine->m_backgroundMusicChannel) {
                    audioEngine->m_backgroundMusicChannel->setVolume(preTestVolume);
                }
                overlayManager->removeChild(animVideo);
                cRunning = false;
            });

            if (fadeOut) {
                animVideo->runAction(CCSequence::create(
                    CCFadeOut::create(fDuration),
                    restoreVolumeAndCleanup,
                    nullptr
                ));
            } else {
                animVideo->runAction(restoreVolumeAndCleanup);
            }  
        }
    }


void onCommit() override {}
void onResetToDefault() override {}

public:
    static ButtonSettingNodeV3* create(std::shared_ptr<ButtonSettingV3> setting, float width) {
        auto ret = new ButtonSettingNodeV3();
        if (ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool hasUncommittedChanges() const override { return false; }
    bool hasNonDefaultValue() const override { return false; }

    std::shared_ptr<ButtonSettingV3> getSetting() const {
        return std::static_pointer_cast<ButtonSettingV3>(SettingNodeV3::getSetting());
    }
};

SettingNodeV3* ButtonSettingV3::createNode(float width) {
    return ButtonSettingNodeV3::create(
        std::static_pointer_cast<ButtonSettingV3>(shared_from_this()),
        width
    );
}

$execute {
    (void)Mod::get()->registerCustomSettingType("button-setting", &ButtonSettingV3::parse);
}


// other things that arent buttons 🤯🤯🤯


class $modify(CustomStartupsLayer, LoadingLayer) {
    struct Fields {
        int lastFrame = -1;
    };

    bool init(bool refresh) {
        if (!LoadingLayer::init(refresh)) return false;

        auto path = Mod::get()->getSettingValue<std::filesystem::path>("startup-image");
        std::string pathStr = utils::string::pathToString(path);

        if (pathStr.empty() || !std::filesystem::exists(path)) {
            log::warn("startup image path is empty or file does not exist!");
            return true;
        }


        auto animVideo = imgp::AnimatedSprite::create(pathStr.c_str());
        if (!animVideo || pathStr.empty()) return true;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        animVideo->setPosition(winSize / 2);
        OverlayManager::get()->addChild(animVideo);

        bool waitLoad = Mod::get()->getSettingValue<bool>("wait-until-load");
        std::string wulType = Mod::get()->getSettingValue<std::string>("wul-type");
        
        if (wulType == "nothing" && waitLoad) animVideo->setVisible(false);
        animVideo->pause();

        if (wulType == "black" && waitLoad) {
            auto blackLayer = CCLayerColor::create({0, 0, 0, 255});
            blackLayer->setID("dominodev.custom-startups/blackOverlay");
            blackLayer->ignoreAnchorPointForPosition(false);
            blackLayer->setAnchorPoint({0.5f, 0.5f});
            OverlayManager::get()->addChild(blackLayer);
            blackLayer->setContentSize(winSize);
            blackLayer->setPosition(winSize / 2);
        }

        if (!waitLoad) {
            auto audioEngine = FMODAudioEngine::sharedEngine();
            audioEngine->m_musicVolume = 0.0f;
            if (audioEngine->m_backgroundMusicChannel) audioEngine->m_backgroundMusicChannel->setVolume(0.0f);
            
            animVideo->play();
            auto sound = Mod::get()->getSettingValue<std::filesystem::path>("startup-sound");

            if (!sound.empty()) {
                FMODAudioEngine::get()->playEffect(geode::utils::string::pathToString(sound).c_str());
            }

            
        }

        auto contentSize = animVideo->getContentSize();
        if (Mod::get()->getSettingValue<bool>("stretch-to-fit")) {
            animVideo->setScaleX(winSize.width / contentSize.width);
            animVideo->setScaleY(winSize.height / contentSize.height);
        } else {
            animVideo->setScale(Mod::get()->getSettingValue<double>("vid-scale"));
        }

        animVideo->setID("dominodev.custom-startups/AnimVideo");
        animVideo->schedule(schedule_selector(CustomStartupsLayer::checkIfDone));
        
        // log::info("Total frames {}", animVideo->getFrameCount());
    
        return true;
    }

    void checkIfDone(float dt) {
        auto overlayManager = OverlayManager::get();
        auto animVideo = static_cast<imgp::AnimatedSprite*>(overlayManager->getChildByID("dominodev.custom-startups/AnimVideo"));

        if (!animVideo) return;

        int earlyFrames = 0;
        if (Mod::get()->getSettingValue<bool>("fade-out-early")) {
            earlyFrames = Mod::get()->getSettingValue<int>("fade-out-early-frames");
        }

        int currentFrame = animVideo->getCurrentFrame();
        int frameCount = animVideo->getFrameCount();
        if (frameCount <= 0) return;

        int targetFrame = frameCount - (1 + earlyFrames);

        bool reachedTarget = currentFrame >= targetFrame;
        bool loopedBack = (m_fields->lastFrame != -1 && currentFrame < m_fields->lastFrame);

        if (reachedTarget || loopedBack) {
            animVideo->unschedule(schedule_selector(CustomStartupsLayer::checkIfDone));
            animVideo->stop();

            auto fDuration = Mod::get()->getSettingValue<float>("fade-out-duration");
            auto fadeOut = Mod::get()->getSettingValue<bool>("fade-out");

            auto restoreVolumeAndCleanup = CallFuncExt::create([overlayManager, animVideo]() {
                auto audioEngine = FMODAudioEngine::sharedEngine();
                auto trueVolume = GameManager::get()->m_bgVolume; 
                
                audioEngine->m_musicVolume = trueVolume;
                if (audioEngine->m_backgroundMusicChannel) {
                    audioEngine->m_backgroundMusicChannel->setVolume(trueVolume);
                }
                overlayManager->removeChild(animVideo);
            });

            if (fadeOut) {
                animVideo->runAction(CCSequence::create(
                    CCFadeOut::create(fDuration),
                    restoreVolumeAndCleanup,
                    nullptr
                ));
            } else {
                animVideo->runAction(restoreVolumeAndCleanup);
            }  
        } else {
            m_fields->lastFrame = currentFrame;
        }
    }
};

class $modify(CustomStartUpsMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto audioEngine = FMODAudioEngine::sharedEngine();

        
        preTestVolume = audioEngine->m_musicVolume;
        auto overlayManager = OverlayManager::get();
        auto animVideo = static_cast<imgp::AnimatedSprite*>(overlayManager->getChildByID("dominodev.custom-startups/AnimVideo"));

        if (!animVideo) {
            // log::info("No video found.");
            auto shownWelcome = Mod::get()->getSavedValue<bool>("shown-welcome");
            if (!shownWelcome) { 
                this->runAction(CCSequence::create(
                    CCDelayTime::create(0.5f),
                    CallFuncExt::create([this]() {
                        auto helpPopup = geode::createQuickPopup(
                            "CustomStartups",
                            "Thank you for installing <co>CustomStartups</c>!\nRead the mod description for a guide. And <cr>look at the settings</c> for <cg>a bunch of important customisation options</c>.",
                            "Don't show again.", nullptr,
                            [this](auto, bool btn1) {
                                if (!btn1) {
                                    // log::info("dont show again");
                                    Mod::get()->setSavedValue<bool>("shown-welcome", true);
                                } else {
                                    // log::info("closed");
                                }
                            }
                        );
                        
                    }),
                    nullptr
                ));
            }
            return true;
        }
        
    


        auto waitForLoad = Mod::get()->getSettingValue<bool>("wait-until-load");
       
        auto uploadedSound = Mod::get()->getSettingValue<std::filesystem::path>("startup-sound");

        audioEngine->m_musicVolume = 0.0f;

        if (Mod::get()->getSettingValue<std::string>("wul-type") == "nothing") {
            animVideo->setVisible(true);
        }

        if (waitForLoad) {
            if (animVideo->getCurrentFrame() <= 1) { 
                auto audioEngine = FMODAudioEngine::sharedEngine();
                auto uploadedSound = Mod::get()->getSettingValue<std::filesystem::path>("startup-sound");

                audioEngine->m_musicVolume = 0.0f;
                if (audioEngine->m_backgroundMusicChannel) {
                    audioEngine->m_backgroundMusicChannel->setVolume(0.0f);
                }

                animVideo->setVisible(true); 
                animVideo->play();
                if (!geode::utils::string::pathToString(uploadedSound).empty()) {
                    FMODAudioEngine::get()->playEffect(geode::utils::string::pathToString(uploadedSound).c_str());
                }
                
            }

            if (auto blackOverlay = overlayManager->getChildByID("dominodev.custom-startups/blackOverlay")) {
                overlayManager->removeChild(blackOverlay);
            }
        }

        return true;
    }
};