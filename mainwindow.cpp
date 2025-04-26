#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "window_2_data_vis.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QLabel>
#include <QUrlQuery>
#include <QDebug>
#include <cmath>
#include <QTimer>
#include <QEventLoop>
#include <QInputDialog>
#include <stdexcept>

/**
 * @brief Konstruktor klasy MainWindow.
 *
 * Inicjalizuje interfejs użytkownika, menedżera sieciowego, menedżera historii
 * i konfiguruje połączenia sygnałów i slotów.
 *
 * @param parent Wskaźnik na widget nadrzędny.
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_stations(),
    m_status("Wpisz lokalizację i kliknij Szukaj."),
    m_inputLocation(),
    m_stationName(),
    m_locationLat(0.0),
    m_locationLon(0.0),
    m_searchRadius(-1.0),
    m_waitingForGeocode(false),
    m_allStations(),
    ui(new Ui::MainWindow)
{
    m_historyManager = new HistoryManager(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/history");
    m_currentSessionId = "";
    ui->setupUi(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        if (m_waitingForGeocode) {
            onGeocodeReply(reply);
        } else {
            onNetworkReply(reply);
        }
    });
    connect(ui->pushButton_szukaj, &QPushButton::clicked, this, &MainWindow::onSearchButtonClicked);
    connect(ui->stationList, &QListWidget::itemClicked, this, &MainWindow::onStationItemClicked);
    connect(ui->pushButton_history, &QPushButton::clicked, this, &MainWindow::onHistoryButtonClicked);
    ui->lineEdit_street_town->setPlaceholderText("ulica numer, Miasto lub Miasto");
    ui->statusLabel->setText(m_status);
}

/**
 * @brief Destruktor klasy MainWindow.
 *
 * Zwalnia zasoby, takie jak interfejs użytkownika, menedżer sieciowy i menedżer historii.
 */
MainWindow::~MainWindow() {
    delete ui;
    delete m_networkManager;
    delete m_historyManager;
}

/**
 * @brief Sprawdza połączenie z internetem.
 *
 * Wykonuje żądania HEAD do znanych punktów końcowych, aby zweryfikować dostępność internetu.
 *
 * @return true, jeśli połączenie istnieje; false w przeciwnym razie.
 */
bool MainWindow::checkInternetConnection() {
    const QStringList endpoints = {"https://www.google.com", "https://cloudflare.com"};
    bool isConnected = false;

    // Temporarily disconnect the global finished signal to prevent HEAD replies from hitting onNetworkReply
    disconnect(m_networkManager, &QNetworkAccessManager::finished, nullptr, nullptr);

    for (const QString& endpoint : endpoints) {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QNetworkReply *reply = nullptr;

        QUrl url(endpoint);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, "AirQualityApp/1.0");

        reply = m_networkManager->head(request);
        qDebug() << "Checking connectivity with HEAD request to:" << endpoint;

        QObject::connect(reply, &QNetworkReply::finished, &loop, [&]() {
            if (reply->error() == QNetworkReply::NoError) {
                isConnected = true;
                qDebug() << "HEAD request to" << reply->url().toString() << "succeeded. Internet is connected.";
            } else {
                qDebug() << "HEAD request to" << reply->url().toString() << "failed:" << reply->errorString();
            }
            loop.quit();
        });

        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(2000);
        loop.exec();

        reply->deleteLater();

        if (isConnected) {
            break;
        }
    }

    // Reconnect the global finished signal
    connect(m_networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        if (m_waitingForGeocode) {
            onGeocodeReply(reply);
        } else {
            onNetworkReply(reply);
        }
    });

    return isConnected;
}

/**
 * @brief Pobiera listę wszystkich stacji z API.
 *
 * Jeśli brak połączenia z internetem, wyświetla odpowiedni komunikat i przerywa operację.
 */
void MainWindow::fetchStations() {
    if (!checkInternetConnection()) {
        m_status = "Brak połączenia z internetem. Sprawdź połączenie\nlub skorzystaj z danych historycznych";
        ui->statusLabel->setText(m_status);
        qDebug() << "No internet connection. Aborting fetchStations.";
        return;
    }

    QUrl url("https://api.gios.gov.pl/pjp-api/rest/station/findAll");
    QNetworkRequest request(url);
    qDebug() << "Fetching stations from:" << url.toString();
    m_networkManager->get(request);
}

/**
 * @brief Obsługuje kliknięcie przycisku wyszukiwania.
 *
 * Pobiera wprowadzoną lokalizację i promień wyszukiwania, generuje nowy identyfikator sesji
 * i inicjuje proces wyszukiwania.
 */
void MainWindow::onSearchButtonClicked() {
    QString input = ui->lineEdit_street_town->text().trimmed();
    if (input.isEmpty()) {
        m_status = "Proszę podać lokalizację.";
        ui->statusLabel->setText(m_status);
        return;
    }

    if (!checkInternetConnection()) {
        m_status = "Brak połączenia z internetem. Sprawdź połączenie i spróbuj ponownie\nlub skorzystaj z danych historycznych";
        ui->statusLabel->setText(m_status);
        qDebug() << "No internet connection. Aborting search.";
        m_locationLat = 0.0;
        m_locationLon = 0.0;
        m_inputLocation = input;
        m_currentSessionId = m_historyManager->generateSessionId();
        qDebug() << "Generated session ID for offline search:" << m_currentSessionId;
        fetchStations();
        return;
    }

    QString radiusInput = ui->lineEdit_promien->text().trimmed();
    radiusInput.replace(",", ".");
    bool ok;
    double radius = radiusInput.toDouble(&ok);
    m_searchRadius = (ok && radius > 0) ? radius : -1.0;

    m_inputLocation = input;
    m_status = "Ładowanie danych dla: " + input;
    if (m_searchRadius > 0) {
        m_status += QString(" (promień: %1 km)").arg(m_searchRadius, 0, 'f', 2);
    }
    ui->statusLabel->setText(m_status);
    m_locationLat = 0.0;
    m_locationLon = 0.0;
    m_currentSessionId = m_historyManager->generateSessionId();
    qDebug() << "Generated session ID for search:" << m_currentSessionId;
    getLocationCoordinates(input);
}

/**
 * @brief Pobiera współrzędne geograficzne dla lokalizacji.
 *
 * Wysyła żądanie do API Nominatim w celu uzyskania współrzędnych geograficznych.
 *
 * @param location Nazwa lokalizacji.
 */
void MainWindow::getLocationCoordinates(const QString &location) {
    if (!checkInternetConnection()) {
        m_status = "Brak połączenia z internetem. Sprawdź połączenie\nlub skorzystaj z danych historycznych";
        ui->statusLabel->setText(m_status);
        qDebug() << "No internet connection. Proceeding without geocoding.";
        fetchStations();
        return;
    }

    QString queryString = location + ", Poland";
    QString encodedQuery = QUrl::toPercentEncoding(queryString);
    QUrl url("https://nominatim.openstreetmap.org/search");
    QUrlQuery query;
    query.addQueryItem("q", encodedQuery);
    query.addQueryItem("format", "json");
    query.addQueryItem("limit", "1");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "AirQualityApp/1.0");
    qDebug() << "Geocoding URL:" << url.toString();

    m_waitingForGeocode = true;
    m_networkManager->get(request);
}

/**
 * @brief Oblicza odległość między dwoma punktami geograficznymi.
 *
 * Używa wzoru haversine do obliczenia odległości w kilometrach.
 *
 * @param lat1 Szerokość geograficzna pierwszego punktu.
 * @param lon1 Długość geograficzna pierwszego punktu.
 * @param lat2 Szerokość geograficzna drugiego punktu.
 * @param lon2 Długość geograficzna drugiego punktu.
 * @return Odległość w kilometrach.
 */
double MainWindow::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0; // Earth's radius in kilometers
    const double PI = 3.14159265358979323846;
    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;
    double a = std::sin(dLat/2.0) * std::sin(dLat/2.0) +
               std::cos(lat1 * PI / 180.0) * std::cos(lat2 * PI / 180.0) *
                   std::sin(dLon/2.0) * std::sin(dLon/2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

/**
 * @brief Obsługuje odpowiedź sieciową dla geokodowania.
 *
 * Przetwarza odpowiedź z API Nominatim, ustawia współrzędne i kontynuuje pobieranie stacji.
 *
 * @param reply Wskaźnik na odpowiedź sieciową.
 */
void MainWindow::onGeocodeReply(QNetworkReply *reply) {
    m_waitingForGeocode = false;

    try {
        if (reply->error() != QNetworkReply::NoError) {
            throw std::runtime_error("Network error: " + reply->errorString().toStdString());
        }

        QByteArray responseData = reply->readAll();
        qDebug() << "Geocode response:" << responseData;

        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isNull()) {
            throw std::runtime_error("Failed to parse geocode response as JSON");
        }
        if (!doc.isArray()) {
            throw std::runtime_error("Geocode response is not a JSON array");
        }

        auto extractCoordinate = [](const QJsonValue &value) -> double {
            if (value.isDouble()) {
                return value.toDouble();
            } else if (value.isString()) {
                bool ok;
                double result = value.toString().toDouble(&ok);
                if (ok) {
                    return result;
                }
            }
            throw std::runtime_error("Invalid coordinate format");
        };

        QJsonArray array = doc.array();
        if (array.isEmpty()) {
            throw std::runtime_error("No coordinates found for: " + m_inputLocation.toStdString());
        }

        QJsonObject result = array[0].toObject();
        qDebug() << "Parsed JSON object:" << result;

        if (!result.contains("lat") || !result.contains("lon")) {
            throw std::runtime_error("Missing 'lat' or 'lon' in response");
        }

        m_locationLat = extractCoordinate(result["lat"]);
        m_locationLon = extractCoordinate(result["lon"]);

        if (m_locationLat == 0.0 || m_locationLon == 0.0) {
            throw std::runtime_error("Invalid coordinate values");
        }

        qDebug() << "Coordinates found - Lat:" << m_locationLat << "Lon:" << m_locationLon;
        m_status = "Znaleziono współrzędne dla: " + m_inputLocation;
        ui->statusLabel->setText(m_status);
        fetchStations();
    } catch (const std::runtime_error &e) {
        m_status = QString("Błąd geokodowania: %1").arg(e.what());
        qDebug() << "Geocode exception:" << e.what();
        ui->statusLabel->setText(m_status);
        fetchStations();
    } catch (...) {
        m_status = "Nieznany błąd podczas geokodowania.";
        qDebug() << "Unknown exception in onGeocodeReply";
        ui->statusLabel->setText(m_status);
        fetchStations();
    }

    reply->deleteLater();
}

/**
 * @brief Obsługuje odpowiedź sieciową dla żądania stacji.
 *
 * Przetwarza dane stacji, filtruje je według lokalizacji i promienia, zapisuje sesję
 * i aktualizuje listę stacji w interfejsie.
 *
 * @param reply Wskaźnik na odpowiedź sieciową.
 */
void MainWindow::onNetworkReply(QNetworkReply *reply) {
    // Only process replies from the station endpoint
    QString replyUrl = reply->url().toString();
    if (!replyUrl.contains("https://api.gios.gov.pl/pjp-api/rest/station/findAll")) {
        qDebug() << "Ignoring non-station reply from:" << replyUrl;
        reply->deleteLater();
        return;
    }

    try {
        if (reply->error() != QNetworkReply::NoError) {
            throw std::runtime_error("Network error: " + reply->errorString().toStdString());
        }

        QByteArray responseData = reply->readAll();
        if (responseData.isEmpty()) {
            throw std::runtime_error("Empty response from server");
        }

        qDebug() << "Stations response:" << responseData;
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isNull()) {
            throw std::runtime_error("Failed to parse stations response as JSON");
        }
        if (!doc.isArray()) {
            throw std::runtime_error("Stations response is not a JSON array");
        }

        QJsonArray stationsArray = doc.array();
        m_stations.clear();
        m_allStations.clear();
        bool found = false;

        QString city;
        if (m_inputLocation.contains(",")) {
            QStringList inputParts = m_inputLocation.split(",", Qt::SkipEmptyParts);
            city = inputParts.size() >= 2 ? inputParts[1].trimmed() : "";
        } else {
            city = m_inputLocation.trimmed();
        }

        if (city.isEmpty()) {
            throw std::runtime_error("Invalid location format");
        }

        for (const QJsonValue &value : stationsArray) {
            QJsonObject station = value.toObject();
            QJsonObject cityObj = station["city"].toObject();
            QString cityName = cityObj["name"].toString();
            QVariantMap stationData;
            stationData["stationId"] = station["id"].toInt();
            stationData["stationName"] = station["stationName"].toString();
            stationData["lat"] = station["gegrLat"].toString();
            stationData["lon"] = station["gegrLon"].toString();
            stationData["address"] = station["addressStreet"].toString();
            stationData["cityName"] = cityName;
            stationData["sessionId"] = m_currentSessionId;

            if (m_locationLat != 0.0 && m_locationLon != 0.0) {
                double lat = stationData["lat"].toString().toDouble();
                double lon = stationData["lon"].toString().toDouble();
                double distance = calculateDistance(m_locationLat, m_locationLon, lat, lon);
                stationData["distance"] = distance;
            } else {
                stationData["distance"] = -1;
            }

            m_allStations.append(stationData);

            if (cityName.toLower() == city.toLower()) {
                found = true;
                m_stations.append(stationData);
            }
        }

        if (found) {
            m_status = "Znaleziono stacje w: " + city;
            if (m_locationLat != 0.0 && m_locationLon != 0.0) {
                QVariantList sortedStations;
                QList<QPair<double, QVariantMap>> stationsWithDistance;
                for (const QVariant &station : m_stations) {
                    QVariantMap stationData = station.toMap();
                    double distance = stationData["distance"].toDouble();
                    stationsWithDistance.append(qMakePair(distance, stationData));
                }
                std::sort(stationsWithDistance.begin(), stationsWithDistance.end(),
                          [](const QPair<double, QVariantMap> &a, const QPair<double, QVariantMap> &b) {
                              return a.first < b.first;
                          });
                for (const auto &pair : stationsWithDistance) {
                    sortedStations.append(pair.second);
                }
                m_stations = sortedStations;
            }
        } else {
            m_status = "Nie znaleziono stacji w: " + city;
            if (m_locationLat != 0.0 && m_locationLon != 0.0) {
                QVariantList nearbyStations;
                QStringList nearbyCities;
                for (const QVariant &station : m_allStations) {
                    QVariantMap stationData = station.toMap();
                    double distance = stationData["distance"].toDouble();
                    if (m_searchRadius > 0) {
                        if (distance <= m_searchRadius) {
                            nearbyStations.append(stationData);
                            QString cityName = stationData["cityName"].toString();
                            if (!nearbyCities.contains(cityName)) {
                                nearbyCities.append(cityName);
                            }
                        }
                    } else {
                        if (nearbyStations.isEmpty() || distance < nearbyStations[0].toMap()["distance"].toDouble()) {
                            nearbyStations.clear();
                            nearbyStations.append(stationData);
                            nearbyCities.clear();
                            nearbyCities.append(stationData["cityName"].toString());
                        }
                    }
                }
                if (!nearbyStations.isEmpty()) {
                    QList<QPair<double, QVariantMap>> stationsWithDistance;
                    for (const QVariant &station : nearbyStations) {
                        QVariantMap stationData = station.toMap();
                        double distance = stationData["distance"].toDouble();
                        stationsWithDistance.append(qMakePair(distance, stationData));
                    }
                    std::sort(stationsWithDistance.begin(), stationsWithDistance.end(),
                              [](const QPair<double, QVariantMap> &a, const QPair<double, QVariantMap> &b) {
                                  return a.first < b.first;
                              });
                    nearbyStations.clear();
                    for (const auto &pair : stationsWithDistance) {
                        nearbyStations.append(pair.second);
                    }
                    m_stations = nearbyStations;
                    m_status += "\nZnaleziono stacje w pobliżu: " + nearbyCities.join(", ");
                } else {
                    m_status += "\nBrak stacji w zadanym promieniu.";
                }
            }
        }

        m_historyManager->addSession(m_currentSessionId, m_inputLocation, m_searchRadius, m_locationLat, m_locationLon, m_stations);
        qDebug() << "Saved session with ID:" << m_currentSessionId << "for stations:" << m_stations.size();

        ui->statusLabel->setText(m_status);
        updateStationList();
    } catch (const std::runtime_error &e) {
        m_status = QString("Błąd pobierania danych: %1").arg(e.what());
        qDebug() << "Network reply exception:" << e.what();
        ui->statusLabel->setText(m_status);
    } catch (...) {
        m_status = "Nieznany błąd podczas pobierania danych stacji.";
        qDebug() << "Unknown exception in onNetworkReply";
        ui->statusLabel->setText(m_status);
    }

    reply->deleteLater();
}

/**
 * @brief Aktualizuje listę stacji w interfejsie użytkownika.
 *
 * Tworzy widżety dla każdej stacji, wyświetlając nazwę, ID, współrzędne, adres
 * i odległość (jeśli dostępna).
 */
void MainWindow::updateStationList() {
    ui->stationList->clear();
    for (const QVariant &station : m_stations) {
        QVariantMap stationData = station.toMap();
        QWidget *itemWidget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(itemWidget);
        layout->setContentsMargins(5, 5, 5, 5);
        layout->setSpacing(4);

        QLabel *nameLabel = new QLabel("<b>Nazwa:</b> " + stationData["stationName"].toString());
        QLabel *idLabel = new QLabel("<b>ID:</b> " + QString::number(stationData["stationId"].toInt()));
        QLabel *coordsLabel = new QLabel("<b>Współrzędne:</b> " + stationData["lat"].toString() + ", " + stationData["lon"].toString());
        QLabel *addressLabel = new QLabel("<b>Adres:</b> " + (stationData["address"].toString().isEmpty() ? "Brak danych" : stationData["address"].toString()));

        QLabel *distanceLabel = nullptr;
        if (stationData.contains("distance") && stationData["distance"].toDouble() >= 0) {
            double distance = stationData["distance"].toDouble();
            distanceLabel = new QLabel(QString("<b>Odległość:</b> %1 km").arg(distance, 0, 'f', 2));
            distanceLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");
        }

        nameLabel->setStyleSheet("font-size: 14px;");
        idLabel->setStyleSheet("font-size: 14px;");
        coordsLabel->setStyleSheet("font-size: 14px;");
        addressLabel->setStyleSheet("font-size: 14px;");

        layout->addWidget(nameLabel);
        layout->addWidget(idLabel);
        layout->addWidget(coordsLabel);
        layout->addWidget(addressLabel);
        if (distanceLabel) {
            layout->addWidget(distanceLabel);
        }

        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(QSize(0, distanceLabel ? 150 : 120));
        ui->stationList->addItem(item);
        ui->stationList->setItemWidget(item, itemWidget);
    }
}

/**
 * @brief Obsługuje kliknięcie elementu listy stacji.
 *
 * Otwiera okno wizualizacji danych dla wybranej stacji.
 *
 * @param item Wskaźnik na kliknięty element listy.
 */
void MainWindow::onStationItemClicked(QListWidgetItem *item) {
    int index = ui->stationList->row(item);

    if (index >= 0 && index < m_stations.size()) {
        QVariantMap stationData = m_stations[index].toMap();
        int stationId = stationData["stationId"].toInt();
        QString stationName = stationData["stationName"].toString();
        QString sessionId = stationData["sessionId"].toString();

        qDebug() << "Opening data vis window for station ID:" << stationId << "with session ID:" << sessionId;

        window_2_data_vis *dataVisWindow = new window_2_data_vis(stationId, m_historyManager, sessionId, this);
        dataVisWindow->setWindowTitle("Dane dla stacji: " + stationName);
        dataVisWindow->show();
    }
}

/**
 * @brief Obsługuje kliknięcie przycisku historii.
 *
 * Wyświetla listę zapisanych sesji i pozwala użytkownikowi wybrać jedną do wczytania.
 */
void MainWindow::onHistoryButtonClicked() {
    QVariantList sessions = m_historyManager->loadSessions();
    if (sessions.isEmpty()) {
        m_status = "Brak zapisanych sesji w historii.";
        ui->statusLabel->setText(m_status);
        qDebug() << "No sessions found in history.";
        return;
    }

    QStringList sessionDescriptions;
    QMap<QString, QString> sessionIdToDescription;
    for (const QVariant &session : sessions) {
        QVariantMap sessionData = session.toMap();
        QString sessionId = sessionData["session_id"].toString();
        QString location = sessionData["location"].toString();
        QString timestamp = sessionData["timestamp"].toString();
        double radius = sessionData["radius"].toDouble();
        QString description = QString("%1 (Data: %2").arg(location, timestamp);
        if (radius > 0) {
            description += QString(", Promień: %1 km)").arg(radius, 0, 'f', 2);
        } else {
            description += ")";
        }
        sessionDescriptions.append(description);
        sessionIdToDescription[description] = sessionId;
    }

    bool ok;
    QString selectedDescription = QInputDialog::getItem(this, "Wybierz sesję",
                                                        "Wybierz sesję z historii:", sessionDescriptions,
                                                        0, false, &ok);
    if (!ok || selectedDescription.isEmpty()) {
        m_status = "Nie wybrano sesji.";
        ui->statusLabel->setText(m_status);
        qDebug() << "No session selected from history.";
        return;
    }

    QString selectedSessionId = sessionIdToDescription[selectedDescription];
    QVariantMap sessionDetails = m_historyManager->loadSessionDetails(selectedSessionId);
    if (sessionDetails.isEmpty()) {
        m_status = "Nie udało się załadować szczegółów sesji.";
        ui->statusLabel->setText(m_status);
        qDebug() << "Failed to load session details for session ID:" << selectedSessionId;
        return;
    }

    m_stations = sessionDetails["stations"].toList();
    m_inputLocation = sessionDetails["location"].toMap()["input"].toString();
    m_locationLat = sessionDetails["location"].toMap()["latitude"].toDouble();
    m_locationLon = sessionDetails["location"].toMap()["longitude"].toDouble();
    m_searchRadius = sessionDetails["radius"].toDouble();
    m_currentSessionId = selectedSessionId;

    m_status = QString("Załadowano sesję dla: %1").arg(m_inputLocation);
    if (m_searchRadius > 0) {
        m_status += QString(" (promień: %1 km)").arg(m_searchRadius, 0, 'f', 2);
    }
    ui->statusLabel->setText(m_status);
    qDebug() << "Loaded session ID:" << selectedSessionId << "with" << m_stations.size() << "stations.";

    updateStationList();
}
