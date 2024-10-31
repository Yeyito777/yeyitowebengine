// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "unpacked_extension_installer.h"

#include "base/files/file_util.h"

namespace QtWebEngineCore {
UnpackedExtensionInstaller::UnpackedExtensionInstaller(
        const scoped_refptr<base::SequencedTaskRunner> &taskRunner, DoneCallback doneCallback)
    : m_taskRunner(taskRunner), m_doneCallback(std::move(doneCallback))
{
}

// static
scoped_refptr<UnpackedExtensionInstaller>
UnpackedExtensionInstaller::Create(const scoped_refptr<base::SequencedTaskRunner> &taskRunner,
                                   DoneCallback doneCallback)
{
    return base::WrapRefCounted(
            new UnpackedExtensionInstaller(taskRunner, std::move(doneCallback)));
}

// static
UnpackedExtensionInstaller::InstallInfo
UnpackedExtensionInstaller::installUnpackedExtensionOnFileThread(const base::FilePath &src,
                                                                 const base::FilePath &installDir)
{
    InstallInfo installInfo;
    if (!base::DirectoryExists(installDir)) {
        if (!base::CreateDirectory(installDir)) {
            installInfo.error = "Install directory does not exists";
            return installInfo;
        }
    }

    // The installed dir format is `dirName_XXXXXX` where `XXXXXX` is populated with
    // mktemp() logic to match the output format of the zip installer.
    base::FilePath extensionInstallPath;
    base::FilePath::StringType dirName = src.BaseName().value() + FILE_PATH_LITERAL("_");
    if (!base::CreateTemporaryDirInDir(installDir, dirName, &extensionInstallPath)) {
        installInfo.error = "Failed to create install directory for extension";
        return installInfo;
    }

    installInfo.extensionInstallPath = extensionInstallPath;

    // This performs a 'cp -r src installDir/', we need to rename the copied directory later.
    if (!base::CopyDirectory(src, installDir, true)) {
        installInfo.error = "Copy directory failed";
        return installInfo;
    }

    auto copyPath = installDir.Append(src.BaseName());
    CHECK(base::DirectoryExists(copyPath));

    if (!base::Move(copyPath, extensionInstallPath))
        installInfo.error = "Move directory failed";
    return installInfo;
}

void UnpackedExtensionInstaller::install(const base::FilePath &src,
                                         const base::FilePath &installDir)
{
    // Verify the extension before doing any file operations by preloading it.
    m_taskRunner->PostTaskAndReplyWithResult(
            FROM_HERE,
            base::BindOnce(&QtWebEngineCore::ExtensionLoader::loadExtensionOnFileThread, src),
            base::BindOnce(&UnpackedExtensionInstaller::installInternal, this, src, installDir));
}

void UnpackedExtensionInstaller::installInternal(const base::FilePath &src,
                                                 const base::FilePath &installDir,
                                                 const ExtensionLoader::LoadingInfo &loadingInfo)
{
    if (!loadingInfo.error.empty()) {
        std::move(m_doneCallback).Run(src, installDir, loadingInfo.error);
        return;
    }

    m_taskRunner->PostTaskAndReplyWithResult(
            FROM_HERE, base::BindOnce(installUnpackedExtensionOnFileThread, src, installDir),
            base::BindOnce(&UnpackedExtensionInstaller::installDone, this, src));
}

void UnpackedExtensionInstaller::installDone(const base::FilePath &src,
                                             const InstallInfo &installInfo)
{
    std::move(m_doneCallback).Run(src, installInfo.extensionInstallPath, installInfo.error);
}

} // namespace QtWebEngineCore
