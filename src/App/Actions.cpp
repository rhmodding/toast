#include "Actions.hpp"

#include <cstdlib>

#include <filesystem>

#include <string>

#include <thread>

#include <tinyfiledialogs.h>
#include <imgui.h>

#include "manager/SessionManager.hpp"
#include "manager/AsyncTaskManager.hpp"

#include "task/AsyncTaskPushSession.hpp"
#include "task/AsyncTaskExportSession.hpp"

#include "Macro.hpp"

namespace Actions {

void CreateSessionPromptPath() {
    const char* filterPatterns[] = { "*.szs", "*.zlib" };
    char* path = tinyfd_openFileDialog(
        "Select a cellanim file",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Cellanim Archive Files",
        false
    );

    if (!path)
        return;

    AsyncTaskManager::getInstance().StartTask<AsyncTaskPushSession>(std::string(path));
}

void ExportSessionPromptPath() {
    auto& sessionManager = SessionManager::getInstance();
    auto& asyncTaskManager = AsyncTaskManager::getInstance();

    if (!sessionManager.getSessionAvaliable() || asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>())
        return;

    const bool isRvl = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL;

    const char* filterPatterns[] = { isRvl ? "*.szs" : "*.zlib" };
    const char* filterDesc = isRvl ? "RVL Cellanim Archive files" : "CTR Cellanim Archive files";

    char* savePath = tinyfd_saveFileDialog(
        "Select a file to save to",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        filterDesc
    );

    if (savePath) {
        asyncTaskManager.StartTask<AsyncTaskExportSession>(
            sessionManager.getCurrentSessionIndex(),
            std::string(savePath)
        );
    }
}

void ExportSession() {
    auto& sessionManager = SessionManager::getInstance();
    auto& asyncTaskManager = AsyncTaskManager::getInstance();

    if (!sessionManager.getSessionAvaliable() || asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>())
        return;

    asyncTaskManager.StartTask<AsyncTaskExportSession>(
        sessionManager.getCurrentSessionIndex()
    );
}

// --- Internal helper for format conversion ---
namespace {
    TPL::TPLImageFormat ConvertToTPL(CTPK::CTPKImageFormat format) {
        using namespace CTPK;
        using namespace TPL;

        switch (format) {
            case CTPK_IMAGE_FORMAT_RGBA8888:
            case CTPK_IMAGE_FORMAT_RGB888:
            case CTPK_IMAGE_FORMAT_RGBA4444:
                return TPL_IMAGE_FORMAT_RGBA32;
            case CTPK_IMAGE_FORMAT_RGB565:
                return TPL_IMAGE_FORMAT_RGB565;
            case CTPK_IMAGE_FORMAT_LA88:
                return TPL_IMAGE_FORMAT_IA8;
            case CTPK_IMAGE_FORMAT_LA44:
                return TPL_IMAGE_FORMAT_IA4;
            case CTPK_IMAGE_FORMAT_L8:
                return TPL_IMAGE_FORMAT_I8;
            default:
                return TPL_IMAGE_FORMAT_RGB5A3;
        }
    }

    CTPK::CTPKImageFormat ConvertToCTPK(TPL::TPLImageFormat format) {
        using namespace TPL;
        using namespace CTPK;

        switch (format) {
            case TPL_IMAGE_FORMAT_RGBA32:
                return CTPK_IMAGE_FORMAT_RGBA8888;
            case TPL_IMAGE_FORMAT_IA8:
                return CTPK_IMAGE_FORMAT_LA88;
            case TPL_IMAGE_FORMAT_IA4:
                return CTPK_IMAGE_FORMAT_LA44;
            case TPL_IMAGE_FORMAT_I8:
                return CTPK_IMAGE_FORMAT_L8;
            default:
                return CTPK_IMAGE_FORMAT_ETC1A4;
        }
    }
}

void ExportSessionAsOther() {
    auto& sessionManager = SessionManager::getInstance();
    auto& asyncTaskManager = AsyncTaskManager::getInstance();

    if (!sessionManager.getSessionAvaliable() || asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>())
        return;

    auto* session = sessionManager.getCurrentSession();
    const bool isRvl = session->type == CellAnim::CELLANIM_TYPE_RVL;

    const char* filterDesc = isRvl ? "CTR Cellanim Archive files" : "RVL Cellanim Archive files";
    const char* filterPatterns[] = { isRvl ? "*.zlib" : "*.szs" };

    char* savePath = tinyfd_saveFileDialog(
        "Select a file to save to",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        filterDesc
    );

    if (!savePath)
        return;

    CellAnim::CellAnimType newType = isRvl ? CellAnim::CELLANIM_TYPE_CTR : CellAnim::CELLANIM_TYPE_RVL;

    auto& sheets = session->sheets->getVector();
    for (size_t i = 0; i < sheets.size(); ++i) {
        auto& sheet = sheets[i];

        if (isRvl) {
            for (const auto& cellanim : session->cellanims) {
                if (cellanim.object->getSheetIndex() == i)
                    sheet->setName(cellanim.name);
            }
            sheet->setTPLOutputFormat(ConvertToTPL(sheet->getCTPKOutputFormat()));
        }
        else {
            sheet->setCTPKOutputFormat(ConvertToCTPK(sheet->getTPLOutputFormat()));
        }
    }

    session->type = newType;

    for (auto& cellanim : session->cellanims) {
        cellanim.object->setType(newType);

        for (auto& arrangement : cellanim.object->getArrangements()) {
            for (auto& part : arrangement.parts) {
                if (isRvl) {
                    part.textureVarying = 0;
                }
                else {
                    part.backColor = {0.f, 0.f, 0.f};
                    part.foreColor = {1.f, 1.f, 1.f};
                    part.quadDepth = {};
                    part.id = 0;
                }
            }
        }

        if (!isRvl) {
            for (auto& animation : cellanim.object->getAnimations()) {
                animation.isInterpolated = false;
                for (auto& key : animation.keys) {
                    key.translateZ = 0.f;
                    key.backColor = {0.f, 0.f, 0.f};
                    key.foreColor = {1.f, 1.f, 1.f};
                }
            }
        }
    }

    asyncTaskManager.StartTask<AsyncTaskExportSession>(
        sessionManager.getCurrentSessionIndex(),
        std::string(savePath)
    );
}

void OpenSessionSourceFolder() {
    const auto& filePath = SessionManager::getInstance().getCurrentSession()->resourcePath;
    std::string folderPath = std::filesystem::absolute(filePath).parent_path().string();

#if defined(_WIN32)
    std::string command = "explorer \"" + folderPath + "\"";
#elif defined(__APPLE__)
    std::string command = "open \"" + folderPath + "\"";
#else
    std::string command = "xdg-open \"" + folderPath + "\"";
#endif

    std::thread([command] { std::system(command.c_str()); }).detach();
}

} // namespace Actions
