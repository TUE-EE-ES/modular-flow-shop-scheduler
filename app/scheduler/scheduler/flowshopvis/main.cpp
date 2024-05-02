#include "flowshopvismainwindow.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QTime>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    FlowshopVisMainWindow w;

    QCommandLineParser parser;
    parser.setApplicationDescription("Flow shop visualization");
    parser.addHelpOption();
    parser.addPositionalArgument("flowshop", "Flow shop to visualize.");

    QCommandLineOption onlyGraphOption("g", "Interpret using only the graph");
    parser.addOption(onlyGraphOption);

    // Process the actual command line arguments given by the user
    parser.process(a);

    const QStringList args = parser.positionalArguments();

    if (args.size() > 0) {
        QString filename = args.at(0);
        if (!parser.isSet(onlyGraphOption)) {
            w.openFlowShop(filename);
        } else {
            w.openGraphWithoutFlowshop(filename);
        }
    }

    w.show();

    return a.exec();
}
