#ifndef IMAGECUT_SETTINGS_H
#define IMAGECUT_SETTINGS_H

#include <string>
#include <map>
#include <mutex>
#include <QString>
#include <QJsonObject>

namespace ImageCut {
namespace Utils {

class Settings {
public:
    static Settings& getInstance();

    void load(const std::string& customPath = "");
    void save();

    QString get(const QString& key, const QString& defaultValue = "") const;
    int getInt(const QString& key, int defaultValue = 0) const;
    double getDouble(const QString& key, double defaultValue = 0.0) const;
    bool getBool(const QString& key, bool defaultValue = false) const;

    void set(const QString& key, const QString& value);
    void setInt(const QString& key, int value);
    void setDouble(const QString& key, double value);
    void setBool(const QString& key, bool value);

private:
    Settings();
    ~Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    std::string getDefaultConfigPath() const;

    mutable std::mutex m_mutex;
    std::string m_configPath;
    QJsonObject m_data;
};

} // namespace Utils
} // namespace ImageCut

#endif // IMAGECUT_SETTINGS_H
