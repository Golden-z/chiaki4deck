#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <regex>

#include "chiaki/log.h"

class JsonUtils {
    public:
        static std::string getJsonAttributeBasic(ChiakiLog* log, std::string url, std::string username, std::string password, std::string jsonAttribute){
            return getJsonAttribute(log, url, generateBasicAuthHeader(username, password), jsonAttribute);
        }

        static std::string getJsonAttributeBearer(ChiakiLog* log, std::string url, std::string token, std::string jsonAttribute){
            return getJsonAttribute(log, url, "Bearer "+token, jsonAttribute);
        }

        static std::string postJsonAttributeBasic(ChiakiLog* log, std::string url, std::string username, std::string password, std::string jsonAttribute, std::string body, std::string contentType){
            return postJsonAttribute(log, url, generateBasicAuthHeader(username, password), jsonAttribute, body, contentType);
        }

        static std::string postJsonAttributeBearer(ChiakiLog* log, std::string url, std::string token, std::string jsonAttribute, std::string body, std::string contentType){
            return postJsonAttribute(log, url, "Bearer "+token, jsonAttribute, body, contentType);
        }

        static QJsonDocument getResponseBodyBasic(ChiakiLog* log, std::string url, std::string username, std::string password){
            return responseBody(log, url, generateBasicAuthHeader(username, password), "", "application.json");
        }

        static QJsonDocument getResponseBodyBearer(ChiakiLog* log, std::string url, std::string token){
            return responseBody(log, url, "Bearer "+token, "", "application.json");
        }

        static QJsonDocument postResponseBodyBasic(ChiakiLog* log, std::string url, std::string username, std::string password, std::string body){
            return responseBody(log, url, generateBasicAuthHeader(username, password), body, "application.json");
        }

        static QJsonDocument postResponseBodyBearer(ChiakiLog* log, std::string url, std::string token, std::string body, std::string contentType){
            return responseBody(log, url, "Bearer "+token, body, contentType);

        }
    private:
        static std::string generateBasicAuthHeader(std::string username, std::string password) {
            QString q_username = QString::fromStdString(username);
            QString q_password = QString::fromStdString(password);
            QString combined = QString("%1:%2").arg(QString(q_username)).arg(QString(q_password));
            QString authHeader = "Basic " + combined.toUtf8().toBase64();
            return authHeader.toStdString();
        }

        static std::string getJsonValueforAttribute(std::string jsonResponse, std::string jsonAttribute) {
            std::string key = "\"";
            key.append(jsonAttribute);
            key.append("\":\"");

            size_t keyStart = jsonResponse.find(key);

            if (keyStart != std::string::npos) {
                keyStart += key.length();
                size_t keyEnd = jsonResponse.find('\"', keyStart);
                if (keyEnd != std::string::npos) {
                    return jsonResponse.substr(keyStart, keyEnd - keyStart);
                }
            }

            // Return an empty string if access_token is not found
            return "";
        }

        static std::vector<std::string> getJsonValuesforAttribute(std::string jsonResponse, std::string jsonAttribute) {
            // Your regular expression pattern with groups
            std::regex regex_pattern("\""+jsonAttribute+"\":\"([^\"]+)\"");

            // Iterator for matching
            std::sregex_iterator iter(jsonResponse.begin(), jsonResponse.end(), regex_pattern);
            std::sregex_iterator end;

            // Vector to store group 1 of each match
            std::vector<std::string> result_vector;

            // Iterate over matches and store group 1 in the vector
            while (iter != end) {
                std::string value = (*iter)[1].str();
                result_vector.emplace_back(value);
                ++iter;
            }

            return result_vector;
        }

        static std::string getJsonAttribute(ChiakiLog* log, std::string url, std::string authHeader, std::string jsonAttribute){
            QJsonDocument jsonResponse = responseBody(log, url, authHeader, "", "application/json");
            QJsonObject object = jsonResponse.object();
            if (object.contains(QString::fromStdString(jsonAttribute))) {
                return object.value(QString::fromStdString(jsonAttribute)).toString().toStdString();
            }
            return nullptr;
        }

        static std::string postJsonAttribute(ChiakiLog* log, std::string url, std::string authHeader, std::string jsonAttribute, std::string body, std::string contentType) {
            QJsonDocument jsonResponse = responseBody(log, url, authHeader, body, contentType);
            QJsonObject object = jsonResponse.object();
            if (object.contains(QString::fromStdString(jsonAttribute))) {
                return object.value(QString::fromStdString(jsonAttribute)).toString().toStdString();
            }
            return nullptr;
        }

        static QJsonDocument responseBody(ChiakiLog* log, std::string url, std::string authHeader, std::string body, std::string contentType) {
            contentType = (contentType.empty()) ? "application/json" : contentType;

            // Create a QNetworkAccessManager
            QNetworkAccessManager manager;

            // Create a QNetworkRequest with the URL you want to make the request to
            QUrl q_url(QString::fromStdString(url));
            QNetworkRequest request(q_url);

            // Add custom headers to the request
            request.setRawHeader("Content-Type", QString::fromStdString(contentType).toUtf8());
            request.setRawHeader("Authorization", QString::fromStdString(authHeader).toUtf8());

            // Make the request
            QNetworkReply *reply;
            if (body.empty()) {
                reply = manager.get(request);
            } else {
                QByteArray postData = QString::fromStdString(body).toUtf8();
                reply = manager.post(request, postData);
            }

            // Setup a QEventLoop to wait for the request to finish
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            QJsonDocument response;
            // Check for errors
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray responseData = reply->readAll();
                response = QJsonDocument::fromJson(responseData);
            } else {
                CHIAKI_LOGI(log, "Error:  %s", reply->errorString().toStdString().c_str());
            }

            // Clean up
            delete reply;

            return response;
        }
};

#endif //JSONUTILS_H
