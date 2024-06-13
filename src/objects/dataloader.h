#ifndef DATALOADER_H
#define DATALOADER_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QObject>
#include <QRegularExpression>
#include <QTextStream>

class DataLoader : public QObject {
    Q_OBJECT

public:
    // 单例模式
    static DataLoader &instance() {
        static DataLoader instance;
        return instance;
    }

    // 删除拷贝构造函数和赋值运算符
    DataLoader(const DataLoader &) = delete;
    DataLoader &operator=(const DataLoader &) = delete;

    // 加载数据的公共接口
    void loadData(const QString &directoryPath) {
        readMarkdownFiles(directoryPath);
        emit dataLoaded();
    }

signals:
    void dataLoaded();

private:
    DataLoader() {}

    struct MdContent {
        QString title;
        QString description;
        QString author;
        QString link;
        QString prompt;
    };

    struct JsonModel {
        QString name;
        QString intro;
        QString image;
        QJsonArray categories;
        QString prompt;
    };

    const QMap<QString, MdContent> &prompts() const { return m_prompts; }
    const QList<JsonModel> &mdoels() const { return m_models; }

    QList<JsonModel> m_models;

    QMap<QString, MdContent> m_prompts;

  // 读取 JSON 模型文件内容的私有函数
  void readJsonModelFiles(const QString &directoryPath) {
      QDir modelsDir(directoryPath);

      qDebug() << "modelsDir:" << modelsDir.dirName();
      // List all JSON files in the directory
      QStringList jsonFiles =
          modelsDir.entryList(QStringList() << "*.json", QDir::Files);

      // Iterate through each JSON file
      foreach (const QString &jsonFile, jsonFiles) {
          qDebug() << "jsonFile:" << jsonFile;

          QFile file(modelsDir.absoluteFilePath(jsonFile));

          if (!file.open(QIODevice::ReadOnly)) {
              qWarning() << "Could not open file" << jsonFile;
              continue;
          }

          QByteArray fileData = file.readAll();
          file.close();

          QJsonParseError parseError;
          QJsonDocument doc = QJsonDocument::fromJson(fileData, &parseError);

          if (parseError.error != QJsonParseError::NoError) {
              qWarning() << "Failed to parse JSON file" << jsonFile << ":"
                         << parseError.errorString();
              continue;
          }

          if (!doc.isObject()) {
              qWarning() << "Invalid JSON structure in file" << jsonFile;
              continue;
          }

          QJsonObject jsonObj = doc.object();

          JsonModel model;
          model.name = jsonObj.value("name").toString();
          model.intro = jsonObj.value("intro").toString();
          model.image = jsonObj.value("image").toString();
          model.categories = jsonObj.value("categories").toArray();
          model.prompt = jsonObj.value("prompt").toString();

          m_models.append(model);
      }
  }

  // 读取 .md 文件内容的私有函数
  void readMarkdownFiles(const QString &directoryPath) {
      QDir dir(directoryPath);

      if (!dir.exists()) {
          qWarning() << "Directory does not exist:" << directoryPath;
          return;
      }

      QStringList filters;
      filters << "*.md";
      dir.setNameFilters(filters);

      QFileInfoList fileList = dir.entryInfoList(QDir::Files);

      foreach (const QFileInfo &fileInfo, fileList) {
          QFile file(fileInfo.filePath());

          if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
              qWarning() << "Cannot open file:" << fileInfo.filePath();
              continue;
          }

          QTextStream in(&file);
          QString content = in.readAll();
          file.close();

          MdContent mdContent = parseMdContent(content);
          qDebug() << "Title:" << mdContent.title;
          qDebug() << "Description:" << mdContent.description;
          qDebug() << "Author:" << mdContent.author;
          qDebug() << "Link:" << mdContent.link;
          qDebug() << "Prompt:" << mdContent.prompt;

          m_prompts[mdContent.title] = mdContent;
      }
  }

  // 解析 .md 文件内容的私有函数
  MdContent parseMdContent(const QString &content) {
      MdContent mdContent;

      // 解析标题
      static QRegularExpression titleRegex(R"(^##\s*(.+)$)",
                                           QRegularExpression::MultilineOption);
      QRegularExpressionMatch titleMatch = titleRegex.match(content);
      if (titleMatch.hasMatch()) {
          mdContent.title = titleMatch.captured(1).trimmed();
      }

      // 解析描述
      QRegularExpression descriptionRegex(
          R"(^##\s*.+$\n(.+?)\n^By\s+.+$)",
          QRegularExpression::MultilineOption |
              QRegularExpression::DotMatchesEverythingOption);
      static QRegularExpressionMatch descriptionMatch = descriptionRegex.match(content);
      if (descriptionMatch.hasMatch()) {
          mdContent.description = descriptionMatch.captured(1).trimmed();
      }

      // 解析作者
      static QRegularExpression authorRegex(R"(^By\s+(.+)$)",
                                            QRegularExpression::MultilineOption);
      QRegularExpressionMatch authorMatch = authorRegex.match(content);
      if (authorMatch.hasMatch()) {
          mdContent.author = authorMatch.captured(1).trimmed();
      }

      // 解析链接
      static QRegularExpression linkRegex(R"((https?://[^\s]+))",
                                          QRegularExpression::MultilineOption);
      QRegularExpressionMatch linkMatch = linkRegex.match(content);
      if (linkMatch.hasMatch()) {
          mdContent.link = linkMatch.captured(1).trimmed();
      }

      // 解析 prompt
      QRegularExpression promptRegex(R"(^```markdown\n([\s\S]+)\n```$)",
                                     QRegularExpression::MultilineOption);
      static QRegularExpressionMatch promptMatch = promptRegex.match(content);
      if (promptMatch.hasMatch()) {
          mdContent.prompt = promptMatch.captured(1).trimmed();
      } else {
          // Handle case where prompt might be missing the ending ```
          int start = content.indexOf("```markdown\n");
          if (start != -1) {
              start += QString("```markdown\n").length();
              int end = content.indexOf("\n```", start);
              if (end == -1) {
                  mdContent.prompt = content.mid(start).trimmed();
              } else {
                  mdContent.prompt = content.mid(start, end - start).trimmed();
              }
          }
      }

      return mdContent;
  }
};

#endif // DATALOADER_H
