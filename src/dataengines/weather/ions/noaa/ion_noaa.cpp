/*
    SPDX-FileCopyrightText: 2007-2009, 2019 Shawn Starr <shawn.starr@rogers.com>
    SPDX-FileCopyrightText: 2024 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for NOAA's National Weather Service openAPI data */

#include "ion_noaa.h"

#include "ion_noaadebug.h"

#include <KIO/TransferJob>
#include <KLocalizedString>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QStandardPaths>
#include <QTimeZone>

using namespace Qt::StringLiterals;
using namespace KUnitConversion;

QMap<QString, IonInterface::ConditionIcons> NOAAIon::setupConditionIconMappings() const
{
    QMap<QString, ConditionIcons> conditionList;
    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> const &NOAAIon::conditionIcons() const
{
    static QMap<QString, ConditionIcons> const condval = setupConditionIconMappings();
    return condval;
}

// ctor, dtor
NOAAIon::NOAAIon(QObject *parent)
    : IonInterface(parent)
{
    // Schedule the API calls according to the previous information required
    connect(this, &NOAAIon::locationUpdated, this, &NOAAIon::getObservation);
    connect(this, &NOAAIon::locationUpdated, this, &NOAAIon::getPointsInfo);
    connect(this, &NOAAIon::pointsInfoUpdated, this, &NOAAIon::getForecast);
    connect(this, &NOAAIon::pointsInfoUpdated, this, &NOAAIon::getAlerts);

    // Get the list of stations for search, location and observation data
    getStationList();
}

void NOAAIon::reset()
{
    m_sourcesToReset = sources();
    getStationList();
}

NOAAIon::~NOAAIon()
{
    // seems necessary to avoid crash
    removeAllSources();
}

QStringList NOAAIon::validate(const QString &source) const
{
    QStringList placeList;
    QString station;
    QString sourceNormalized = source.toUpper();

    QHash<QString, NOAAIon::StationInfo>::const_iterator it = m_places.constBegin();
    // If the source name might look like a station ID, check these too and return the name
    bool checkState = source.count() == 2;

    while (it != m_places.constEnd()) {
        if (checkState) {
            if (it.value().stateName == source) {
                placeList.append(u"place|"_s.append(it.key()));
            }
        } else if (it.key().toUpper().contains(sourceNormalized)) {
            placeList.append(u"place|"_s.append(it.key()));
        } else if (it.value().stationID == sourceNormalized) {
            station = u"place|"_s.append(it.key());
        }

        ++it;
    }

    placeList.sort();
    if (!station.isEmpty()) {
        placeList.prepend(station);
    }

    return placeList;
}

bool NOAAIon::updateIonSource(const QString &source)
{
    // We expect the applet to send the source in the following tokenization:
    // ionname:validate:place_name - Triggers validation of place
    // ionname:weather:place_name - Triggers receiving weather of place

    QStringList sourceAction = source.split(QLatin1Char('|'));

    // Guard: if the size of array is not 2 then we have bad data, return an error
    if (sourceAction.size() < 2) {
        setData(source, u"validate"_s, u"noaa|malformed"_s);
        return true;
    }

    if (sourceAction[1] == "validate"_L1 && sourceAction.size() > 2) {
        QStringList result = validate(sourceAction[2]);

        if (result.size() == 1) {
            setData(source, u"validate"_s, u"noaa|valid|single|"_s.append(result.join(QLatin1Char('|'))));
            return true;
        }
        if (result.size() > 1) {
            setData(source, u"validate"_s, u"noaa|valid|multiple|"_s.append(result.join(QLatin1Char('|'))));
            return true;
        }
        // result.size() == 0
        setData(source, u"validate"_s, u"noaa|invalid|single|"_s.append(sourceAction[2]));
        return true;
    }

    if (sourceAction[1] == "weather"_L1 && sourceAction.size() > 2) {
        setUpStation(source);
        return true;
    }

    setData(source, u"validate"_s, u"noaa|malformed"_s);
    return true;
}

KJob *NOAAIon::requestAPIJob(const QString &source, const QUrl &url, Callback onResult)
{
    KIO::TransferJob *getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);

    m_jobData.insert(getJob, QByteArray());

    qCDebug(IONENGINE_NOAA) << "Requesting URL:" << url;

    connect(getJob, &KIO::TransferJob::data, this, [this](KIO::Job *job, const QByteArray &data) {
        if (data.isEmpty() || !m_jobData.contains(job)) {
            return;
        }
        m_jobData[job].append(data);
    });

    if (!onResult) {
        return getJob;
    }

    connect(getJob, &KJob::result, this, [this, source, onResult](KJob *job) {
        if (job->error()) {
            qCWarning(IONENGINE_NOAA) << "Error retrieving data" << job->errorText();
            m_jobData.remove(job);
            return;
        }

        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(m_jobData.value(job), &jsonError);
        m_jobData.remove(job);

        if (doc.isNull()) {
            qCWarning(IONENGINE_NOAA) << "Received invalid JSON data:" << jsonError.errorString();
            return;
        }

        // Invoke callback method
        (this->*onResult)(source, doc);
    });

    return getJob;
}

// Opens and reads the list of weather stations, which provide a place name,
// the station ID code and the coordinates of the station
void NOAAIon::getStationList()
{
    const QString stationsFileName = u"plasma/weather_legacy/noaa_station_list.xml"_s;
    const QString stationsPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, stationsFileName);

    if (stationsPath.isEmpty()) {
        qCWarning(IONENGINE_NOAA) << "Couldn't find file" << stationsFileName << "on the local data path";
        return;
    }

    QFile stationsFile(stationsPath);
    if (!stationsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(IONENGINE_NOAA) << "Couldn't open stations file:" << stationsPath << stationsFile.errorString();
        return;
    }

    QXmlStreamReader reader = QXmlStreamReader(&stationsFile);
    const bool success = readStationList(reader);
    setInitialized(success);
    stationsFile.close();

    for (const QString &source : std::as_const(m_sourcesToReset)) {
        updateSourceEvent(source);
    }
}

void NOAAIon::setUpStation(const QString &source)
{
    QString dataKey = source;
    dataKey.remove(u"noaa|weather|"_s);
    // If this is empty we have no valid data, send out an error and abort.
    if (!m_places.contains(dataKey)) {
        setData(source, u"validate"_s, u"noaa|malformed"_s);
        return;
    }

    const StationInfo &station = m_places.value(dataKey);
    WeatherData &data = m_weatherData[source];

    data.locationName = station.stationName;
    data.stationID = station.stationID;
    data.stationLongitude = station.location.x();
    data.stationLatitude = station.location.y();

    qCDebug(IONENGINE_NOAA) << "Established station:" << data.locationName << data.stationID << data.stationLatitude << data.stationLongitude;

    Q_EMIT locationUpdated(source);
}

// handle when no XML tag is found
void NOAAIon::parseUnknownElement(QXmlStreamReader &xml) const
{
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            parseUnknownElement(xml);
        }
    }
}

void NOAAIon::parseStationID(QXmlStreamReader &xml)
{
    QString state;
    QString stationName;
    QString stationID;
    float latitude = qQNaN();
    float longitude = qQNaN();

    while (!xml.atEnd()) {
        xml.readNext();

        const auto elementName = xml.name();

        if (xml.isEndElement() && elementName == "station"_L1) {
            if (!stationID.isEmpty()) {
                NOAAIon::StationInfo info;
                info.stateName = state;
                info.stationName = stationName;
                info.stationID = stationID;
                info.location = QPointF(longitude, latitude);

                QString key = "%1, %2"_L1.arg(stationName, state);
                m_places[key] = info;
            }
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == "station_id"_L1) {
                stationID = xml.readElementText();
            } else if (elementName == "state"_L1) {
                state = xml.readElementText();
            } else if (elementName == "station_name"_L1) {
                stationName = xml.readElementText();
            } else if (elementName == "latitude"_L1) {
                latitude = xml.readElementText().toFloat();
            } else if (elementName == "longitude"_L1) {
                longitude = xml.readElementText().toFloat();
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

// Parse the city list and store into a QMap
bool NOAAIon::readStationList(QXmlStreamReader &xml)
{
    bool success = false;
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("wx_station_index")) {
                success = true;
            } else if (xml.name() == "station"_L1) {
                parseStationID(xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }

    return (!xml.error() && success);
}

void NOAAIon::getObservation(const QString &source)
{
    const QString stationID = m_weatherData[source].stationID;
    requestAPIJob(source, //
                  QUrl(u"https://api.weather.gov/stations/%1/observations/latest"_s.arg(stationID)),
                  &NOAAIon::readObservation);
}

void NOAAIon::readObservation(const QString &source, const QJsonDocument &doc)
{
    const QJsonValue properties = doc[u"properties"_s];
    if (!properties.isObject()) {
        qCWarning(IONENGINE_NOAA) << "Malformed observation report" << doc;
        return;
    }

    WeatherData::Observation &data = m_weatherData[source].observation;
    data = WeatherData::Observation{};

    data.weather = properties[u"textDescription"_s].toString();
    data.timestamp = QDateTime::fromString(properties[u"timestamp"_s].toString(), Qt::ISODate);
    data.isNight = isNightTime(data.timestamp, m_weatherData[source].stationLatitude, m_weatherData[source].stationLongitude);

    data.temperature_F = parseQV(properties[u"temperature"_s], Fahrenheit);
    data.humidity = parseQV(properties[u"relativeHumidity"_s], Percent);
    data.pressure = parseQV(properties[u"barometricPressure"_s], InchesOfMercury);
    data.visibility = parseQV(properties[u"visibility"_s], Mile);

    data.windDirection = parseQV(properties[u"windDirection"_s], Degree);
    data.windSpeed = parseQV(properties[u"windSpeed"_s], MilePerHour);
    data.windGust = parseQV(properties[u"windGust"_s], MilePerHour);

    data.dewpoint_F = parseQV(properties[u"dewpoint"_s], Fahrenheit);
    data.heatindex_F = parseQV(properties[u"heatIndex"_s], Fahrenheit);
    data.windchill_F = parseQV(properties[u"windChill"_s], Fahrenheit);

    qCDebug(IONENGINE_NOAA) << "Received observation data:" << data.timestamp << data.weather;

    Q_EMIT observationUpdated(source);
}

void NOAAIon::updateWeather(const QString &source)
{
    const WeatherData &weatherData = m_weatherData[source];
    const WeatherData::Observation &current = weatherData.observation;

    if (weatherData.isForecastsDataPending) {
        return;
    }

    Plasma5Support::DataEngine::Data data;

    data.insert(u"Place"_s, weatherData.locationName);
    data.insert(u"Station"_s, weatherData.stationID);

    const bool stationCoordValid = (!qIsNaN(weatherData.stationLatitude) && !qIsNaN(weatherData.stationLongitude));

    if (stationCoordValid) {
        data.insert(u"Latitude"_s, weatherData.stationLatitude);
        data.insert(u"Longitude"_s, weatherData.stationLongitude);
    }

    // Real weather - Current conditions
    if (current.timestamp.isValid()) {
        data.insert(u"Observation Timestamp"_s, current.timestamp);
    }

    const QString conditionI18n = current.weather.isEmpty() ? i18n("N/A") : i18nc("weather condition", current.weather.toUtf8().data());

    data.insert(u"Current Conditions"_s, conditionI18n);
    qCDebug(IONENGINE_NOAA) << "i18n condition string: " << qPrintable(conditionI18n);

    const QString weather = current.weather.toLower();
    ConditionIcons condition = getConditionIcon(weather, !current.isNight);
    data.insert(u"Condition Icon"_s, getWeatherIcon(condition));

    if (!qIsNaN(current.temperature_F)) {
        data.insert(u"Temperature"_s, current.temperature_F);
    }

    // Used for all temperatures
    data.insert(u"Temperature Unit"_s, Fahrenheit);

    if (!qIsNaN(current.windchill_F)) {
        data.insert(u"Windchill"_s, current.windchill_F);
    }

    if (!qIsNaN(current.heatindex_F)) {
        data.insert(u"Heat Index"_s, current.heatindex_F);
    }

    if (!qIsNaN(current.dewpoint_F)) {
        data.insert(u"Dewpoint"_s, current.dewpoint_F);
    }

    if (!qIsNaN(current.pressure)) {
        data.insert(u"Pressure"_s, current.pressure);
        data.insert(u"Pressure Unit"_s, InchesOfMercury);
    }

    if (!qIsNaN(current.visibility)) {
        data.insert(u"Visibility"_s, current.visibility);
        data.insert(u"Visibility Unit"_s, Mile);
    }

    if (!qIsNaN(current.humidity)) {
        data.insert(u"Humidity"_s, current.humidity);
        data.insert(u"Humidity Unit"_s, Percent);
    }

    if (!qIsNaN(current.windSpeed)) {
        data.insert(u"Wind Speed"_s, current.windSpeed);
    }

    if (!qIsNaN(current.windSpeed) || !qIsNaN(current.windGust)) {
        data.insert(u"Wind Speed Unit"_s, MilePerHour);
    }

    if (!qIsNaN(current.windGust)) {
        data.insert(u"Wind Gust"_s, current.windGust);
    }

    if (!qIsNaN(current.windSpeed) && qFuzzyIsNull(current.windSpeed)) {
        data.insert(u"Wind Direction"_s, u"VR"_s); // Variable/calm
    } else if (!qIsNaN(current.windDirection)) {
        data.insert(u"Wind Direction"_s, windDirectionFromAngle(current.windDirection));
    }

    // Daily forecasts
    int forecastDay = 0;
    for (const WeatherData::Forecast &forecast : weatherData.forecasts) {
        ConditionIcons icon = getConditionIcon(forecast.summary.toLower(), forecast.isDayTime);
        QString iconName = getWeatherIcon(icon);

        // Sometimes the forecast for the later days is unavailable, so skip
        // remianing days since their forecast data is probably unavailable.
        if (forecast.summary.isEmpty()) {
            break;
        }

        // Indicate whether the first forcast is a nightly one
        if (forecastDay == 0) {
            data.insert(u"Forecast Starts at Night"_s, !forecast.isDayTime);
        }

        // Get the short day name for the forecast
        data.insert(u"Short Forecast Day %1"_s.arg(forecastDay),
                    u"%1|%2|%3|%4|%5|%6"_s.arg(forecast.day,
                                               iconName,
                                               i18nForecast(forecast.summary),
                                               qIsNaN(forecast.high) ? u"N/A"_s : QString::number(forecast.high),
                                               qIsNaN(forecast.low) ? u"N/A"_s : QString::number(forecast.low),
                                               QString::number(forecast.precipitation)));
        ++forecastDay;
    }
    // Set the number of days we provide after the filtering
    data.insert(u"Total Weather Days"_s, forecastDay);

    data.insert(u"Total Warnings Issued"_s, weatherData.alerts.size());
    int alertNum = 0;
    for (const WeatherData::Alert &alert : weatherData.alerts) {
        // TODO: Add a Headline parameter to the engine and the applet
        data.insert(u"Warning Description %1"_s.arg(alertNum), u"<p><b>%1</b></p>%2"_s.arg(alert.headline, alert.description));
        data.insert(u"Warning Timestamp %1"_s.arg(alertNum), QLocale().toString(alert.startTime, QLocale::ShortFormat));
        data.insert(u"Warning Priority %1"_s.arg(alertNum), alert.priority);
        ++alertNum;
    }

    data.insert(u"Credit"_s, i18nc("credit line, keep string short)", "Data from NOAA National\302\240Weather\302\240Service"));

    Q_EMIT cleanUpData(source);
    setData(source, data);

    qCDebug(IONENGINE_NOAA) << "Updated weather data for" << source;
}

/**
 * Determine the condition icon based on the list of possible NOAA weather conditions as defined at
 * <https://www.weather.gov/xml/current_obs/weather.php> and
 * <https://graphical.weather.gov/xml/mdl/XML/Design/MDL_XML_Design.htm#_Toc141760782>
 * Since the number of NOAA weather conditions need to be fitted into the narowly defined groups in IonInterface::ConditionIcons, we
 * try to group the NOAA conditions as best as we can based on their priorities/severity.
 * TODO: summaries "Hot" & "Cold" have no proper matching entry in ConditionIcons, consider extending it
 */
IonInterface::ConditionIcons NOAAIon::getConditionIcon(const QString &weather, bool isDayTime) const
{
    IonInterface::ConditionIcons result;
    // Consider any type of storm, tornado or funnel to be a thunderstorm.
    if (weather.contains("thunderstorm"_L1) || weather.contains("funnel"_L1) || weather.contains("tornado"_L1) || weather.contains("storm"_L1)
        || weather.contains("tstms"_L1)) {
        if (weather.contains("vicinity"_L1) || weather.contains("chance"_L1)) {
            result = isDayTime ? IonInterface::ChanceThunderstormDay : IonInterface::ChanceThunderstormNight;
        } else {
            result = IonInterface::Thunderstorm;
        }

    } else if (weather.contains("pellets"_L1) || weather.contains("crystals"_L1) || weather.contains("hail"_L1)) {
        result = IonInterface::Hail;

    } else if (((weather.contains("rain"_L1) || weather.contains("drizzle"_L1) || weather.contains("showers"_L1)) && weather.contains("snow"_L1))
               || weather.contains("wintry mix"_L1)) {
        result = IonInterface::RainSnow;

    } else if (weather.contains("flurries"_L1)) {
        result = IonInterface::Flurries;

    } else if (weather.contains("snow"_L1) && weather.contains("light"_L1)) {
        result = IonInterface::LightSnow;

    } else if (weather.contains("snow"_L1)) {
        if (weather.contains("vicinity"_L1) || weather.contains("chance"_L1)) {
            result = isDayTime ? IonInterface::ChanceSnowDay : IonInterface::ChanceSnowNight;
        } else {
            result = IonInterface::Snow;
        }

    } else if (weather.contains("freezing rain"_L1)) {
        result = IonInterface::FreezingRain;

    } else if (weather.contains("freezing drizzle"_L1)) {
        result = IonInterface::FreezingDrizzle;

    } else if (weather.contains("cold"_L1)) {
        // temperature condition has not hint about air ingredients, so let's assume chance of snow
        result = isDayTime ? IonInterface::ChanceSnowDay : IonInterface::ChanceSnowNight;

    } else if (weather.contains("showers"_L1)) {
        if (weather.contains("vicinity"_L1) || weather.contains("chance"_L1)) {
            result = isDayTime ? IonInterface::ChanceShowersDay : IonInterface::ChanceShowersNight;
        } else {
            result = IonInterface::Showers;
        }
    } else if (weather.contains("light rain"_L1) || weather.contains("drizzle"_L1)) {
        result = IonInterface::LightRain;

    } else if (weather.contains("rain"_L1)) {
        result = IonInterface::Rain;

    } else if (weather.contains("few clouds"_L1) || weather.contains("mostly sunny"_L1) || weather.contains("mostly clear"_L1)
               || weather.contains("increasing clouds"_L1) || weather.contains("becoming cloudy"_L1) || weather.contains("clearing"_L1)
               || weather.contains("decreasing clouds"_L1) || weather.contains("becoming sunny"_L1)) {
        if (weather.contains("breezy"_L1) || weather.contains("wind"_L1) || weather.contains("gust"_L1)) {
            result = isDayTime ? IonInterface::FewCloudsWindyDay : IonInterface::FewCloudsWindyNight;
        } else {
            result = isDayTime ? IonInterface::FewCloudsDay : IonInterface::FewCloudsNight;
        }

    } else if (weather.contains("partly cloudy"_L1) || weather.contains("partly sunny"_L1) || weather.contains("partly clear"_L1)) {
        if (weather.contains("breezy"_L1) || weather.contains("wind"_L1) || weather.contains("gust"_L1)) {
            result = isDayTime ? IonInterface::PartlyCloudyWindyDay : IonInterface::PartlyCloudyWindyNight;
        } else {
            result = isDayTime ? IonInterface::PartlyCloudyDay : IonInterface::PartlyCloudyNight;
        }

    } else if (weather.contains("overcast"_L1) || weather.contains("cloudy"_L1)) {
        if (weather.contains("breezy"_L1) || weather.contains("wind"_L1) || weather.contains("gust"_L1)) {
            result = IonInterface::OvercastWindy;
        } else {
            result = IonInterface::Overcast;
        }

    } else if (weather.contains("haze"_L1) || weather.contains("smoke"_L1) || weather.contains("dust"_L1) || weather.contains("sand"_L1)) {
        result = IonInterface::Haze;

    } else if (weather.contains("fair"_L1) || weather.contains("clear"_L1) || weather.contains("sunny"_L1)) {
        if (weather.contains("breezy"_L1) || weather.contains("wind"_L1) || weather.contains("gust"_L1)) {
            result = isDayTime ? IonInterface::ClearWindyDay : IonInterface::ClearWindyNight;
        } else {
            result = isDayTime ? IonInterface::ClearDay : IonInterface::ClearNight;
        }

    } else if (weather.contains("fog"_L1)) {
        result = IonInterface::Mist;

    } else if (weather.contains("hot"_L1)) {
        // temperature condition has not hint about air ingredients, so let's assume the sky is clear when it is hot
        if (weather.contains("breezy"_L1) || weather.contains("wind"_L1) || weather.contains("gust"_L1)) {
            result = isDayTime ? IonInterface::ClearWindyDay : IonInterface::ClearWindyNight;
        } else {
            result = isDayTime ? IonInterface::ClearDay : IonInterface::ClearNight;
        }

    } else if (weather.contains("breezy"_L1) || weather.contains("wind"_L1) || weather.contains("gust"_L1)) {
        // Assume a clear sky when it's windy but no clouds have been mentioned
        result = isDayTime ? IonInterface::ClearWindyDay : IonInterface::ClearWindyNight;
    } else {
        result = IonInterface::NotAvailable;
    }

    return result;
}

QString NOAAIon::i18nForecast(const QString &summary) const
{
    const QStringList conditions = summary.split(u" then "_s, Qt::SkipEmptyParts, Qt::CaseInsensitive);

    QStringList i18nSummary;
    for (const auto &condition : conditions) {
        i18nSummary << i18nc("weather forecast", condition.toUtf8().data());
    }

    // i18n: The forecast summary can include several single conditions that follow
    // a temporary sequence, separated by " then ". Also include spaces if necessary.
    // If there is no suitable separtor, a sentence separator might do.
    const QString separator = i18nc("Separator between forecast conditions that follow a temporal sequence (ex. \"Rain then Sunny\")", " then ");
    return i18nSummary.join(separator);
}

/* UnitOfMeasure
 * https://www.weather.gov/documentation/services-web-api
 * pattern: ^((wmo|uc|wmoUnit|nwsUnit):)?.*$
 * A string denoting a unit of measure, expressed in the format "{unit}" or "{namespace}:{unit}".
 * Units with the namespace "wmo" or "wmoUnit" are defined in the World Meteorological Organization Codes Registry at
 * http://codes.wmo.int/common/unit and should be canonically resolvable to http://codes.wmo.int/common/unit/{unit}.
 * Units with the namespace "nwsUnit" are currently custom and do not align to any standard.
 * Units with no namespace or the namespace "uc" are compliant with the Unified Code for Units of Measure
 * syntax defined at https://unitsofmeasure.org/. This also aligns with recent versions of the Geographic
 * Markup Language (GML) standard, the IWXXM standard, and OGC Observations and Measurements v2.0 (ISO/DIS 19156).
 * Namespaced units are considered deprecated. We will be aligning API to use the same standards as GML/IWXXM in the future.
 */
UnitId NOAAIon::parseUnit(const QString &unitCode) const
{
    const auto unitsMap = std::map<QString, UnitId>{
        // Simple deprecated "Temperature Unit" string
        {u"F"_s, Fahrenheit},
        {u"C"_s, Celsius},
        // WMO
        {u"wmoUnit:degC"_s, Celsius},
        {u"wmoUnit:percent"_s, Percent},
        {u"wmoUnit:km_h-1"_s, KilometerPerHour},
        {u"wmoUnit:Pa"_s, Pascal},
        {u"wmoUnit:m"_s, Meter},
        {u"wmoUnit:mm"_s, Millimeter},
        {u"wmoUnit:degree_(angle)"_s, Degree},
    };

    QString unit = unitCode;
    unit.replace(u"wmo:"_s, u"wmoUnit:"_s);
    unit.replace(u"uc:"_s, u""_s);

    if (!unitsMap.contains(unit)) {
        qCWarning(IONENGINE_NOAA) << "Couldn't parse remote unit" << unitCode;
        return InvalidUnit;
    }

    return unitsMap.at(unit);
}

/* QuantitativeValue
 * https://www.weather.gov/documentation/services-web-api
 */
float NOAAIon::parseQV(const QJsonValue &qv, UnitId destUnit) const
{
    if (qv.isNull() || !qv.isObject()) {
        return qQNaN();
    }

    const float value = qv[u"value"_s].toDouble(qQNaN());
    const UnitId unit = parseUnit(qv[u"unitCode"_s].toString());

    // We don't need or we can't make a conversion
    if (qIsNaN(value) || unit == destUnit || unit == InvalidUnit || destUnit == InvalidUnit) {
        return value;
    }

    return m_converter.convert({value, unit}, destUnit).number();
}

QString NOAAIon::windDirectionFromAngle(float degrees) const
{
    if (qIsNaN(degrees)) {
        return u"VR"_s;
    }

    // We have a discrete set of 16 directions, with resolution of 22.5º
    const std::array<QString, 16> directions{
        u"N"_s,
        u"NNE"_s,
        u"NE"_s,
        u"ENE"_s,
        u"E"_s,
        u"ESE"_s,
        u"SE"_s,
        u"SSE"_s,
        u"S"_s,
        u"SSW"_s,
        u"SW"_s,
        u"WSW"_s,
        u"W"_s,
        u"WNW"_s,
        u"NW"_s,
        u"NNW"_s,
    };
    const int index = qRound(degrees / 22.5) % 16;

    return directions.at(index);
}

void NOAAIon::getForecast(const QString &source)
{
    if (m_weatherData[source].forecastUrl.isEmpty()) {
        qCWarning(IONENGINE_NOAA) << "Cannot request forecast because the URL is missing";
        return;
    }

    m_weatherData[source].isForecastsDataPending = true;
    requestAPIJob(source, //
                  QUrl(m_weatherData[source].forecastUrl),
                  &NOAAIon::readForecast);
}

void NOAAIon::readForecast(const QString &source, const QJsonDocument &doc)
{
    WeatherData &weatherData = m_weatherData[source];
    weatherData.isForecastsDataPending = false;

    const QJsonValue properties = doc[u"properties"_s];
    if (!properties.isObject()) {
        qCWarning(IONENGINE_NOAA) << "Malformed forecast report" << doc;
        return;
    }

    QList<WeatherData::Forecast> &forecasts = weatherData.forecasts;
    forecasts.clear();

    const QJsonArray periods = properties[u"periods"_s].toArray();
    forecasts.reserve(periods.count());

    for (const auto &period : periods) {
        WeatherData::Forecast forecast;

        // Time period. Date and day/night flag
        const QDateTime date = QDateTime::fromString(period[u"startTime"_s].toString(), Qt::ISODate);
        forecast.day = QLocale().toString(date.date().day());
        forecast.isDayTime = period[u"isDaytime"_s].toBool();

        // The temperature reported is daytime's highest or night's lowest
        // "temperature" can be either an integer (to be deprecated) or a QuantitativeValue
        // Let's use Fahrenheit for now as the default unit for the provider
        const auto tempJson = period[u"temperature"_s];
        float tempF = qQNaN();
        if (tempJson.isObject()) {
            tempF = parseQV(tempJson, Fahrenheit);
        } else {
            const auto temperature = Value(tempJson.toInt(), parseUnit(period[u"temperatureUnit"_s].toString()));
            tempF = m_converter.convert(temperature, Fahrenheit).number();
        }

        if (forecast.isDayTime) {
            forecast.high = tempF;
        } else {
            forecast.low = tempF;
        }

        // Precipitation (%)
        forecast.precipitation = period[u"probabilityOfPrecipitation"_s][u"value"_s].toInt();

        // Weather conditions
        forecast.summary = period[u"shortForecast"].toString();

        forecasts << forecast;
    }

    qCDebug(IONENGINE_NOAA) << "Received forecast data:" << forecasts.count() << "periods."
                            << "Starts at night:" << (!forecasts.isEmpty() && !forecasts.first().isDayTime);

    updateWeather(source);
}

void NOAAIon::getPointsInfo(const QString &source)
{
    const double lat = m_weatherData[source].stationLatitude;
    const double lon = m_weatherData[source].stationLongitude;
    if (qIsNaN(lat) || qIsNaN(lon)) {
        qCWarning(IONENGINE_NOAA) << "Cannot request grid info because the lat/lon coordinates are missing";
    }

    requestAPIJob(source, //
                  QUrl(u"https://api.weather.gov/points/%1,%2"_s.arg(lat).arg(lon)),
                  &NOAAIon::readPointsInfo);
}

void NOAAIon::readPointsInfo(const QString &source, const QJsonDocument &doc)
{
    const auto properties = doc[u"properties"_s];
    if (!properties.isObject()) {
        qCWarning(IONENGINE_NOAA) << "Malformed points information" << doc;
        return;
    }

    m_weatherData[source].forecastUrl = properties[u"forecast"_s].toString();

    // County ID, used to retrieve alerts
    const QString countyUrl = properties[u"county"_s].toString();
    const QString countyID = countyUrl.split('/'_L1).last();
    m_weatherData[source].countyID = countyID;

    Q_EMIT pointsInfoUpdated(source);
}

void NOAAIon::getAlerts(const QString &source)
{
    // We get the alerts by county because it includes all the events.
    // Using the forecast zone would miss some of them, and the lat/lon point
    // corresponds to the weather station, not necessarily the user location
    const QString &countyID = m_weatherData[source].countyID;
    if (countyID.isEmpty()) {
        qCWarning(IONENGINE_NOAA) << "Cannot request alerts because the county ID is missing";
        return;
    }

    requestAPIJob(source, //
                  QUrl(u"https://api.weather.gov/alerts/active?zone=%1"_s.arg(countyID)),
                  &NOAAIon::readAlerts);
}

// Helpers to parse warnings
int mapSeverity(const QString &severity)
{
    if (severity == "Extreme"_L1) {
        return 4;
    } else if (severity == "Severe"_L1) {
        return 3;
    } else if (severity == "Moderate"_L1) {
        return 2;
    } else if (severity == "Minor"_L1) {
        return 1;
    } else { // severity: "Unknown"
        return 0;
    }
};

QString formatAlertDescription(QString description)
{
    /* -- Example of an alert's description --
    * WHAT...Minor flooding is occurring and minor flooding is forecast.\n
    \n
    * WHERE...Santee River near Jamestown.\n
    \n
    * WHEN...Until further notice.\n
    \n
    * IMPACTS...At 12.0 feet, several dirt logging roads are impassable.\n
    \n
    * ADDITIONAL DETAILS...\n
    - At 930 PM EST Tuesday, the stage was 11.4 feet.\n
    - Forecast...The river is expected to rise to a crest of 11.7\n
    feet Thursday evening.\n
    - Flood stage is 10.0 feet.\n
    */
    description.replace("* "_L1, "<b>"_L1);
    description.replace("..."_L1, ":</b> "_L1);
    description.replace("\n\n"_L1, "<br/>"_L1);
    description.replace("\n-"_L1, "<br/>-"_L1);
    return description;
}

void NOAAIon::readAlerts(const QString &source, const QJsonDocument &doc)
{
    auto &alerts = m_weatherData[source].alerts;
    alerts.clear();

    const auto features = doc[u"features"_s].toArray();
    qCDebug(IONENGINE_NOAA) << u"Received %1 alert/s"_s.arg(features.count());

    for (const auto &alertInfo : features) {
        const auto properties = alertInfo[u"properties"_s];
        if (!properties.isObject()) {
            continue;
        }

        auto alert = WeatherData::Alert();
        alert.startTime = QDateTime::fromString(properties[u"onset"_s].toString(), Qt::ISODate);
        alert.endTime = QDateTime::fromString(properties[u"ends"_s].toString(), Qt::ISODate);
        alert.priority = mapSeverity(properties[u"severity"_s].toString());
        alert.headline = properties[u"parameters"_s][u"NWSheadline"_s][0].toString();
        alert.description = formatAlertDescription(properties[u"description"_s].toString());

        alerts << alert;
    }

    // Sort by higher priority and the lower start time
    std::sort(alerts.begin(), alerts.end(), [](auto a, auto b) {
        if (a.priority != b.priority) {
            return a.priority > b.priority;
        }
        return a.startTime < b.startTime;
    });

    updateWeather(source);
    forceImmediateUpdateOfAllVisualizations();
    Q_EMIT forceUpdate(this, source);
}

K_PLUGIN_CLASS_WITH_JSON(NOAAIon, "ion-noaa.json")

#include "ion_noaa.moc"

#include "moc_ion_noaa.cpp"
