#include <QApplication>
#include <QStyleFactory>
#include <QMessageBox>
#include <unistd.h>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // تعيين النمط
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // التحقق من صلاحيات المستخدم الجذر
    if (geteuid() != 0) {
        QMessageBox::critical(nullptr, "خطأ", 
            "يتطلب هذا التطبيق صلاحيات المستخدم الجذر (root).\n"
            "الرجاء تشغيل التطبيق باستخدام sudo.");
        return 1;
    }
    
    // تعيين اللغة العربية
    app.setLayoutDirection(Qt::RightToLeft);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
