#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <prevter.imageplus/include/api.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

auto totalFrames = 0;
auto originalVolume = 0.f;


class $modify(CustomStartupsLayer, LoadingLayer) {

    bool init(bool refresh) {
        if (!LoadingLayer::init(refresh)) return false;

        auto path = Mod::get()->getSettingValue<std::filesystem::path>("startup-image");
        std::string pathStr = utils::string::pathToString(path);

        auto animVideo = imgp::AnimatedSprite::create(pathStr.c_str());


        
        if (!animVideo) {
            // log::error("Failed to load sprite from: {}", pathStr);
            return true;
        }



        auto winSize = CCDirector::sharedDirector()->getWinSize();
        animVideo->setPosition(winSize / 2);
        
        OverlayManager::get()->addChild(animVideo);
        animVideo->pause();

        if (Mod::get()->getSettingValue<std::string>("wul-type") == "black" && Mod::get()->getSettingValue<bool>("wait-until-load")) {
            auto blackLayer = CCLayerColor::create({0, 0, 0, 255});
            blackLayer->setID("blackOverlay");
            blackLayer->ignoreAnchorPointForPosition(false);
            blackLayer->setAnchorPoint({0.5f, 0.5f});

            OverlayManager::get()->addChild(blackLayer);
            blackLayer->setContentSize(winSize);
            blackLayer->setPosition(winSize / 2);
            
        }

        auto waitForLoad = Mod::get()->getSettingValue<bool>("wait-until-load");
        auto uploadedSound = Mod::get()->getSettingValue<std::filesystem::path>("startup-sound");

        if (!waitForLoad) {
            animVideo->play();
            FMODAudioEngine::get()->playEffect(geode::utils::string::pathToString(uploadedSound).c_str());
        }
        

        animVideo->setScale(Mod::get()->getSettingValue<float>("vid-scale"));

        log::info("Successfully added startup animation: {}", pathStr);
        totalFrames = animVideo->getFrameCount();

        animVideo->setID("AnimVideo");
        OverlayManager::get()->schedule(schedule_selector(CustomStartupsLayer::checkIfDone));
        log::info("Total frames in animation: {}", totalFrames);
    

        return true;
    }

    void checkIfDone(float dt) {
        auto overlayManager = OverlayManager::get();

        auto animVideo = static_cast<imgp::AnimatedSprite*>(overlayManager->getChildByID("AnimVideo"));

        if (!animVideo) {
            log::info("Failed to find video");
            return;
        }

        //log::info("total frames: {}, current frame: {}", totalFrames, animVideo->getCurrentFrame());
        if (animVideo->getCurrentFrame() >= totalFrames - 1) {
            animVideo->stop();
            overlayManager->unschedule(schedule_selector(CustomStartupsLayer::checkIfDone));

            auto audioEngine = FMODAudioEngine::sharedEngine();

            audioEngine->m_musicVolume = originalVolume;
            audioEngine->m_backgroundMusicChannel->setVolume(originalVolume);

            auto fDuration = Mod::get()->getSettingValue<float>("fade-out-duration");
            auto fadeOut = Mod::get()->getSettingValue<bool>("fade-out");
            if (fadeOut == true) {
                animVideo->runAction(CCSequence::create(
                CCFadeOut::create(fDuration),
                CallFuncExt::create([overlayManager, animVideo]() {
                    overlayManager->removeChild(animVideo);
                }),
                nullptr
             ));
            } else {
                animVideo->runAction(CCSequence::create(
                    CallFuncExt::create([overlayManager, animVideo]() {
                        overlayManager->removeChild(animVideo);
                    }),
                    nullptr));
            }  
            

        }
    }
    
};

class $modify(CustomStartUpsMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto overlayManager = OverlayManager::get();
        auto animVideo = static_cast<imgp::AnimatedSprite*>(overlayManager->getChildByID("AnimVideo"));

        if (!animVideo) {
            log::info("Failed to find video");
            return true;
        }
        
        auto waitForLoad = Mod::get()->getSettingValue<bool>("wait-until-load");

        auto audioEngine = FMODAudioEngine::sharedEngine();

        originalVolume = audioEngine->m_musicVolume;

        audioEngine->m_musicVolume = 0.0f;
        audioEngine->m_backgroundMusicChannel->setVolume(0.0f);


        auto uploadedSound = Mod::get()->getSettingValue<std::filesystem::path>("startup-sound");

        if (waitForLoad && animVideo) {
            animVideo->play();
            FMODAudioEngine::get()->playEffect(geode::utils::string::pathToString(uploadedSound).c_str());
            if (Mod::get()->getSettingValue<std::string>("wul-type") == "black") {
                auto blackOverlay = typeinfo_cast<CCLayerColor*>(overlayManager->getChildByID("blackOverlay"));
                overlayManager->removeChild(blackOverlay);
            }
        }

        return true;


    }

};
