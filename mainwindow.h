#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QListWidgetItem>
#include <QStandardPaths>
#include "historymanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Główne okno aplikacji do wyszukiwania stacji pomiarowych.
 *
 * Klasa MainWindow zarządza interfejsem użytkownika, obsługuje wyszukiwanie stacji
 * na podstawie lokalizacji, komunikuje się z API i zarządza historią sesji.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy MainWindow.
     * @param parent Wskaźnik na widget nadrzędny (domyślnie nullptr).
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destruktor klasy MainWindow.
     */
    ~MainWindow();

private slots:
    /**
     * @brief Obsługuje kliknięcie przycisku wyszukiwania.
     */
    void onSearchButtonClicked();

    /**
     * @brief Obsługuje odpowiedź sieciową dla żądań API stacji.
     * @param reply Wskaźnik na odpowiedź sieciową.
     */
    void onNetworkReply(QNetworkReply *reply);

    /**
     * @brief Obsługuje odpowiedź sieciową dla geokodowania.
     * @param reply Wskaźnik na odpowiedź sieciową.
     */
    void onGeocodeReply(QNetworkReply *reply);

    /**
     * @brief Obsługuje kliknięcie elementu listy stacji.
     * @param item Wskaźnik na kliknięty element listy.
     */
    void onStationItemClicked(QListWidgetItem *item);

    /**
     * @brief Obsługuje kliknięcie przycisku historii.
     */
    void onHistoryButtonClicked();

private:
    /**
     * @brief Sprawdza połączenie z internetem.
     * @return true, jeśli połączenie istnieje; false w przeciwnym razie.
     */
    bool checkInternetConnection();

    /**
     * @brief Pobiera listę wszystkich stacji z API.
     */
    void fetchStations();

    /**
     * @brief Pobiera współrzędne geograficzne dla podanej lokalizacji.
     * @param location Nazwa lokalizacji (np. miasto lub adres).
     */
    void getLocationCoordinates(const QString &location);

    /**
     * @brief Aktualizuje listę stacji w interfejsie użytkownika.
     */
    void updateStationList();

    /**
     * @brief Oblicza odległość między dwoma punktami geograficznymi.
     * @param lat1 Szerokość geograficzna pierwszego punktu.
     * @param lon1 Długość geograficzna pierwszego punktu.
     * @param lat2 Szerokość geograficzna drugiego punktu.
     * @param lon2 Długość geograficzna drugiego punktu.
     * @return Odległość w kilometrach.
     */
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);

    /**
     * @brief Wskaźnik na interfejs użytkownika.
     */
    Ui::MainWindow *ui;

    /**
     * @brief Manager sieciowy do wysyłania żądań HTTP.
     */
    QNetworkAccessManager *m_networkManager;

    /**
     * @brief Lista stacji pomiarowych.
     */
    QVariantList m_stations;

    /**
     * @brief Status aplikacji wyświetlany w interfejsie.
     */
    QString m_status;

    /**
     * @brief Wprowadzona lokalizacja przez użytkownika.
     */
    QString m_inputLocation;

    /**
     * @brief Nazwa wybranej stacji.
     */
    QString m_stationName;

    /**
     * @brief Szerokość geograficzna lokalizacji.
     */
    double m_locationLat;

    /**
     * @brief Długość geograficzna lokalizacji.
     */
    double m_locationLon;

    /**
     * @brief Promień wyszukiwania w kilometrach.
     */
    double m_searchRadius;

    /**
     * @brief Flaga wskazująca, czy oczekiwana jest odpowiedź geokodowania.
     */
    bool m_waitingForGeocode;

    /**
     * @brief Lista wszystkich stacji (przed filtrowaniem).
     */
    QVariantList m_allStations;

    /**
     * @brief Wskaźnik na menedżera historii sesji.
     */
    HistoryManager *m_historyManager;

    /**
     * @brief Identyfikator bieżącej sesji.
     */
    QString m_currentSessionId;
};

#endif // MAINWINDOW_H
