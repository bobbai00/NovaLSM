rm -rf /tmp/YCSB-Nova
cp -r "/proj/bg-PG0/bobbai/YCSB-Nova/" /tmp/
cd /tmp/YCSB-Nova
git remote set-url origin https://github.com/bobbai00/NovaLSM-YCSB-Client
git pull
mvn -pl com.yahoo.ycsb:jdbc-binding -am clean package -DskipTests