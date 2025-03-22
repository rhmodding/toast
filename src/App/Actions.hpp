#ifndef ACTIONS_HPP
#define ACTIONS_HPP

#include <tinyfiledialogs.h>

#include <imgui.h>

#include "../SessionManager.hpp"

#include "../task/AsyncTaskManager.hpp"
#include "../task/AsyncTaskPushSession.hpp"
#include "../task/AsyncTaskExportSession.hpp"

#include <cstdlib>

#include <string>

#include <thread>

#include <filesystem>

#include "../common.hpp"

namespace Actions {

void CreateSessionPromptPath() {
    const char* filterPatterns[] { "*.szs", "*.zlib" };
    char* openFileDialog = tinyfd_openFileDialog(
        "Select a cellanim file",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Cellanim Archive Files",
        false
    );

    if (!openFileDialog)
        return;

    AsyncTaskManager::getInstance().StartTask<AsyncTaskPushSession>(
        std::string(openFileDialog)
    );
}

void ExportSessionPromptPath() {
    SessionManager& sessionManager = SessionManager::getInstance();
    AsyncTaskManager& asyncTaskManager = AsyncTaskManager::getInstance();

    if (
        !sessionManager.getSessionAvaliable() ||
        asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>()
    )
        return;

    const bool isRvl = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL;

    const char* filterPatterns[] { isRvl ? "*.szs" : "*.zlib" };
    const char* filterDesc = isRvl ? "RVL Cellanim Archive files" : "CTR Cellanim Archive files";

    char* saveFileDialog = tinyfd_saveFileDialog(
        "Select a file to save to",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        filterDesc
    );

    if (saveFileDialog)
        asyncTaskManager.StartTask<AsyncTaskExportSession>(
            sessionManager.getCurrentSessionIndex(),
            std::string(saveFileDialog)
        );
}

void ExportSession() {
    SessionManager& sessionManager = SessionManager::getInstance();
    AsyncTaskManager& asyncTaskManager = AsyncTaskManager::getInstance();

    if (
        !sessionManager.getSessionAvaliable() ||
        asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>()
    )
        return;

    asyncTaskManager.StartTask<AsyncTaskExportSession>(
        sessionManager.getCurrentSessionIndex()
    );
}

void ExportSessionAsOther() {
    SessionManager& sessionManager = SessionManager::getInstance();
    AsyncTaskManager& asyncTaskManager = AsyncTaskManager::getInstance();
    
    if (
        !sessionManager.getSessionAvaliable() ||
        asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>()
    )
    return;
    
    const bool isRvl = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL;
    const char* filterDesc = isRvl ? "CTR Cellanim Archive files" : "RVL Cellanim Archive files";

    const char* filterPatterns[] { isRvl ? "*.zlib" : "*.szs" };
    char* saveFileDialog = tinyfd_saveFileDialog(
        "Select a file to save to",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        filterDesc
    );

    const CellAnim::CellAnimType newType = isRvl ? CellAnim::CELLANIM_TYPE_CTR : CellAnim::CELLANIM_TYPE_RVL;

    if (saveFileDialog) {
        auto* currentSession = sessionManager.getCurrentSession();

        auto& sheets = currentSession->sheets->getVector();
        
        for (unsigned i = 0; i < sheets.size(); i++) {
            const auto& sheet = sheets[i];

            if (isRvl) {
                for (const auto& cellanim : currentSession->cellanims) {
                    if (cellanim.object->sheetIndex == i)
                        sheet->setName(cellanim.name);
                }

                const CTPK::CTPKImageFormat ctpkFormat = sheet->getCTPKOutputFormat();
                TPL::TPLImageFormat tplFormat;

                switch (ctpkFormat) {
                case CTPK::CTPK_IMAGE_FORMAT_RGBA8888:
                case CTPK::CTPK_IMAGE_FORMAT_RGB888:
                case CTPK::CTPK_IMAGE_FORMAT_RGBA4444:
                    tplFormat = TPL::TPL_IMAGE_FORMAT_RGBA32;
                    break;

                case CTPK::CTPK_IMAGE_FORMAT_RGB565:
                    tplFormat = TPL::TPL_IMAGE_FORMAT_RGB565;
                    break;

                case CTPK::CTPK_IMAGE_FORMAT_LA88:
                    tplFormat = TPL::TPL_IMAGE_FORMAT_IA8;
                    break;
                case CTPK::CTPK_IMAGE_FORMAT_LA44:
                    tplFormat = TPL::TPL_IMAGE_FORMAT_IA4;
                    break;

                case CTPK::CTPK_IMAGE_FORMAT_L8:
                    tplFormat = TPL::TPL_IMAGE_FORMAT_I8;
                    break;
                
                default:
                    tplFormat = TPL::TPL_IMAGE_FORMAT_RGB5A3;
                    break;
                }

                sheet->setTPLOutputFormat(tplFormat);
            }
            else {
                const TPL::TPLImageFormat tplFormat = sheet->getTPLOutputFormat();
                CTPK::CTPKImageFormat ctpkFormat;

                switch (tplFormat) {
                case TPL::TPL_IMAGE_FORMAT_RGBA32:
                    ctpkFormat = CTPK::CTPK_IMAGE_FORMAT_RGBA8888;
                    break;
                
                case TPL::TPL_IMAGE_FORMAT_IA8:
                    ctpkFormat = CTPK::CTPK_IMAGE_FORMAT_LA88;
                    break;
                case TPL::TPL_IMAGE_FORMAT_IA4:
                    ctpkFormat = CTPK::CTPK_IMAGE_FORMAT_LA44;
                    break;

                case TPL::TPL_IMAGE_FORMAT_I8:
                    ctpkFormat = CTPK::CTPK_IMAGE_FORMAT_L8;
                    break;

                default:
                    ctpkFormat = CTPK::CTPK_IMAGE_FORMAT_ETC1A4;
                    break;
                }

                sheet->setCTPKOutputFormat(ctpkFormat);
            }
        }

        currentSession->type = newType;

        for (auto& cellanim : currentSession->cellanims) {
            cellanim.object->setType(newType);

            for (auto& arrangement : cellanim.object->arrangements) {
                for (auto& part : arrangement.parts) {
                    if (isRvl) {
                        part.textureVarying = 0;
                    }
                    else {
                        part.backColor = CellAnim::CTRColor { 0.f, 0.f, 0.f };
                        part.foreColor = CellAnim::CTRColor { 1.f, 1.f, 1.f };

                        part.quadDepth = CellAnim::CTRQuadDepth {};

                        part.id = 0;
                    }
                }
            }

            if (!isRvl) {
                for (auto& animation : cellanim.object->animations) {
                    animation.isInterpolated = false;
                    
                    for (auto& key : animation.keys) {
                        key.translateZ = 0.f;

                        key.backColor = CellAnim::CTRColor { 0.f, 0.f, 0.f };
                        key.foreColor = CellAnim::CTRColor { 1.f, 1.f, 1.f };
                    }
                }
            }
        }

        asyncTaskManager.StartTask<AsyncTaskExportSession>(
            sessionManager.getCurrentSessionIndex(),
            std::string(saveFileDialog)
        );
    }
}

void OpenSessionSourceFolder() {
    const auto& filePath = SessionManager::getInstance().getCurrentSession()->resourcePath;

    std::string folderPath = std::filesystem::absolute(filePath).parent_path();

#if defined(__WIN32__)
    std::string command = "explorer \"" + folderPath + "\"";
#elif defined(__APPLE__)
    std::string command = "open \"" + folderPath + "\"";
#else // Probably Linux.
    std::string command = "xdg-open \"" + folderPath + "\"";
#endif // defined(__WIN32__), defined(__APPLE__)

    std::thread([command]() {
        std::system(command.c_str());
    }).detach();
}

} // namespace Actions

#endif // ACTIONS_HPP
