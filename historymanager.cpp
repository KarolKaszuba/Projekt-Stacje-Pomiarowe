#include "historymanager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QUuid>

/**
 * @brief Konstruktor klasy HistoryManager.
 *
 * Inicjalizuje obiekt HistoryManager, ustawia ścieżkę katalogu historii i zapewnia jego istnienie.
 *
 * @param storagePath Ścieżka do katalogu przechowywania danych historii.
 * @param parent Wskaźnik na obiekt nadrzędny.
 */
HistoryManager::HistoryManager(const QString &storagePath, QObject *parent)
    : QObject(parent), m_historyDir(storagePath) {
    ensureHistoryDir();
    m_indexFilePath = m_historyDir.filePath("history_index.json");
}

/**
 * @brief Dodaje nową sesję do historii.
 *
 * Tworzy plik JSON dla sesji i aktualizuje plik indeksu. Sesja zawiera dane takie jak identyfikator,
 * lokalizacja, współrzędne, promień i lista stacji.
 *
 * @param sessionId Unikalny identyfikator sesji.
 * @param location Nazwa lokalizacji.
 * @param radius Promień wyszukiwania w kilometrach.
 * @param latitude Szerokość geograficzna.
 * @param longitude Długość geograficzna.
 * @param stations Lista stacji jako QVariantList.
 */
void HistoryManager::addSession(const QString &sessionId, const QString &location, double radius, double latitude, double longitude, const QVariantList &stations) {
    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString sessionFile = QString("session_%1.json").arg(sessionId);

    // Create session data
    QVariantMap sessionData;
    sessionData["session_id"] = sessionId;
    sessionData["timestamp"] = timestamp;
    sessionData["location"] = QVariantMap{
        {"input", location},
        {"latitude", latitude},
        {"longitude", longitude}
    };
    sessionData["radius"] = radius;
    sessionData["stations"] = stations;
    sessionData["sensors"] = QVariantList(); // Initialize empty sensors list

    // Write session file
    QJsonDocument doc(QJsonObject::fromVariantMap(sessionData));
    QFile file(m_historyDir.filePath(sessionFile));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Wrote session file:" << sessionFile;
    } else {
        qDebug() << "Failed to write session file:" << sessionFile << "Error:" << file.errorString();
    }

    // Update index
    QVariantMap indexEntry;
    indexEntry["session_id"] = sessionId;
    indexEntry["timestamp"] = timestamp;
    indexEntry["location"] = location;
    indexEntry["radius"] = radius;
    indexEntry["file"] = sessionFile;
    updateIndexFile(indexEntry);
}

/**
 * @brief Dodaje sensory do istniejącej sesji.
 *
 * Wczytuje istniejące dane sesji, dodaje nowe sensory (unikając duplikatów) i zapisuje zaktualizowane dane.
 *
 * @param sessionId Identyfikator sesji.
 * @param sensors Lista sensorów jako QList<QVariantMap>.
 */
void HistoryManager::addSessionSensors(const QString &sessionId, const QList<QVariantMap> &sensors) {
    QString sessionFile = QString("session_%1.json").arg(sessionId);
    QFile file(m_historyDir.filePath(sessionFile));

    try {
        // Read existing session data
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to read session file:" << sessionFile << "Error:" << file.errorString();
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Failed to parse session file JSON:" << sessionFile;
            return;
        }

        QVariantMap sessionData = doc.object().toVariantMap();
        QVariantList existingSensors = sessionData["sensors"].toList();

        // Create a set of existing sensor IDs to avoid duplicates
        QSet<int> existingSensorIds;
        for (const QVariant &sensorVariant : existingSensors) {
            QVariantMap sensor = sensorVariant.toMap();
            existingSensorIds.insert(sensor["id"].toInt());
        }

        // Convert input sensors to the desired structure and append non-duplicates
        QVariantList newSensors = existingSensors;
        for (const QVariantMap &sensor : sensors) {
            int sensorId = sensor["id"].toInt();
            if (!existingSensorIds.contains(sensorId)) {
                QVariantMap sensorEntry = sensor;
                sensorEntry["measurements"] = QVariantList();
                newSensors.append(sensorEntry);
                existingSensorIds.insert(sensorId);
            }
        }

        // Update session data with the merged sensors list
        sessionData["sensors"] = newSensors;

        // Write updated session data
        QJsonDocument updatedDoc(QJsonObject::fromVariantMap(sessionData));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qDebug() << "Failed to open session file for writing:" << sessionFile << "Error:" << file.errorString();
            return;
        }

        qint64 bytesWritten = file.write(updatedDoc.toJson());
        file.close();

        if (bytesWritten == -1) {
            qDebug() << "Failed to write updated session file:" << sessionFile << "Error:" << file.errorString();
        } else {
            qDebug() << "Successfully updated session file:" << sessionFile << "with" << newSensors.size() << "sensors";
        }
    } catch (const std::exception &e) {
        qDebug() << "Exception in addSessionSensors for session" << sessionId << ":" << e.what();
        // Continue without crashing; session file remains unchanged
    } catch (...) {
        qDebug() << "Unknown exception in addSessionSensors for session" << sessionId;
        // Continue without crashing
    }
}

/**
 * @brief Dodaje pomiary do sensorów w sesji.
 *
 * Wczytuje dane sesji, organizuje pomiary według identyfikatorów sensorów i aktualizuje dane sesji.
 *
 * @param sessionId Identyfikator sesji.
 * @param measurements Lista pomiarów jako QList<QVariantMap>.
 */
void HistoryManager::addSessionMeasurements(const QString &sessionId, const QList<QVariantMap> &measurements) {
    QString sessionFile = QString("session_%1.json").arg(sessionId);
    QFile file(m_historyDir.filePath(sessionFile));

    try {
        // Read existing session data
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to read session file:" << sessionFile << "Error:" << file.errorString();
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Failed to parse session file JSON:" << sessionFile;
            return;
        }

        QVariantMap sessionData = doc.object().toVariantMap();
        QVariantList sensors = sessionData["sensors"].toList();

        // Organize measurements by sensorId
        QMap<int, QVariantList> measurementsBySensor;
        for (const QVariantMap &measurement : measurements) {
            int sensorId = measurement["sensorId"].toInt();
            QVariantMap measurementEntry;
            measurementEntry["date"] = measurement["date"];
            measurementEntry["value"] = measurement["value"];
            measurementsBySensor[sensorId].append(measurementEntry);
        }

        // Update measurements for each sensor
        bool updated = false;
        for (QVariant &sensorVariant : sensors) {
            QVariantMap sensor = sensorVariant.toMap();
            int sensorId = sensor["id"].toInt();
            if (measurementsBySensor.contains(sensorId)) {
                QVariantList existingMeasurements = sensor["measurements"].toList();
                existingMeasurements.append(measurementsBySensor[sensorId]);
                sensor["measurements"] = existingMeasurements;
                sensorVariant = sensor;
                updated = true;
            }
        }

        if (updated) {
            sessionData["sensors"] = sensors;

            // Write updated session data
            QJsonDocument updatedDoc(QJsonObject::fromVariantMap(sessionData));
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qDebug() << "Failed to open session file for writing:" << sessionFile << "Error:" << file.errorString();
                return;
            }

            qint64 bytesWritten = file.write(updatedDoc.toJson());
            file.close();

            if (bytesWritten == -1) {
                qDebug() << "Failed to write updated session file:" << sessionFile << "Error:" << file.errorString();
            } else {
                qDebug() << "Successfully updated session file:" << sessionFile << "with measurements for" << measurementsBySensor.size() << "sensors";
            }
        } else {
            qDebug() << "No sensors matched the provided measurements for session:" << sessionId;
        }
    } catch (const std::exception &e) {
        qDebug() << "Exception in addSessionMeasurements for session" << sessionId << ":" << e.what();
        // Continue without crashing; session file remains unchanged
    } catch (...) {
        qDebug() << "Unknown exception in addSessionMeasurements for session" << sessionId;
        // Continue without crashing
    }
}

/**
 * @brief Dodaje dane o jakości powietrza do sesji.
 *
 * Wczytuje dane sesji, dodaje informacje o jakości powietrza i zapisuje zaktualizowane dane.
 *
 * @param sessionId Identyfikator sesji.
 * @param airQualityData Dane o jakości powietrza jako QVariantMap.
 */
void HistoryManager::addSessionAirQuality(const QString &sessionId, const QVariantMap &airQualityData) {
    QString sessionFile = QString("session_%1.json").arg(sessionId);
    QFile file(m_historyDir.filePath(sessionFile));

    try {
        // Read existing session data
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to read session file:" << sessionFile << "Error:" << file.errorString();
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Failed to parse session file JSON:" << sessionFile;
            return;
        }

        QVariantMap sessionData = doc.object().toVariantMap();
        sessionData["airQuality"] = airQualityData;

        // Write updated session data
        QJsonDocument updatedDoc(QJsonObject::fromVariantMap(sessionData));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qDebug() << "Failed to open session file for writing:" << sessionFile << "Error:" << file.errorString();
            return;
        }

        qint64 bytesWritten = file.write(updatedDoc.toJson());
        file.close();

        if (bytesWritten == -1) {
            qDebug() << "Failed to write updated session file:" << sessionFile << "Error:" << file.errorString();
        } else {
            qDebug() << "Successfully updated session file:" << sessionFile << "with air quality data";
        }
    } catch (const std::exception &e) {
        qDebug() << "Exception in addSessionAirQuality for session" << sessionId << ":" << e.what();
        // Continue without crashing; session file remains unchanged
    } catch (...) {
        qDebug() << "Unknown exception in addSessionAirQuality for session" << sessionId;
        // Continue without crashing
    }
}

/**
 * @brief Aktualizuje plik indeksu sesji.
 *
 * Dodaje nową sesję do indeksu i usuwa najstarszą, jeśli limit sesji został przekroczony.
 *
 * @param session Dane sesji jako QVariantMap.
 */
void HistoryManager::updateIndexFile(const QVariantMap &session) {
    QVariantList sessions;
    QFile file(m_indexFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isNull() && doc.isObject()) {
            sessions = doc.object()["sessions"].toVariant().toList();
        }
        file.close();
    }

    sessions.prepend(session);
    if (sessions.size() > MAX_SESSIONS) {
        QVariantMap oldSession = sessions.takeLast().toMap();
        QString oldFile = oldSession["file"].toString();
        if (m_historyDir.remove(oldFile)) {
            qDebug() << "Removed old session file:" << oldFile;
        } else {
            qDebug() << "Failed to remove old session file:" << oldFile;
        }
    }

    QJsonObject indexObj;
    indexObj["sessions"] = QJsonValue::fromVariant(sessions);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(indexObj).toJson());
        file.close();
        qDebug() << "Updated index file:" << m_indexFilePath;
    } else {
        qDebug() << "Failed to write index file:" << m_indexFilePath << "Error:" << file.errorString();
    }
}

/**
 * @brief Wczytuje listę sesji z pliku indeksu.
 *
 * @return QVariantList zawierający listę sesji.
 */
QVariantList HistoryManager::loadSessions() const {
    QFile file(m_indexFilePath);
    try {
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to read index file:" << m_indexFilePath << "Error:" << file.errorString();
            return QVariantList();
        }
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Failed to parse index file JSON:" << m_indexFilePath;
            return QVariantList();
        }
        return doc.object()["sessions"].toVariant().toList();
    } catch (const std::exception &e) {
        qDebug() << "Exception in loadSessions:" << e.what();
        return QVariantList();
    } catch (...) {
        qDebug() << "Unknown exception in loadSessions";
        return QVariantList();
    }
}

/**
 * @brief Wczytuje szczegóły sesji z pliku sesji.
 *
 * @param sessionId Identyfikator sesji.
 * @return QVariantMap zawierający szczegóły sesji.
 */
QVariantMap HistoryManager::loadSessionDetails(const QString &sessionId) const {
    QString sessionFile = QString("session_%1.json").arg(sessionId);
    QFile file(m_historyDir.filePath(sessionFile));
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to read session file:" << sessionFile << "Error:" << file.errorString();
        return QVariantMap();
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    return doc.object().toVariantMap();
}

/**
 * @brief Zapewnia istnienie katalogu historii.
 *
 * Tworzy katalog, jeśli nie istnieje, i loguje wynik operacji.
 */
void HistoryManager::ensureHistoryDir() {
    if (!m_historyDir.exists()) {
        if (!m_historyDir.mkpath(".")) {
            qDebug() << "Failed to create history directory:" << m_historyDir.path();
        } else {
            qDebug() << "Created history directory:" << m_historyDir.path();
        }
    }
}

/**
 * @brief Generuje unikalny identyfikator sesji.
 *
 * @return QString zawierający UUID bez nawiasów.
 */
QString HistoryManager::generateSessionId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
