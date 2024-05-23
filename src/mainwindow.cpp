#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QDialog>
#include "ioverlaybutton.h"
#include <QLabel>
#include <QStandardItemModel>
#include "ichatwidget.h"
#include "itestwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chatbot (new ChatBot(this))
    , user(new IUserPage(this))
    , test(new ITestWidget(this))
{
    ui->setupUi(this);
    ui->chatTabs->tabBar()->hide();
    ui->sendButton->setDisabled(true);
    ui->sendButton->setStatusTip("Nothing");
    ui->expandButton->icon().addPixmap(QPixmap(":/icon/full-screen.svg"), QIcon::Normal, QIcon::On);
    ui->expandButton->icon().addPixmap(QPixmap(":/icon/full-screen-zoom.svg"), QIcon::Normal, QIcon::Off);

    setWindowIcon(QIcon("://icon/qollama.png"));

    test->hide();

    connect(ui->welcomePage, &IWelcomePage::send, this, &MainWindow::addMessage);

    connect(chatbot, &ChatBot::replyReceived, this, &MainWindow::appendWordToActiveChat);
    connect(chatbot, &ChatBot::finish, [&](){
        ui->sendButton->setStatusTip("Waiting");
        on_inputLine_textChanged(ui->inputLine->text());
        auto *chatListView = getCurrentChatList();
        if (!chatListView) {
            qDebug() << "Current chat list is null.";
            return;
        }

        IMessageWidget* curr = chatListView->getLatestMessageWidget();
        curr->finish();
    });

    connect(ui->sendButton, &QPushButton::clicked,[&](){
        if (ui->sendButton->statusTip() == "Pending") {
            chatbot->abort();
        }
        emit ui->inputLine->returnPressed();
    });

    connect(ui->inputLine, &QLineEdit::returnPressed, ui->sendButton, &QPushButton::pressed);

    connect(ui->newChatButton, &QPushButton::pressed, this, &MainWindow::addNewChat);
    connect(ui->historyList, &QListWidget::itemClicked, this, &MainWindow::on_historyListItem_clicked);
    connect(ui->expandButton, &QPushButton::pressed, this, &MainWindow::expandSideWidget);
    connect(ui->historyList, &IHistoryList::itemDeleted, [&](int row){ ui->chatTabs->removeTab(row); ui->welcomePage->show(); });

    connect(ui->shareButton, &QPushButton::pressed, test, &QWidget::show);
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::expandSideWidget()
{
    if ( ui->frameleft->isHidden() ) {
        ui->expandButton->setIcon(ui->expandButton->icon().pixmap(30, QIcon::Normal, QIcon::Off));
    }
    else {
        ui->expandButton->setIcon(ui->expandButton->icon().pixmap(30, QIcon::Normal, QIcon::On));
    }

    ui->frameleft->setVisible(!ui->frameleft->isVisible());
   // ui->expandButton->
    ui->chatTabs->updateGeometry();

    QApplication::processEvents();

    ui->welcomePage->setGeometry(ui->chatTabs->currentWidget()->geometry());
}

void MainWindow::appendWordToActiveChat(QString text)
{
    auto chatListView = getCurrentChatList();
    if (!chatListView) {
        qDebug() << "Current chat list is null.";
        return;
    }

    IMessageWidget* curr = chatListView->getLatestMessageWidget();
    curr->appendMessage(text);

    chatListView->scrollToBottom();
}

void MainWindow::addNewChat()
{

    if (ui->chatTabs->count() >= 50) {
        qDebug() << "Maximum chat tabs reached.";
        return;
    }

    IChatWidget *uniqueListWidget = getCurrentChatList();

    if (!uniqueListWidget || uniqueListWidget->isNew()) {
        qDebug() << "Current chat list is null or already contains a new chat.";
        return;
    }

    ui->chatTabs->setCurrentIndex(ui->chatTabs->count()-1);
    ui->welcomePage->show();
}

IChatWidget *MainWindow::getCurrentChatList()
{
    QWidget *currentTabWidget = ui->chatTabs->currentWidget();
    if (!currentTabWidget) {
        qDebug() << "Current tab widget is null.";
        return nullptr;
    }

    QList<IChatWidget *> listWidgets = currentTabWidget->findChildren<IChatWidget *>();
    if (listWidgets.size() != 1) {
        qDebug() << "Error: There is not exactly one IChatList widget in the current tab.";
        return nullptr;
    }

    return listWidgets.at(0);
}

void MainWindow::addMessage(QString text )
{
    if (text.isEmpty()) {
        qDebug() << "Input text is empty.";
        return;
    }

    ui->sendButton->setEnabled(true);
    ui->sendButton->setIcon(QIcon(":/icon/stop.svg"));
    ui->sendButton->setStatusTip("Pending");

    auto *chatListView = getCurrentChatList();
    auto hisItem = ui->historyList->item(ui->chatTabs->currentIndex());

    if (!chatListView || ui->welcomePage->isVisible()) {
        qDebug() << "Create new chatList.";

        auto tab = new QWidget();
        auto verticalLayout = new QVBoxLayout(tab);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        auto newChatList = new IChatWidget(tab);
        verticalLayout->addWidget(newChatList);

        ui->chatTabs->setCurrentIndex(ui->chatTabs->insertTab(0, tab, QString()));

        chatListView = newChatList;

        auto item = new QListWidgetItem(ui->historyList);
        ui->historyList->insertItem(0, item);
        hisItem = item;
    }

    ui->welcomePage->hide();

    chatListView->addMessage(ui->userButton->text(), ui->userButton->icon().pixmap(30), text);
    chatListView->addMessage(ui->comboBox->currentText(), ui->newChatButton->icon().pixmap(30), "");
    chatListView->scrollToBottom();

    if (hisItem) {
        hisItem->setToolTip(text);
    } else {
        qDebug() << "Current history list item is null.";
    }

    QMap<QString, QString> map;
    map["message"] = text;
    map["model"] = ui->comboBox->currentText();

    chatbot->reply(map);
}

void MainWindow::on_historyListItem_clicked(QListWidgetItem *item)
{
    if (item) {
        ui->chatTabs->setCurrentIndex(ui->historyList->row(item));
    } else {
        qDebug() << "Clicked history list item is null.";
    }
}

void MainWindow::on_comboBox_activated(int index)
{
    auto text = ui->comboBox->currentText();
    ui->inputLine->setPlaceholderText(QString("Message ") + text + "...");
}

void MainWindow::on_inputLine_textChanged(const QString &arg1)
{
    if ( ui->sendButton->statusTip() == "Pending") {
        return;
    }
    ui->sendButton->setIcon(QIcon(":/icon/arrow-up-circle.svg"));
    if (arg1.isEmpty()) {
        ui->sendButton->setDisabled(true);
        ui->sendButton->setStatusTip("Nothing");
    }
    else {
        ui->sendButton->setEnabled(true);
        ui->sendButton->setStatusTip("Waiting");
    }
}

void MainWindow::on_inputLine_returnPressed()
{
    QString text = ui->inputLine->text().trimmed();

    addMessage(text);
    ui->inputLine->clear();
}

