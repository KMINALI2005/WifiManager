#!/bin/bash

# سكريپت تثبيت المتطلبات لمدير Wi-Fi
# مخصص لأنظمة Debian/Ubuntu/Kali Linux

echo "🔧 مدير شبكة Wi-Fi - تثبيت المتطلبات"
echo "========================================"

# ألوان للنص
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m'

# التحقق من نوع التوزيعة
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
else
    echo -e "${RED}❌ لا يمكن تحديد نوع التوزيعة${NC}"
    exit 1
fi

echo -e "${BLUE}💻 النظام المكتشف: $OS $VER${NC}"

# التحقق من صلاحيات sudo
if ! sudo -n true 2>/dev/null; then
    echo -e "${YELLOW}🔐 يتطلب هذا السكريپت صلاحيات sudo${NC}"
    echo "الرجاء إدخال كلمة مرور sudo:"
fi

# وظيفة لعرض التقدم
show_progress() {
    echo -e "${PURPLE}▶️  $1${NC}"
}

# وظيفة للتحقق من نجاح الأمر
check_success() {
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ نجح: $1${NC}"
    else
        echo -e "${RED}❌ فشل: $1${NC}"
        exit 1
    fi
}

# تحديث قائمة الحزم
show_progress "تحديث قائمة الحزم"
sudo apt update
check_success "تحديث قائمة الحزم"

# تثبيت أدوات التطوير الأساسية
show_progress "تثبيت أدوات التطوير الأساسية"
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config
check_success "أدوات التطوير الأساسية"

# تثبيت Qt6
show_progress "تثبيت Qt6 ومكتباته"
sudo apt install -y \
    qt6-base-dev \
    qt6-charts-dev \
    qt6-tools-dev \
    libqt6core6 \
    libqt6widgets6 \
    libqt6network6 \
    libqt6charts6
check_success "Qt6"

# تثبيت أدوات الشبكة الأساسية
show_progress "تثبيت أدوات الشبكة الأساسية"
sudo apt install -y \
    wireless-tools \
    net-tools \
    iw \
    iproute2 \
    iputils-ping
check_success "أدوات الشبكة الأساسية"

# تثبيت أدوات فحص الشبكة
show_progress "تثبيت أدوات فحص الشبكة"
sudo apt install -y \
    nmap \
    arp-scan \
    netdiscover
check_success "أدوات فحص الشبكة"

# تثبيت أدوات Access Point (اختيارية)
show_progress "تثبيت أدوات Access Point"
sudo apt install -y \
    hostapd \
    dnsmasq \
    iptables \
    iptables-persistent
check_success "أدوات Access Point"

# تثبيت أدوات إضافية مفيدة
show_progress "تثبيت أدوات إضافية"
sudo apt install -y \
    aircrack-ng \
    macchanger \
    ethtool \
    tcpdump \
    wireshark-common
check_success "أدوات إضافية"

# إعداد permissions لـ wireshark (اختياري)
echo -e "${BLUE}🔧 إعداد صلاحيات wireshark...${NC}"
if command -v wireshark >/dev/null 2>&1; then
    sudo dpkg-reconfigure wireshark-common
fi

# فحص التثبيت
echo
echo -e "${BLUE}🔍 فحص التثبيت...${NC}"
echo "================================"

# فحص Qt6
if pkg-config --exists Qt6Core Qt6Widgets Qt6Network Qt6Charts; then
    QT_VERSION=$(pkg-config --modversion Qt6Core)
    echo -e "${GREEN}✅ Qt6 ($QT_VERSION)${NC}"
else
    echo -e "${RED}❌ Qt6${NC}"
fi

# فحص الأدوات
tools=(
    "cmake:بناء المشروع"
    "make:تجميع الكود"
    "g++:المترجم"
    "nmap:فحص الشبكة"
    "arp-scan:فحص ARP"
    "iwconfig:إعدادات Wi-Fi"
    "iw:أداة Wi-Fi الحديثة"
    "iptables:جدار الحماية"
    "hostapd:Access Point"
    "dnsmasq:خادم DHCP/DNS"
)

for tool_info in "${tools[@]}"; do
    tool=$(echo $tool_info | cut -d: -f1)
    desc=$(echo $tool_info | cut -d: -f2)
    
    if command -v $tool >/dev/null 2>&1; then
        echo -e "${GREEN}✅ $tool${NC} - $desc"
    else
        echo -e "${YELLOW}⚠️  $tool${NC} - $desc (غير متوفر)"
    fi
done

echo
echo -e "${GREEN}🎉 انتهى تثبيت المتطلبات!${NC}"
echo "================================"
echo -e "${BLUE}📋 الخطوات التالية:${NC}"
echo "1. تشغيل سكريپت البناء: ./build.sh"
echo "2. تشغيل التطبيق: sudo ./run.sh"
echo
echo -e "${YELLOW}💡 ملاحظات مهمة:${NC}"
echo "• يتطلب التطبيق صلاحيات root للوظائف المتقدمة"
echo "• تأكد من الاتصال بشبكة Wi-Fi قبل التشغيل"
echo "• بعض الوظائف تحتاج إعداد Access Point"
echo "• استخدم 'فحص المتطلبات' داخل التطبيق للتأكد"

# إنشاء ملف معلومات النظام
cat > system_info.txt << EOF
معلومات النظام - $(date)
=====================================
النظام: $OS $VER
معالج: $(uname -m)
النواة: $(uname -r)

Qt6 Version: $(pkg-config --modversion Qt6Core 2>/dev/null || echo "غير مثبت")
GCC Version: $(gcc --version | head -n1)
CMake Version: $(cmake --version | head -n1)

واجهات الشبكة:
$(ip link show | grep -E '^[0-9]+:' | cut -d: -f2 | sed 's/^ *//')

الأدوات المثبتة:
$(for tool in nmap arp-scan iwconfig iw iptables hostapd dnsmasq; do
    if command -v $tool >/dev/null 2>&1; then
        echo "✅ $tool"
    else
        echo "❌ $tool"
    fi
done)
EOF

echo -e "${BLUE}📄 تم حفظ معلومات النظام في: system_info.txt${NC}"
