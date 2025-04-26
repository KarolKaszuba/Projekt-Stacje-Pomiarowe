/**
 * @file window_2_data_vis.cpp
 * @brief Implementacja klasy window_2_data_vis do wizualizacji danych pomiarowych ze stacji.
 *
 * Plik zawiera definicje metod klasy window_2_data_vis, która odpowiada za wyświetlanie danych
 * z sensorów, wykresów oraz informacji o jakości powietrza dla wybranej stacji pomiarowej.
 * Klasa obsługuje zarówno dane pobierane online, jak i dane przechowywane w historii sesji.
 */

#include "window_2_data_vis.h"
#include "ui_window_2_data_vis.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QLabel>
#include <QTextCharFormat>
#include <QDateTime>
#include <QListWidgetItem>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <QEventLoop>
#include <thread>
//#include <mutex>

/**
 * @brief Konstruktor klasy window_2_data_vis.
 *
 * Inicjalizuje okno wizualizacji danych, ustawia interfejs użytkownika, menedżera sieciowego
 * oraz konfiguruje połączenia sygnałów i slotów. Pobiera dane sensorów i jakości powietrza
 * dla podanej stacji.
 *
 * @param stationId Identyfikator stacji pomiarowej.
 * @param historyManager Wskaźnik na menedżera historii sesji.
 * @param sessionId Identyfikator sesji.
 * @param parent Wskaźnik na widget nadrzędny (domyślnie nullptr).
 */
window_2_data_vis::window_2_data_vis(int stationId, HistoryManager *historyManager, const QString &sessionId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::window_2_data_vis)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_stationId(stationId)
    , m_chart(nullptr)
    , m_chartView(nullptr)
    , m_historyManager(historyManager)
    , m_sessionId(sessionId)
{
    ui->setupUi(this);

    qDebug() << "Initialized window_2_data_vis with session ID:" << m_sessionId;

    m_sensorLayout = ui->grBox_sensors->findChild<QVBoxLayout*>("verticalLayout");
    if (!m_sensorLayout) {
        qDebug() << "Error: Could not find verticalLayout in grBox_sensors.";
        m_sensorLayout = new QVBoxLayout(ui->grBox_sensors);
        ui->grBox_sensors->setLayout(m_sensorLayout);
    }

    connect(m_networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        if (reply->url().toString().contains("/data/getData/")) {
            onMeasurementReply(reply);
        } else if (reply->url().toString().contains("/aqindex/getIndex/")) {
            onAirQualityReply(reply);
        } else {
            onSensorReply(reply);
        }
    });

    ui->calendarWidget->setSelectionMode(QCalendarWidget::SingleSelection);
    connect(ui->calendarWidget, &QCalendarWidget::clicked, this, &window_2_data_vis::onDateClicked);

    connect(ui->wykr_kolowy, &QCheckBox::clicked, this, &window_2_data_vis::onChartTypeClicked);

    connect(ui->pushButton, &QPushButton::clicked, this, &window_2_data_vis::onDisplayButtonClicked);

    fetchSensors(m_stationId);
    fetchAirQualityIndex(m_stationId);
}

/**
 * @brief Destruktor klasy window_2_data_vis.
 *
 * Zwalnia zasoby, takie jak interfejs użytkownika i menedżer sieciowy.
 */
window_2_data_vis::~window_2_data_vis()
{
    delete ui;
    delete m_networkManager;
}

/**
 * @brief Sprawdza dostępność połączenia internetowego.
 *
 * Wykonuje żądania HEAD do znanych punktów końcowych w celu weryfikacji połączenia z internetem.
 *
 * @return true, jeśli połączenie istnieje; false w przeciwnym razie.
 */
bool window_2_data_vis::checkInternetConnection() {
    const QStringList endpoints = {"https://www.google.com", "https://cloudflare.com"};
    bool isConnected = false;

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
                qDebug() << "HEAD request to" << reply->url().toString() << "succeeded.";
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

    return isConnected;
}

/**
 * @brief Pobiera dane sensorów dla wybranej stacji.
 *
 * Jeśli połączenie internetowe jest dostępne, pobiera dane z API. W przeciwnym razie
 * ładuje dane z historii sesji.
 *
 * @param stationId Identyfikator stacji pomiarowej.
 */
void window_2_data_vis::fetchSensors(int stationId)
{
    if (checkInternetConnection()) {
        QUrl url("https://api.gios.gov.pl/pjp-api/rest/station/sensors/" + QString::number(stationId));
        QNetworkRequest request(url);
        qDebug() << "Fetching sensors for station ID:" << stationId << "from:" << url.toString();
        m_networkManager->get(request);
    } else {
        qDebug() << "No internet connection. Loading sensors from history for station ID:" << stationId;
        QVariantMap sessionData = m_historyManager->loadSessionDetails(m_sessionId);
        if (sessionData.isEmpty()) {
            qDebug() << "No session data found for session ID:" << m_sessionId;
            populateSensors(QJsonArray());
            return;
        }

        QVariantList sensors = sessionData["sensors"].toList();
        QJsonArray sensorsArray;
        for (const QVariant &sensorVariant : sensors) {
            QVariantMap sensor = sensorVariant.toMap();
            if (sensor["stationId"].toInt() == stationId) {
                QJsonObject sensorObj;
                sensorObj["id"] = sensor["id"].toInt();
                sensorObj["stationId"] = sensor["stationId"].toInt();
                QJsonObject paramObj;
                QVariantMap param = sensor["param"].toMap();
                paramObj["paramName"] = param["paramName"].toString();
                paramObj["paramFormula"] = param["paramFormula"].toString();
                paramObj["paramCode"] = param["paramCode"].toString();
                paramObj["idParam"] = param["idParam"].toInt();
                sensorObj["param"] = paramObj;
                sensorsArray.append(sensorObj);
            }
        }

        if (sensorsArray.isEmpty()) {
            qDebug() << "No sensors found in history for station ID:" << stationId;
        } else {
            qDebug() << "Loaded" << sensorsArray.size() << "sensors from history for station ID:" << stationId;
        }
        populateSensors(sensorsArray);
    }
}

/**
 * @brief Pobiera dane pomiarowe dla wybranego sensora.
 *
 * Jeśli połączenie internetowe jest dostępne, pobiera dane z API. W przeciwnym razie
 * ładuje pomiary z historii sesji.
 *
 * @param sensorId Identyfikator sensora.
 */
void window_2_data_vis::fetchMeasurementData(int sensorId)
{
    if (checkInternetConnection()) {
        QUrl url("https://api.gios.gov.pl/pjp-api/rest/data/getData/" + QString::number(sensorId));
        QNetworkRequest request(url);
        qDebug() << "Fetching measurement data for sensor ID:" << sensorId << "from:" << url.toString();
        m_networkManager->get(request);
    } else {
        qDebug() << "No internet connection. Loading measurements from history for sensor ID:" << sensorId;
        QVariantMap sessionData = m_historyManager->loadSessionDetails(m_sessionId);
        if (sessionData.isEmpty()) {
            qDebug() << "No session data found for session ID:" << m_sessionId;
            return;
        }

        QVariantList sensors = sessionData["sensors"].toList();
        QJsonArray measurementsArray;
        for (const QVariant &sensorVariant : sensors) {
            QVariantMap sensor = sensorVariant.toMap();
            if (sensor["id"].toInt() == sensorId) {
                QVariantList measurements = sensor["measurements"].toList();
                for (const QVariant &measurementVariant : measurements) {
                    QVariantMap measurement = measurementVariant.toMap();
                    QJsonObject measurementObj;
                    measurementObj["date"] = measurement["date"].toString();
                    measurementObj["value"] = measurement["value"].isValid() ? measurement["value"].toDouble() : QJsonValue::Null;
                    measurementsArray.append(measurementObj);
                }
                break;
            }
        }

        if (measurementsArray.isEmpty()) {
            qDebug() << "No measurements found in history for sensor ID:" << sensorId;
        } else {
            qDebug() << "Loaded" << measurementsArray.size() << "measurements from history for sensor ID:" << sensorId;
        }

        m_measurementData[sensorId] = measurementsArray;
    }
}

/**
 * @brief Pobiera indeks jakości powietrza dla wybranej stacji.
 *
 * Jeśli połączenie internetowe jest dostępne, pobiera dane z API. W przeciwnym razie
 * ładuje dane z historii sesji.
 *
 * @param stationId Identyfikator stacji pomiarowej.
 */
void window_2_data_vis::fetchAirQualityIndex(int stationId)
{
    if (checkInternetConnection()) {
        QUrl url("https://api.gios.gov.pl/pjp-api/rest/aqindex/getIndex/" + QString::number(stationId));
        QNetworkRequest request(url);
        qDebug() << "Fetching air quality index for station ID:" << stationId << "from:" << url.toString();
        m_networkManager->get(request);
    } else {
        qDebug() << "No internet connection. Loading air quality index from history for station ID:" << stationId;
        QVariantMap sessionData = m_historyManager->loadSessionDetails(m_sessionId);
        if (sessionData.isEmpty()) {
            qDebug() << "No session data found for session ID:" << m_sessionId;
            m_airQualityData = QJsonObject();
            displayAirQuality();
            return;
        }

        QVariantMap airQuality = sessionData["airQuality"].toMap();
        if (airQuality.isEmpty()) {
            qDebug() << "No air quality data found in history for session ID:" << m_sessionId;
            m_airQualityData = QJsonObject();
        } else {
            QJsonObject airQualityObj;
            airQualityObj["stCalcDate"] = airQuality["stCalcDate"].toString();
            QJsonObject indexLevelObj;
            indexLevelObj["indexLevelName"] = airQuality["indexLevelName"].toString();
            airQualityObj["stIndexLevel"] = indexLevelObj;
            m_airQualityData = airQualityObj;
            qDebug() << "Loaded air quality data from history for session ID:" << m_sessionId;
        }
        displayAirQuality();
    }
}

/**
 * @brief Sprawdza poprawność identyfikatora sesji.
 *
 * Weryfikuje, czy podany identyfikator sesji jest niepusty i czy odpowiadający mu plik sesji istnieje.
 *
 * @param sessionId Identyfikator sesji.
 * @return true, jeśli sesja jest ważna; false w przeciwnym razie.
 */
bool window_2_data_vis::isValidSessionId(const QString &sessionId) const
{
    if (sessionId.isEmpty()) {
        qDebug() << "Invalid session ID: empty";
        return false;
    }
    QString sessionFile = QString("session_%1.json").arg(sessionId);
    QFile file(m_historyManager->m_historyDir.filePath(sessionFile));
    if (!file.exists()) {
        qDebug() << "Invalid session ID: session file does not exist:" << sessionFile;
        return false;
    }
    return true;
}

/**
 * @brief Obsługuje odpowiedź sieciową dla żądania sensorów.
 *
 * Przetwarza dane sensorów z API, zapisuje je do historii sesji (jeśli sesja jest ważna)
 * i aktualizuje listę sensorów w interfejsie użytkownika.
 *
 * @param reply Wskaźnik na odpowiedź sieciową.
 */
void window_2_data_vis::onSensorReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Sensor fetch error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    qDebug() << "Sensor request reply for station ID" << m_stationId << ":" << responseData;

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isArray()) {
        qDebug() << "Failed to parse sensor response as JSON array. Response:" << responseData;
        reply->deleteLater();
        return;
    }

    QJsonArray sensorsArray = doc.array();
    qDebug() << "Parsed sensors array:" << QJsonDocument(sensorsArray).toJson(QJsonDocument::Indented);

    QList<QVariantMap> sensorsList;
    for (const QJsonValue &value : sensorsArray) {
        QJsonObject sensor = value.toObject();
        QVariantMap sensorData;
        sensorData["id"] = sensor["id"].toInt();
        sensorData["stationId"] = sensor["stationId"].toInt();
        QVariantMap paramData;
        if (sensor.contains("param")) {
            QJsonObject param = sensor["param"].toObject();
            paramData["paramName"] = param["paramName"].toString();
            paramData["paramFormula"] = param["paramFormula"].toString();
            paramData["paramCode"] = param["paramCode"].toString();
            paramData["idParam"] = param["idParam"].toInt();
        }
        sensorData["param"] = paramData;
        sensorData["measurements"] = QVariantList();
        sensorsList.append(sensorData);
    }
    if (!sensorsList.isEmpty() && isValidSessionId(m_sessionId)) {
        m_historyManager->addSessionSensors(m_sessionId, sensorsList);
        qDebug() << "Saved" << sensorsList.size() << "sensors for session:" << m_sessionId;
    } else if (!isValidSessionId(m_sessionId)) {
        qDebug() << "Skipped saving sensors due to invalid session ID:" << m_sessionId;
    }

    populateSensors(sensorsArray);

    reply->deleteLater();
}

/**
 * @brief Obsługuje odpowiedź sieciową dla żądania danych pomiarowych.
 *
 * Przetwarza dane pomiarowe z API, zapisuje je do historii sesji i przechowuje w lokalnej strukturze danych.
 *
 * @param reply Wskaźnik na odpowiedź sieciową.
 */
void window_2_data_vis::onMeasurementReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Measurement fetch error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Failed to parse measurement response as JSON object. Response:" << responseData;
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    int sensorId = reply->url().toString().split("/").last().toInt();
    m_measurementData[sensorId] = obj["values"].toArray();

    QList<QVariantMap> measurementsList;
    QJsonArray values = obj["values"].toArray();
    for (const QJsonValue &value : values) {
        QJsonObject measurement = value.toObject();
        QVariantMap measurementData;
        measurementData["sensorId"] = sensorId;
        measurementData["date"] = measurement["date"].toString();
        measurementData["value"] = measurement["value"].isNull() ? QVariant() : measurement["value"].toDouble();
        measurementsList.append(measurementData);
    }
    if (!measurementsList.isEmpty() && isValidSessionId(m_sessionId)) {
        m_historyManager->addSessionMeasurements(m_sessionId, measurementsList);
        qDebug() << "Saved" << measurementsList.size() << "measurements for sensor ID:" << sensorId << "in session:" << m_sessionId;
    } else if (!isValidSessionId(m_sessionId)) {
        qDebug() << "Skipped saving measurements due to invalid session ID:" << m_sessionId;
    }

    qDebug() << "Stored measurement data for sensor ID" << sensorId << ":" << QJsonDocument(obj).toJson(QJsonDocument::Indented);

    reply->deleteLater();
}

/**
 * @brief Obsługuje odpowiedź sieciową dla żądania jakości powietrza.
 *
 * Przetwarza dane o jakości powietrza, zapisuje je do historii sesji i aktualizuje
 * wyświetlanie w interfejsie użytkownika.
 *
 * @param reply Wskaźnik na odpowiedź sieciową.
 */
void window_2_data_vis::onAirQualityReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Air quality fetch error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Failed to parse air quality response as JSON object. Response:" << responseData;
        reply->deleteLater();
        return;
    }

    m_airQualityData = doc.object();
    qDebug() << "Stored air quality data for station ID" << m_stationId << ":" << QJsonDocument(m_airQualityData).toJson(QJsonDocument::Indented);

    QVariantMap airQualityData;
    airQualityData["stCalcDate"] = m_airQualityData["stCalcDate"].toString();
    QJsonObject indexLevelObj = m_airQualityData["stIndexLevel"].toObject();
    airQualityData["indexLevelName"] = indexLevelObj["indexLevelName"].toString();

    if (isValidSessionId(m_sessionId)) {
        m_historyManager->addSessionAirQuality(m_sessionId, airQualityData);
        qDebug() << "Saved air quality data for session:" << m_sessionId;
    } else {
        qDebug() << "Skipped saving air quality data due to invalid session ID:" << m_sessionId;
    }

    displayAirQuality();
    reply->deleteLater();
}

/**
 * @brief Wypełnia listę sensorów w interfejsie użytkownika.
 *
 * Tworzy pola wyboru dla każdego sensora na podstawie danych z tablicy JSON.
 * Jeśli brak sensorów, wyświetla odpowiedni komunikat.
 *
 * @param sensors Tablica JSON z danymi sensorów.
 */
void window_2_data_vis::populateSensors(const QJsonArray &sensors)
{
    m_sensorCheckBoxes.clear();
    QLayoutItem *item;
    while (m_sensorLayout->count() > 1) {
        item = m_sensorLayout->takeAt(1);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    if (sensors.isEmpty()) {
        qDebug() << "No sensors found for station ID" << m_stationId;
        QLabel *noSensorsLabel = new QLabel("Brak dostępnych sensorów.");
        m_sensorLayout->addWidget(noSensorsLabel);
        return;
    }

    for (const QJsonValue &value : sensors) {
        QJsonObject sensor = value.toObject();
        if (sensor.contains("param")) {
            QJsonObject param = sensor["param"].toObject();
            QString paramName = param["paramName"].toString();
            QString paramFormula = param["paramFormula"].toString();
            int sensorId = sensor["id"].toInt();
            QString checkBoxText = QString("sensor: '%1' -> '%2'").arg(paramName, paramFormula);
            QCheckBox *checkBox = new QCheckBox(checkBoxText);
            checkBox->setProperty("sensorId", sensorId);
            m_sensorLayout->addWidget(checkBox);
            m_sensorCheckBoxes.append(checkBox);
            m_sensorIdToName[sensorId] = paramName;
            qDebug() << "Added checkbox:" << checkBoxText << "with sensor ID:" << sensorId;
        }
    }

    m_sensorLayout->addStretch();
}

/**
 * @brief Obsługuje kliknięcie daty w kalendarzu.
 *
 * Dodaje lub usuwa wybraną datę z listy wybranych dat i aktualizuje jej wyświetlanie.
 *
 * @param date Wybrana data.
 */
void window_2_data_vis::onDateClicked(const QDate &date)
{
    if (m_selectedDates.contains(date)) {
        m_selectedDates.removeAll(date);
    } else {
        m_selectedDates.append(date);
    }

    QTextCharFormat format;
    format.setBackground(Qt::lightGray);
    ui->calendarWidget->setDateTextFormat(QDate(), format);

    format.setBackground(QColor(173, 216, 230));
    for (const QDate &selectedDate : m_selectedDates) {
        ui->calendarWidget->setDateTextFormat(selectedDate, format);
    }

    updateSelectedDatesDisplay();
}

/**
 * @brief Aktualizuje wyświetlanie listy wybranych dat.
 *
 * Czyści listę wybranych dat w interfejsie i wyświetla nowe daty lub komunikat,
 * jeśli brak wybranych dat. Wyświetla również informacje o jakości powietrza.
 */
void window_2_data_vis::updateSelectedDatesDisplay()
{
    ui->listWidget->clear();
    displayAirQuality();
    if (m_selectedDates.isEmpty()) {
        ui->listWidget->addItem("Brak wybranych dat.");
    } else {
        QList<QDate> sortedDates = m_selectedDates;
        std::sort(sortedDates.begin(), sortedDates.end());
        for (const QDate &date : sortedDates) {
            ui->listWidget->addItem(date.toString("yyyy-MM-dd"));
        }
    }
}

/**
 * @brief Wyświetla informacje o jakości powietrza.
 *
 * Tworzy widżet z informacjami o jakości powietrza (data ostatniego pomiaru i poziom indeksu)
 * na podstawie danych online lub z historii sesji.
 */
void window_2_data_vis::displayAirQuality()
{
    QWidget *airQualityWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(airQualityWidget);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(4);

    QString calcDate = "Brak danych";
    QString indexLevel = "Brak danych";

    if (!m_airQualityData.isEmpty()) {
        calcDate = m_airQualityData["stCalcDate"].toString();
        QJsonObject indexLevelObj = m_airQualityData["stIndexLevel"].toObject();
        indexLevel = indexLevelObj["indexLevelName"].toString();
    } else if (!checkInternetConnection()) {
        QVariantMap sessionData = m_historyManager->loadSessionDetails(m_sessionId);
        if (!sessionData.isEmpty()) {
            QVariantMap airQuality = sessionData["airQuality"].toMap();
            if (!airQuality.isEmpty()) {
                calcDate = airQuality["stCalcDate"].toString();
                indexLevel = airQuality["indexLevelName"].toString();
                qDebug() << "Loaded air quality data from history: calcDate=" << calcDate << ", indexLevel=" << indexLevel;
            } else {
                indexLevel = "Niedostępne w trybie offline";
            }
        } else {
            indexLevel = "Niedostępne w trybie offline";
        }
    }

    QLabel *titleLabel = new QLabel("<b>Indeks jakości powietrza</b>");
    QLabel *calcDateLabel = new QLabel(QString("<b>Ostatni pomiar:</b> %1").arg(calcDate));
    QLabel *indexLevelLabel = new QLabel(QString("<b>Jakość powietrza:</b> %1").arg(indexLevel));

    titleLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");
    calcDateLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");
    indexLevelLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");

    layout->addWidget(titleLabel);
    layout->addWidget(calcDateLabel);
    layout->addWidget(indexLevelLabel);

    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 100));
    ui->listWidget->insertItem(0, item);
    ui->listWidget->setItemWidget(item, airQualityWidget);
}

/**
 * @brief Obsługuje zmianę typu wykresu.
 *
 * Ustawia wybór typu wykresu (liniowy) w interfejsie użytkownika, odznaczając inne opcje.
 */
void window_2_data_vis::onChartTypeClicked()
{
    QCheckBox *sender = qobject_cast<QCheckBox*>(QObject::sender());
    if (!sender) return;
    if (sender != ui->wykr_kolowy) ui->wykr_kolowy->setChecked(false);
}

/**
 * @brief Obsługuje kliknięcie przycisku wyświetlania danych.
 *
 * Inicjuje proces pobierania i agregacji danych dla wybranych sensorów i dat,
 * a następnie wyświetla wykresy i statystyki.
 */
void window_2_data_vis::onDisplayButtonClicked()
{
    ui->listWidget->clear();
    displayAirQuality();
    m_measurementData.clear();

    QList<int> selectedSensorIds;
    for (QCheckBox *checkBox : m_sensorCheckBoxes) {
        if (checkBox->isChecked()) {
            int sensorId = checkBox->property("sensorId").toInt();
            selectedSensorIds.append(sensorId);
        }
    }

    if (selectedSensorIds.isEmpty()) {
        ui->listWidget->addItem("Proszę wybrać co najmniej jeden sensor.");
        return;
    }

    if (m_selectedDates.isEmpty()) {
        ui->listWidget->addItem("Proszę wybrać co najmniej jeden dzień.");
        return;
    }

    bool isLineChart = ui->wykr_kolowy->isChecked();
    if (!isLineChart) {
        ui->listWidget->addItem("Proszę wybrać typ wykresu.");
        return;
    }

    //bool hasInternet = checkInternetConnection();
    for (int sensorId : selectedSensorIds) {
        fetchMeasurementData(sensorId);
    }

    // Pokazanie komunikatu ładowania
    ui->listWidget->addItem("Agregowanie danych...");

    // Uruchomienie agregacji danych w osobnym wątku
    std::thread t([this]() {
        m_aggregatedData = aggregateData();
    });

    // Oczekiwanie na zakończenie wątku
    t.join();

    // Usunięcie komunikatu ładowania
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        if (ui->listWidget->item(i)->text() == "Agregowanie danych...") {
            delete ui->listWidget->takeItem(i);
            break;
        }
    }

    // Wyświetlenie wykresów z zagregowanymi danymi
    displayCharts();//isLineChart);
}

/**
 * @brief Agreguje dane pomiarowe według dat i sensorów.
 *
 * Łączy dane z historii sesji i dane online, organizując je według dat, nazw sensorów
 * i godzin pomiarów.
 *
 * @return Mapa z danymi zagregowanymi.
 */
QMap<QDate, QMap<QString, QMap<int, double>>> window_2_data_vis::aggregateData()
{
    QMap<QDate, QMap<QString, QMap<int, double>>> aggregatedData;

    // Wczytanie danych sesji
    QVariantMap sessionData = m_historyManager->loadSessionDetails(m_sessionId);
    if (sessionData.isEmpty()) {
        qDebug() << "No session data found for session ID:" << m_sessionId;
        return aggregatedData;
    }

    QVariantList sensors = sessionData["sensors"].toList();
    QSet<int> selectedSensorIds;
    for (QCheckBox *checkBox : m_sensorCheckBoxes) {
        if (checkBox->isChecked()) {
            selectedSensorIds.insert(checkBox->property("sensorId").toInt());
        }
    }

    // Agregacja danych z historii
    for (const QVariant &sensorVariant : sensors) {
        QVariantMap sensor = sensorVariant.toMap();
        int sensorId = sensor["id"].toInt();
        if (!selectedSensorIds.contains(sensorId) || sensor["stationId"].toInt() != m_stationId) {
            continue;
        }
        QString sensorName = sensor["param"].toMap()["paramName"].toString();
        QVariantList measurements = sensor["measurements"].toList();

        for (const QVariant &measurementVariant : measurements) {
            QVariantMap measurement = measurementVariant.toMap();
            QString dateTimeStr = measurement["date"].toString();
            QDateTime dateTime = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd HH:mm:ss");
            if (!dateTime.isValid()) {
                qDebug() << "Invalid date format in measurement:" << dateTimeStr;
                continue;
            }

            QDate date = dateTime.date();
            if (!m_selectedDates.contains(date)) {
                continue;
            }

            int hour = dateTime.time().hour();
            double value = measurement["value"].isValid() ? measurement["value"].toDouble() : 0.0;
            if (value != 0.0) {
                aggregatedData[date][sensorName][hour] += value;
            }
        }
    }

    // Włączenie danych online, jeśli dostępne
    for (int sensorId : m_measurementData.keys()) {
        if (!selectedSensorIds.contains(sensorId)) {
            continue;
        }
        QString sensorName = m_sensorIdToName[sensorId];
        QJsonArray values = m_measurementData[sensorId];

        for (const QJsonValue &value : values) {
            QJsonObject obj = value.toObject();
            QString dateTimeStr = obj["date"].toString();
            QDateTime dateTime = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd HH:mm:ss");
            if (!dateTime.isValid()) {
                qDebug() << "Invalid date format in online measurement:" << dateTimeStr;
                continue;
            }

            QDate date = dateTime.date();
            if (!m_selectedDates.contains(date)) {
                continue;
            }

            int hour = dateTime.time().hour();
            double measurementValue = obj["value"].isNull() ? 0.0 : obj["value"].toDouble();
            if (measurementValue != 0.0) {
                aggregatedData[date][sensorName][hour] += measurementValue;
            }
        }
    }

    if (aggregatedData.isEmpty()) {
        qDebug() << "No data aggregated for session ID:" << m_sessionId << "for selected dates and sensors";
    } else {
        qDebug() << "Aggregated data for" << aggregatedData.size() << "dates and" << selectedSensorIds.size() << "sensors";
    }

    return aggregatedData;
}

/**
 * @brief Wyświetla wykresy danych dla wybranych sensorów.
 *
 * Tworzy wykresy liniowe dla każdego sensora, wyświetla statystyki (maksimum, minimum, średnia, trend)
 * i dodaje je do listy w interfejsie użytkownika.
 *
 */
void window_2_data_vis::displayCharts()//bool isLineChart)
{
    if (m_aggregatedData.isEmpty()) {
        ui->listWidget->addItem("Brak danych do wyświetlenia dla wybranych dat i sensorów.");
        return;
    }

    bool singleDay = m_selectedDates.size() == 1;
    QList<QDate> sortedDates = m_selectedDates;
    std::sort(sortedDates.begin(), sortedDates.end());

    // Przetwarzanie danych każdego sensora w osobnym wątku
    for (const QString &sensorName : m_aggregatedData[sortedDates.first()].keys()) {
        SensorChartData chartData;
        chartData.sensorName = sensorName;

        // Uruchomienie przetwarzania danych w osobnym wątku
        std::thread t([this, &chartData, sensorName, singleDay, &sortedDates]() {
            double minValue = std::numeric_limits<double>::max();
            double maxValue = std::numeric_limits<double>::lowest();
            double sum = 0.0;
            int count = 0;
            QVector<QPointF> points;

            if (singleDay) {
                QMap<int, double> hourlyData = m_aggregatedData[sortedDates.first()][sensorName];
                for (int hour = 0; hour < 24; ++hour) {
                    double value = hourlyData.value(hour, 0.0);
                    if (value != 0.0) {
                        if (value < minValue) minValue = value;
                        if (value > maxValue) maxValue = value;
                        sum += value;
                        count++;
                        points.append(QPointF(hour, value));
                    }
                }
            } else {
                QDate earliestDate = sortedDates.first();
                for (const QDate &date : sortedDates) {
                    QMap<int, double> hourlyData = m_aggregatedData[date][sensorName];
                    qint64 daysSinceEarliest = earliestDate.daysTo(date);
                    double xBase = daysSinceEarliest * 24.0;
                    for (int hour = 0; hour < 24; ++hour) {
                        double value = hourlyData.value(hour, 0.0);
                        if (value < minValue) minValue = value;
                        if (value > maxValue) maxValue = value;
                        sum += value;
                        count++;
                        double x = xBase + hour;
                        points.append(QPointF(x, value));
                    }
                }
            }

            chartData.average = count > 0 ? sum / count : 0.0;

            if (points.size() >= 2) {
                double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumXX = 0.0;
                int n = points.size();
                for (const QPointF &p : points) {
                    sumX += p.x();
                    sumY += p.y();
                    sumXY += p.x() * p.y();
                    sumXX += p.x() * p.x();
                }
                double m = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
                if (std::abs(m) < 0.01) {
                    chartData.trend = "stabilny";
                } else if (m > 0) {
                    chartData.trend = "rosnący";
                } else {
                    chartData.trend = "malejący";
                }
            } else {
                chartData.trend = "brak danych do analizy trendu";
            }

            if (count == 0) {
                minValue = 0.0;
                maxValue = 0.0;
                chartData.trend = "brak danych";
            }

            chartData.minValue = minValue;
            chartData.maxValue = maxValue;
            chartData.points = points;
        });

        // Oczekiwanie na zakończenie wątku
        t.join();

        // Tworzenie i wyświetlanie widżetu ze statystykami
        QWidget *statsWidget = new QWidget();
        QVBoxLayout *statsLayout = new QVBoxLayout(statsWidget);
        statsLayout->setContentsMargins(5, 5, 5, 5);
        statsLayout->setSpacing(4);

        QLabel *titleLabel = new QLabel("<b>Statystyki dla: " + chartData.sensorName + "</b>");
        QLabel *maxLabel = new QLabel(QString("<b>Wartość maksymalna:</b> %1").arg(chartData.maxValue, 0, 'f', 2));
        QLabel *minLabel = new QLabel(QString("<b>Wartość minimalna:</b> %1").arg(chartData.minValue, 0, 'f', 2));
        QLabel *avgLabel = new QLabel(QString("<b>Wartość średnia:</b> %1").arg(chartData.average, 0, 'f', 2));
        QLabel *trendLabel = new QLabel("<b>Trend:</b> " + chartData.trend);

        titleLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");
        maxLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");
        minLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");
        avgLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");
        trendLabel->setStyleSheet("font-size: 14px; color: #FFFFFF;");

        statsLayout->addWidget(titleLabel);
        statsLayout->addWidget(maxLabel);
        statsLayout->addWidget(minLabel);
        statsLayout->addWidget(avgLabel);
        statsLayout->addWidget(trendLabel);

        QListWidgetItem *statsItem = new QListWidgetItem();
        statsItem->setSizeHint(QSize(0, 150));
        ui->listWidget->addItem(statsItem);
        ui->listWidget->setItemWidget(statsItem, statsWidget);

        // Tworzenie wykresu
        m_chart = new QChart();
        m_chart->setTitle(chartData.sensorName);
        m_chart->setMargins(QMargins(50, 50, 50, 50));

        QLineSeries *series = new QLineSeries();
        series->setName(chartData.sensorName);
        series->setPen(QPen(Qt::blue, 2));
        series->setPointsVisible(true);
        series->setPointLabelsVisible(true);
        series->setPointLabelsFormat("@yPoint");
        series->setPointLabelsClipping(false);

        double maxY = chartData.maxValue;

        for (const QPointF &point : chartData.points) {
            series->append(point);
        }

        if (singleDay) {
            QValueAxis *axisX = new QValueAxis();
            axisX->setTitleText("Czas (godziny)");
            axisX->setRange(0, 23);
            axisX->setTickCount(24);
            axisX->setLabelFormat("%d");
            axisX->setGridLineVisible(true);
            axisX->setLabelsFont(QFont("Arial", 10, QFont::Bold));
            axisX->setTitleFont(QFont("Arial", 12, QFont::Bold));
            axisX->setLinePen(QPen(Qt::black, 2));
            axisX->setGridLinePen(QPen(Qt::gray, 1, Qt::DashLine));

            QValueAxis *axisY = new QValueAxis();
            axisY->setTitleText(chartData.sensorName);
            double yRange = maxY * 0.1;
            double yMin = 0.0;
            double yMax = maxY + yRange;
            if (yMax <= 1.0) {
                yMax = 10.0;
            }
            axisY->setRange(yMin, yMax);
            int tickCount = std::min(10, std::max(5, static_cast<int>(yMax / 5)));
            axisY->setTickCount(tickCount);
            axisY->setLabelFormat("%.2f");
            axisY->setGridLineVisible(true);
            axisY->setLabelsFont(QFont("Arial", 10, QFont::Bold));
            axisY->setTitleFont(QFont("Arial", 12, QFont::Bold));
            axisY->setLinePen(QPen(Qt::black, 2));
            axisY->setGridLinePen(QPen(Qt::gray, 1, Qt::DashLine));

            m_chart->addSeries(series);
            m_chart->addAxis(axisX, Qt::AlignBottom);
            m_chart->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis(axisX);
            series->attachAxis(axisY);

            for (int i = 0; i < chartData.points.size(); ++i) {
                if (chartData.points[i].y() > 0) {
                    QPointF pos = m_chart->mapToPosition(chartData.points[i], series);
                    QGraphicsEllipseItem *dot = new QGraphicsEllipseItem(m_chart);
                    dot->setRect(pos.x() - 4, pos.y() - 4, 8, 8);
                    dot->setBrush(QBrush(Qt::red));
                    dot->setPen(QPen(Qt::black, 1));
                    QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(chartData.points[i].y(), 'f', 2), m_chart);
                    label->setFont(QFont("Arial", 8));
                    label->setDefaultTextColor(Qt::black);
                    label->setPos(pos.x() - label->boundingRect().width() / 2,
                                  pos.y() - label->boundingRect().height() - 5);
                }
            }
        } else {
            QCategoryAxis *axisX = new QCategoryAxis();
            axisX->setTitleText("Data i czas");
            axisX->setGridLineVisible(true);
            axisX->setLabelsFont(QFont("Arial", 10, QFont::Bold));
            axisX->setTitleFont(QFont("Arial", 12, QFont::Bold));
            axisX->setLinePen(QPen(Qt::black, 2));
            axisX->setGridLinePen(QPen(Qt::gray, 1, Qt::DashLine));

            QDate earliestDate = sortedDates.first();
            for (const QDate &date : sortedDates) {
                qint64 daysSinceEarliest = earliestDate.daysTo(date);
                double xStart = daysSinceEarliest * 24.0;
                double xEnd = xStart + 23.0;
                axisX->append(date.toString("yyyy-MM-dd"), xEnd);
                for (int hour = 0; hour < 24; ++hour) {
                    double xHour = xStart + hour;
                    axisX->append(QString("%1 %2").arg(date.toString("yyyy-MM-dd")).arg(hour, 2, 10, QChar('0')), xHour);
                }
            }

            QDate latestDate = sortedDates.last();
            qint64 daysSinceEarliest = earliestDate.daysTo(latestDate);
            double xEnd = (daysSinceEarliest + 1) * 24.0;
            axisX->setRange(0, xEnd);

            QValueAxis *axisY = new QValueAxis();
            axisY->setTitleText(chartData.sensorName);
            double yRange = maxY * 0.1;
            double yMin = 0.0;
            double yMax = maxY + yRange;
            if (yMax <= 1.0) {
                yMax = 10.0;
            }
            axisY->setRange(yMin, yMax);
            int tickCount = std::min(10, std::max(5, static_cast<int>(yMax / 5)));
            axisY->setTickCount(tickCount);
            axisY->setLabelFormat("%.2f");
            axisY->setGridLineVisible(true);
            axisY->setLabelsFont(QFont("Arial", 10, QFont::Bold));
            axisY->setTitleFont(QFont("Arial", 12, QFont::Bold));
            axisY->setLinePen(QPen(Qt::black, 2));
            axisY->setGridLinePen(QPen(Qt::gray, 1, Qt::DashLine));

            m_chart->addSeries(series);
            m_chart->addAxis(axisX, Qt::AlignBottom);
            m_chart->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis(axisX);
            series->attachAxis(axisY);

            for (int i = 0; i < chartData.points.size(); ++i) {
                if (chartData.points[i].y() > 0) {
                    QPointF pos = m_chart->mapToPosition(chartData.points[i], series);
                    QGraphicsEllipseItem *dot = new QGraphicsEllipseItem(m_chart);
                    dot->setRect(pos.x() - 4, pos.y() - 4, 8, 8);
                    dot->setBrush(QBrush(Qt::red));
                    dot->setPen(QPen(Qt::black, 1));
                    QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(chartData.points[i].y(), 'f', 2), m_chart);
                    label->setFont(QFont("Arial", 8));
                    label->setDefaultTextColor(Qt::black);
                    label->setPos(pos.x() - label->boundingRect().width() / 2,
                                  pos.y() - label->boundingRect().height() - 5);
                }
            }
        }

        m_chartView = new QChartView(m_chart);
        m_chartView->setRenderHint(QPainter::Antialiasing);
        m_chartView->setMinimumSize(600, 400);

        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(QSize(600, 400));
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, m_chartView);
    }
}
