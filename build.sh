#!/bin/bash

# سكريبت بناء مدير Wi-Fi
# يتطلب نظام Kali Linux أو توزيعة Debian-based

echo "🔧 مدير شبكة Wi-Fi - سكريپت البناء"
echo "=================================="

# ألوان للنص
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# التحقق من المستخدم الجذر
if [[ $EUID -eq 0 ]]; then
    echo -e "${YELLOW}تحذير: يتم تشغيل السكريپت بصلاحيات root${NC}"
fi

# وظيفة للتحقق من وجود الأوامر
check_command() {
    if command -v "$1" &> /dev/null; then
        echo -e "${GREEN}✓${NC} $1 متوفر"
        return 0
    else
        echo -e "${RED}✗${NC} $1 غير متوفر"
        return 1
    fi
}

# وظيفة لتثبيت الحزم
install_packages() {
    echo -e "${BLUE}📦 تحديث قائمة الحزم...${NC}"
    sudo apt update

    echo -e "${BLUE}📦 تثبيت أدوات التطوير...${NC}"
    sudo apt install -y build-essential cmake git

    echo -e "${BLUE}📦 تثبيت Qt6...${NC}"
    sudo apt install -y qt6-base-dev qt6-charts-dev qt6-tools-dev

    echo -e "${BLUE}📦 تثبيت أدوات الشبكة...${NC}"
    sudo apt install -y wireless-tools net-tools iw nmap arp-scan

    echo -e "${BLUE}📦 تثبيت أدوات اختيارية...${NC}"
    sudo apt install -y hostapd dnsmasq aircrack-ng
}

# فحص المتطلبات
echo -e "${BLUE}🔍 فحص المتطلبات الأساسية...${NC}"

missing_basic=0
check_command "cmake" || ((missing_basic++))
check_command "make" || ((missing_basic++))
check_command "g++" || ((missing_basic++))

# فحص Qt6
if pkg-config --exists Qt6Core Qt6Widgets Qt6Network Qt6Charts; then
    echo -e "${GREEN}✓${NC} Qt6 متوفر"
else
    echo -e "${RED}✗${NC} Qt6 غير متوفر أو ناقص"
    ((missing_basic++))
fi

# فحص أدوات الشبكة
echo -e "${BLUE}🔍 فحص أدوات الشبكة...${NC}"
missing_network=0
check_command "ip" || ((missing_network++))
check_command "arp" || ((missing_network++))

echo -e "${BLUE}🔍 فحص أدوات الشبكة الاختيارية...${NC}"
check_command "nmap"
check_command "arp-scan"
check_command "iwconfig"
check_command "iw"
check_command "nmcli"
check_command "iptables"

# تثبيت المتطلبات المفقودة
if [[ $missing_basic -gt 0 ]]; then
    echo -e "${YELLOW}⚠️  بعض المتطلبات الأساسية مفقودة${NC}"
    read -p "هل تريد تثبيت المتطلبات المفقودة؟ (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        install_packages
    else
        echo -e "${RED}❌ لا يمكن المتابعة بدون المتطلبات الأساسية${NC}"
        exit 1
    fi
fi

# إنشاء مجلد البناء
echo -e "${BLUE}🏗️  إنشاء مجلد البناء...${NC}"
mkdir -p build
cd build

# تشغيل cmake
echo -e "${BLUE}⚙️  تكوين المشروع...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release

if [[ $? -ne 0 ]]; then
    echo -e "${RED}❌ فشل في تكوين المشروع${NC}"
    exit 1
fi

# بناء المشروع
echo -e "${BLUE}🔨 بناء المشروع...${NC}"
make -j$(nproc)

if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}✅ تم بناء المشروع بنجاح!${NC}"
    echo -e "${BLUE}📁 الملف التنفيذي: $(pwd)/WifiManager${NC}"
    
    # إنشاء سكريپت تشغيل
    cat > ../run.sh << 'EOF'
#!/bin/bash
# سكريپت تشغيل مدير Wi-Fi

# التحقق من صلاحيات root
if [[ $EUID -ne 0 ]]; then
    echo "يتطلب هذا التطبيق صلاحيات root"
    echo "استخدم: sudo ./run.sh"
    exit 1
fi

# تشغيل التطبيق
cd "$(dirname "$0")"
./build/WifiManager
EOF
    chmod +x ../run.sh
    
    echo -e "${GREEN}✅ تم إنشاء سكريپت التشغيل: run.sh${NC}"
    echo
    echo -e "${BLUE}🚀 لتشغيل التطبيق:${NC}"
    echo -e "   ${YELLOW}sudo ./run.sh${NC}"
    echo
    echo -e "${BLUE}📋 ملاحظات مهمة:${NC}"
    echo -e "   • يتطلب صلاحيات root للوظائف المتقدمة"
    echo -e "   • تأكد من اتصالك بشبكة Wi-Fi"
    echo -e "   • بعض الوظائف تتطلب إعداد Access Point"
    
else
    echo -e "${RED}❌ فشل في بناء المشروع${NC}"
    exit 1
fi
