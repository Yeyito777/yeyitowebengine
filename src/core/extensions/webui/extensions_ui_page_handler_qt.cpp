// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "extensions_ui_page_handler_qt.h"

#include "extensions/extension_system_qt.h"
#include "extensions/webui/select_file_dialog.h"

#include "extensions/browser/extension_registry.h"
#include "extensions/extension_manager.h"
#include "api/qwebengineextensionmanager.h"
#include "profile_adapter.h"
#include "profile_qt.h"
#include "type_conversion.h"

ExtensionsUIPageHandlerQt::ExtensionsUIPageHandlerQt(
        content::WebUI *webui, Profile *profile,
        mojo::PendingReceiver<qtwebengine::mojom::PageHandler> receiver,
        mojo::PendingRemote<qtwebengine::mojom::Page> page)
    : receiver_(this, std::move(receiver)), page_(std::move(page)), webui_(webui), profile_(profile)
{
}

ExtensionsUIPageHandlerQt::~ExtensionsUIPageHandlerQt() { }

void ExtensionsUIPageHandlerQt::GetAllExtensionInfo(GetAllExtensionInfoCallback callback)
{
    std::vector<qtwebengine::mojom::ExtensionInfoPtr> extensionsInfo;

    auto add_to_list = [&extensionsInfo](const extensions::ExtensionSet &extensions,
                                         bool is_enabled) {
        for (auto extension : extensions) {
            auto info = qtwebengine::mojom::ExtensionInfo::New();
            info->name = extension->name();
            info->description = extension->description();
            info->version = extension->VersionString();
            info->id = extension->id();
            info->isEnabled = is_enabled;
            extensionsInfo.push_back(std::move(info));
        }
    };

    extensions::ExtensionRegistry *registry = extensions::ExtensionRegistry::Get(profile_);
    add_to_list(registry->enabled_extensions(), true);
    add_to_list(registry->disabled_extensions(), false);

    std::move(callback).Run(std::move(extensionsInfo));
}

void ExtensionsUIPageHandlerQt::LoadExtension()
{
    SelectFileDialog::Show(
            base::BindOnce(&ExtensionsUIPageHandlerQt::InnerLoadExtension, base::Unretained(this)),
            base::FilePath(), webui_->GetWebContents());
}

void ExtensionsUIPageHandlerQt::InnerLoadExtension(const base::FilePath &path)
{
    auto profileAdapter = static_cast<QtWebEngineCore::ProfileQt *>(profile_)->profileAdapter();
    QWebEngineExtensionManager *manager = profileAdapter->extensionManager();

    QMetaObject::Connection *const connection = new QMetaObject::Connection;
    *connection = QObject::connect(manager, &QWebEngineExtensionManager::extensionLoadFinished,
                                   [this, connection]() {
                                       page_->ReloadPage();
                                       QObject::disconnect(*connection);
                                       delete connection;
                                   });
    manager->loadExtension(QtWebEngineCore::toQt(path));
}
