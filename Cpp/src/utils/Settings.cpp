#include "utils/Settings.h"
#include "utils/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QStandardPaths>

namespace ImageCut {
namespace Utils {

Settings& Settings::getInstance() {
    static Settings instance;
    return instance;
}

Settings::Settings() {
    m_configPath = getDefaultConfigPath();

    // Default settings
    m_data["ai_model"] = "RMBG-1.4";
    m_data["ai_device"] = "Auto";
    m_data["inference_size"] = 512;
    m_data["theme"] = "Dark";
    m_data["checkerboard_size"] = 16;
    m_data["export_format"] = "PNG";
    m_data["export_quality"] = 95;
    m_data["export_folder"] = "";
    m_data["max_undo_steps"] = 30;

    load();
}

std::string Settings::getDefaultConfigPath() const {
    QString homeDir = QDir::homePath();
    QDir configDir(homeDir + "/.imagecut");
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    return configDir.filePath("settings.json").toStdString();
}

void Settings::load(const std::string& customPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!customPath.empty()) {
        m_configPath = customPath;
    }

    QFile file(QString::fromStdString(m_configPath));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray content = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(content);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                m_data[it.key()] = it.value();
            }
            LOG_INFO("Settings loaded from: " + m_configPath);
        }
    }
}

void Settings::save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    QFile file(QString::fromStdString(m_configPath));
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_data);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        LOG_INFO("Settings saved to: " + m_configPath);
    }
}

QString Settings::get(const QString& key, const QString& defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data.contains(key)) {
        return m_data[key].toString(defaultValue);
    }
    return defaultValue;
}

int Settings::getInt(const QString& key, int defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data.contains(key)) {
        return m_data[key].toInt(defaultValue);
    }
    return defaultValue;
}

double Settings::getDouble(const QString& key, double defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data.contains(key)) {
        return m_data[key].toDouble(defaultValue);
    }
    return defaultValue;
}

bool Settings::getBool(const QString& key, bool defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data.contains(key)) {
        return m_data[key].toBool(defaultValue);
    }
    return defaultValue;
}

void Settings::set(const QString& key, const QString& value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data[key] = value;
    }
    save();
}

void Settings::setInt(const QString& key, int value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data[key] = value;
    }
    save();
}

void Settings::setDouble(const QString& key, double value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data[key] = value;
    }
    save();
}

void Settings::setBool(const QString& key, bool value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data[key] = value;
    }
    save();
}

} // namespace Utils
} // namespace ImageCut
