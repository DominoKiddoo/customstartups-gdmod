#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <prevter.imageplus/include/api.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

auto totalFrames = 0;



class $modify(CustomStartupsLayer, LoadingLayer) {
    static void onModify(auto& self) {
        if (!self.setHookPriorityPost("LoadingLayer::init", Priority::Late)) {
            log::error("failed to set hook priority :c");
        }
    }

    bool init(bool refresh) {
        if (!LoadingLayer::init(refresh)) return false;

        auto path = Mod::get()->getSettingValue<std::filesystem::path>("startup-image");
        std::string pathStr = utils::string::pathToString(path);

        auto animVideo = imgp::AnimatedSprite::create(pathStr.c_str());


        auto usingCustomSounds = Mod::get()->getSettingValue<bool>("custom-sounds");
        auto inbuiltSound = Mod::get()->getSettingValue<std::string>("inbuilt-sound");
        if (!animVideo) {
            log::error("Failed to load sprite from: {}", pathStr);
            return true;
        }

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        animVideo->setPosition(winSize / 2);
        
        OverlayManager::get()->addChild(animVideo);

        animVideo->play(); 
        animVideo->setScale(2.5f);
        log::info("Successfully added startup animation: {}", pathStr);
        totalFrames = animVideo->getFrameCount();

        animVideo->setID("AnimVideo");
        OverlayManager::get()->schedule(schedule_selector(CustomStartupsLayer::checkIfDone));
        log::info("Total frames in animation: {}", totalFrames);

        auto uploadedSound = Mod::get()->getSettingValue<std::filesystem::path>("startup-sound");

        FMODAudioEngine::get()->playEffect(geode::utils::string::pathToString(uploadedSound).c_str());

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
            overlayManager->removeChild(animVideo);
            overlayManager->unschedule(schedule_selector(CustomStartupsLayer::checkIfDone));
        }
    }
    
};

class $modify(CustomStartUpsMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        FMODAudioEngine::get()->stopAllMusic(true);
        return true;
    }

};
