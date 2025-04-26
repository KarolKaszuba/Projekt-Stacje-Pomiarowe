#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QDir>
#include <QVariantList>
#include <QVariantMap>

/**
 * @class HistoryManager
 * @brief Klasa zarządzająca historią sesji aplikacji.
 *
 * Klasa HistoryManager odpowiada za zapisywanie, wczytywanie i aktualizowanie danych sesji,
 * takich jak informacje o stacjach pomiarowych, sensorach, pomiarach i jakości powietrza.
 * Dane są przechowywane w plikach JSON w określonym katalogu.
 */
class HistoryManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Konstruktor klasy HistoryManager.
     * @param storagePath Ścieżka do katalogu, w którym przechowywane są dane historii.
     * @param parent Wskaźnik na obiekt nadrzędny (domyślnie nullptr).
     */
    explicit HistoryManager(const QString &storagePath, QObject *parent = nullptr);

    /**
     * @brief Generuje unikalny identyfikator sesji.
     * @return QString zawierający UUID sesji bez nawiasów.
     */
    QString generateSessionId() const;

    /**
     * @brief Dodaje nową sesję do historii.
     * @param sessionId Unikalny identyfikator sesji.
     * @param location Nazwa lokalizacji (np. miasto lub adres).
     * @param radius Promień wyszukiwania w kilometrach.
     * @param latitude Szerokość geograficzna lokalizacji.
     * @param longitude Długość geograficzna lokalizacji.
     * @param stations Lista stacji pomiarowych jako QVariantList.
     */
    void addSession(const QString &sessionId, const QString &location, double radius, double latitude, double longitude, const QVariantList &stations);

    /**
     * @brief Dodaje sensory do istniejącej sesji, unikając duplikatów.
     * @param sessionId Identyfikator sesji.
     * @param sensors Lista sensorów jako QList<QVariantMap>.
     */
    void addSessionSensors(const QString &sessionId, const QList<QVariantMap> &sensors);

    /**
     * @brief Dodaje pomiary do sensorów w istniejącej sesji.
     * @param sessionId Identyfikator sesji.
     * @param measurements Lista pomiarów jako QList<QVariantMap>.
     */
    void addSessionMeasurements(const QString &sessionId, const QList<QVariantMap> &measurements);

    /**
     * @brief Dodaje dane o jakości powietrza do sesji.
     * @param sessionId Identyfikator sesji.
     * @param airQualityData Dane o jakości powietrza jako QVariantMap.
     */
    void addSessionAirQuality(const QString &sessionId, const QVariantMap &airQualityData);

    /**
     * @brief Wczytuje listę zapisanych sesji.
     * @return QVariantList zawierający dane sesji.
     */
    QVariantList loadSessions() const;

    /**
     * @brief Wczytuje szczegóły konkretnej sesji.
     * @param sessionId Identyfikator sesji.
     * @return QVariantMap zawierający szczegóły sesji.
     */
    QVariantMap loadSessionDetails(const QString &sessionId) const;

    /**
     * @brief Katalog przechowujący pliki historii.
     */
    QDir m_historyDir;

private:
    /**
     * @brief Zapewnia istnienie katalogu historii.
     *
     * Tworzy katalog, jeśli nie istnieje.
     */
    void ensureHistoryDir();

    /**
     * @brief Aktualizuje plik indeksu sesji.
     * @param session Dane sesji do dodania do indeksu jako QVariantMap.
     */
    void updateIndexFile(const QVariantMap &session);

    /**
     * @brief Ścieżka do pliku indeksu historii.
     */
    QString m_indexFilePath;

    /**
     * @brief Maksymalna liczba przechowywanych sesji.
     */
    static const int MAX_SESSIONS = 100;
};

#endif // HISTORYMANAGER_H
