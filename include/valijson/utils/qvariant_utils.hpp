#pragma once

#include <QFile>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, QVariant &root)
{
    // Load schema JSON from file
    QFile file(QString::fromStdString(path));
    if (!file.open(QFile::ReadOnly)) {
        std::cerr << "Failed to load json from file '" << path << "'." << std::endl;
        return false;
    }

    QByteArray data = file.readAll();

    // Parse schema
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        std::cerr << "qt failed to parse the document:" << std::endl
                  << parseError.errorString().toStdString() << std::endl;
        return false;
    } else if (doc.isObject() || doc.isArray()) {
        root = QVariant(doc.toVariant() );
    } else if (doc.isEmpty()) {
        root = QVariant();
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
