#ifndef WINDOW_2_DATA_VIS_H
#define WINDOW_2_DATA_VIS_H
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QCalendarWidget>
#include <QDate>
#include <QMap>
#include <QVector>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCore/qjsonobject.h>
#include "historymanager.h"

namespace Ui {
class window_2_data_vis;
}

/**
 * @class window_2_data_vis
 * @brief Okno do wizualizacji danych pomiarowych ze stacji.
 *
 * Klasa zarządza wyświetlaniem danych z sensorów, wykresów i informacji o jakości powietrza
 * dla wybranej stacji pomiarowej.
 */
class window_2_data_vis : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Konstruktor klasy window_2_data_vis.
     * @param stationId Identyfikator stacji pomiarowej.
     * @param historyManager Wskaźnik na menedżera historii.
     * @param sessionId Identyfikator sesji.
     * @param parent Wskaźnik na widget nadrzędny (domyślnie nullptr).
     */
    explicit window_2_data_vis(int stationId, HistoryManager *historyManager, const QString &sessionId, QWidget *parent = nullptr);

    /**
     * @brief Destruktor klasy window_2_data_vis.
     */
    ~window_2_data_vis();

private slots:
    /**
     * @brief Obsługuje odpowiedź sieciową dla żądania sensorów.
     * @param reply Wskaźnik na odpowiedź sieciową.
     */
    void onSensorReply(QNetworkReply *reply);

    /**
     * @brief Obsługuje odpowiedź sieciową dla żądania pomiarów.
     * @param reply Wskaźnik na odpowiedź sieciową.
     */
    void onMeasurementReply(QNetworkReply *reply);

    /**
     * @brief Obsługuje odpowiedź sieciową dla żądania jakości powietrza.
     * @param reply Wskaźnik na odpowiedź sieciową.
     */
    void onAirQualityReply(QNetworkReply *reply);

    /**
     * @brief Obsługuje kliknięcie daty w kalendarzu.
     * @param date Wybrana data.
     */
    void onDateClicked(const QDate &date);

    /**
     * @brief Obsługuje zmianę typu wykresu.
     */
    void onChartTypeClicked();

    /**
     * @brief Obsługuje kliknięcie przycisku wyświetlania danych.
     */
    void onDisplayButtonClicked();

private:
    /**
     * @struct SensorChartData
     * @brief Struktura przechowująca dane wykresu dla sensora.
     */
    struct SensorChartData {
        QString sensorName; ///< Nazwa sensora.
        double minValue;    ///< Minimalna wartość pomiaru.
        double maxValue;    ///< Maksymalna wartość pomiaru.
        double average;     ///< Średnia wartość pomiarów.
        QString trend;      ///< Trend danych (rosnący, malejący, stabilny).
        QVector<QPointF> points; ///< Punkty danych dla wykresu.
    };

    /**
     * @brief Sprawdza połączenie z internetem.
     * @return true, jeśli połączenie istnieje; false w przeciwnym razie.
     */
    bool checkInternetConnection();

    /**
     * @brief Pobiera dane sensorów dla stacji.
     * @param stationId Identyfikator stacji.
     */
    void fetchSensors(int stationId);

    /**
     * @brief Pobiera dane pomiarowe dla sensora.
     * @param sensorId Identyfikator sensora.
     */
    void fetchMeasurementData(int sensorId);

    /**
     * @brief Pobiera indeks jakości powietrza dla stacji.
     * @param stationId Identyfikator stacji.
     */
    void fetchAirQualityIndex(int stationId);

    /**
     * @brief Wypełnia listę sensorów w interfejsie.
     * @param sensors Tablica JSON z danymi sensorów.
     */
    void populateSensors(const QJsonArray &sensors);

    /**
     * @brief Aktualizuje wyświetlanie wybranych dat.
     */
    void updateSelectedDatesDisplay();

    /**
     * @brief Wyświetla wykresy danych.
     * @param isLineChart Czy wyświetlić wykres liniowy.
     */
    void displayCharts();//bool isLineChart);

    /**
     * @brief Wyświetla informacje o jakości powietrza.
     */
    void displayAirQuality();

    /**
     * @brief Agreguje dane pomiarowe według dat i sensorów.
     * @return Mapa z danymi agregowanymi.
     */
    QMap<QDate, QMap<QString, QMap<int, double>>> aggregateData();

    /**
     * @brief Sprawdza poprawność identyfikatora sesji.
     * @param sessionId Identyfikator sesji.
     * @return true, jeśli sesja jest ważna; false w przeciwnym razie.
     */
    bool isValidSessionId(const QString &sessionId) const;

    /**
     * @brief Wskaźnik na interfejs użytkownika.
     */
    Ui::window_2_data_vis *ui;

    /**
     * @brief Manager sieciowy do żądań HTTP.
     */
    QNetworkAccessManager *m_networkManager;

    /**
     * @brief Układ dla listy sensorów.
     */
    QVBoxLayout *m_sensorLayout;

    /**
     * @brief Identyfikator stacji pomiarowej.
     */
    int m_stationId;

    /**
     * @brief Lista wybranych dat.
     */
    QList<QDate> m_selectedDates;

    /**
     * @brief Lista pól wyboru dla sensorów.
     */
    QList<QCheckBox*> m_sensorCheckBoxes;

    /**
     * @brief Mapa identyfikatorów sensorów na ich nazwy.
     */
    QMap<int, QString> m_sensorIdToName;

    /**
     * @brief Dane pomiarowe dla sensorów.
     */
    QMap<int, QJsonArray> m_measurementData;

    /**
     * @brief Dane o jakości powietrza.
     */
    QJsonObject m_airQualityData;

    /**
     * @brief Wskaźnik na obiekt wykresu.
     */
    QChart *m_chart;

    /**
     * @brief Wskaźnik na widok wykresu.
     */
    QChartView *m_chartView;

    /**
     * @brief Wskaźnik na menedżera historii.
     */
    HistoryManager *m_historyManager;

    /**
     * @brief Identyfikator bieżącej sesji.
     */
    QString m_sessionId;

    /**
     * @brief Agregowane dane pomiarowe.
     */
    QMap<QDate, QMap<QString, QMap<int, double>>> m_aggregatedData;
};

#endif // WINDOW_2_DATA_VIS_H
