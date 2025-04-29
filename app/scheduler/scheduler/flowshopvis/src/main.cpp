#include "flowshopvismainwindow.hpp"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[]) {
    const QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon(":/favicon.ico"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Flow shop visualization");
    parser.addHelpOption();
    parser.addPositionalArgument("flowshop", "Flow shop to visualize.");

    // Process the actual command line arguments given by the user
    parser.process(a);

    const QStringList args = parser.positionalArguments();

    FlowshopVisMainWindow w(nullptr);

    if (!args.empty()) {
        w.openFlowShop(args.at(0), false, 0);
    }

    w.show();
    return QApplication::exec();
}
